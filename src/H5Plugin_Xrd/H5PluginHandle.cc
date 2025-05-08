#include <stdio.h>
#include "XrdHttp/XrdHttpExtHandler.hh"
#include "XrdSec/XrdSecEntity.hh"
#include "XrdSfs/XrdSfsInterface.hh"
#include "XrdSys/XrdSysAtomics.hh"
#include "XrdSys/XrdSysFD.hh"

#include "H5PluginHandle.hh"
#include "client.hh"
#include "jsmn.h"
#include <stdlib.h>
#include <fstream>
#include <json/json.h>
#include <sstream>
using namespace std;
XrdVERSIONINFO(XrdHttpGetExtHandler, XrdHTTP);

//------------------------------------------------------------------------------
// Initialize handler
//------------------------------------------------------------------------------
XrdHttpHandler::XrdHttpHandler(XrdSysError *log, const char *config, XrdOucEnv *myEnv) : m_desthttps(false),
                                                                                         m_timeout(60),
                                                                                         m_first_timeout(120),
                                                                                         m_log(log->logger(), "TPC_"),
                                                                                         m_sfs(NULL)
{
    Configure(config, myEnv);
}
static int jsoneq(const char *json, jsmntok_t *tok, const char *s)
{
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0)
    {
        return 0;
    }
    return -1;
}
//------------------------------------------------------------------------------
// Check if request should be handled by the current handler
//------------------------------------------------------------------------------
bool XrdHttpHandler::MatchesPath(const char *verb, const char *path)
{
    // Token validation
    // printf("verb: %s\t path:%s\n", verb, path);
    return true;
}


/* add uuid for dataset of hdf5 file by*/
void add_attribute_to_dataset(hid_t file_id, const std::string& dataset_name, const std::string& attribute_name, const std::string& attribute_value) {
    hid_t dataset_id = H5Dopen2(file_id, dataset_name.c_str(), H5P_DEFAULT);
    hid_t attribute_space_id = H5Screate(H5S_SCALAR);
    hid_t attribute_id = H5Acreate2(dataset_id, attribute_name.c_str(), H5T_C_S1, attribute_space_id, H5P_DEFAULT, H5P_DEFAULT);

    H5Awrite(attribute_id, H5T_C_S1, attribute_value.c_str());

    H5Aclose(attribute_id);
    H5Sclose(attribute_space_id);
    H5Dclose(dataset_id);
}


/* Read file by hdf5 lib————int、string、double、float */
bool XrdHttpHandler::Xrd_read_dataset_info(std::string filepath,
                                           std::string h5path, void **buffer, size_t *DataSize)
{
    /*
     * need datase information of hdf5 file----offset、buffer、 size、 fpath、dbname
     */
    hid_t file_id = H5Fopen(filepath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    // from params get dataset name
    hid_t dataset_id = H5Dopen(file_id, h5path.c_str(), H5P_DEFAULT);
    hid_t dataspace_id = H5Dget_space(dataset_id);
    int rank = H5Sget_simple_extent_ndims(dataspace_id);
    hsize_t dims[rank];
    H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
    hsize_t cumulative_product = 1;

    for (int i = 0; i < rank; i++)
    {
        cumulative_product *= dims[i];
    }
    std::stringstream ss;
    for (int i = 0; i < rank; i++)
    {
        ss << dims[i];
        if (i != rank - 1)
        {
            ss << ",";
        }
    }
    // this->Size = ss.str();

    hid_t datatype_id = H5Dget_type(dataset_id);

    size_t data_size = H5Tget_size(datatype_id);

    *buffer = malloc(cumulative_product * data_size);
    // cout << "Data Size：" << data_size << endl;

    // cout << "size：" << cumulative_product << "\trank:" << rank << "," << dims[0] << "," << dims[1] << "," << dims[2] << endl;
    H5Dread(dataset_id, datatype_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, *buffer);
    *DataSize = cumulative_product * data_size;
    // cout << "Data Size：" << *DataSize << endl;
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);
    return 1;
}

/* Read file by hdf5 lib————int、string、double、float by chunked size */
bool XrdHttpHandler::Xrd_read_dataset_info_by_chunked(std::string filepath,
                                                      std::string h5path, std::string select, void **buffer, size_t *DataSize)
{
    /*
     * need datase information of hdf5 file----offset、buffer、 size、 fpath、dbname
     */
    // Data preprocessing
    std::string input = select;
    std::vector<int> result1;
    std::vector<int> result2;

    input = input.substr(1, input.size() - 2);

    std::stringstream s2s(input);
    std::string token;
    while (std::getline(s2s, token, ','))
    {
        std::stringstream s2s2(token);
        std::string subtoken;
        int count = 0;
        while (std::getline(s2s2, subtoken, ':'))
        {
            if (count == 0)
            {
                result1.push_back(std::stoi(subtoken));
            }
            else if (count == 1)
            {
                result2.push_back(std::stoi(subtoken));
            }
            count++;
        }
    }

    hsize_t offset[result1.size()];
    hsize_t count[result2.size()];
    for (int i = 0; i < result1.size(); i++)
    {
        offset[i] = result1[i];
    }
    for (int i = 0; i < result2.size(); i++)
    {
        count[i] = result2[i]-result1[i];
    }
    // cout << "\t count:" << count[0] << "," << count[1] << "," << count[2] << endl;
    // cout << "\t offset:" << offset[0] << "," << offset[1] << "," << offset[2] << endl;

    hid_t file_id = H5Fopen(filepath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    // from params get dataset name
    hid_t dataset_id = H5Dopen(file_id, h5path.c_str(), H5P_DEFAULT);
    hid_t dataspace_id = H5Dget_space(dataset_id);
    int rank = H5Sget_simple_extent_ndims(dataspace_id);
    hsize_t dims[rank];
    H5Sget_simple_extent_dims(dataspace_id, dims, NULL);
    hsize_t cumulative_product = 1;
    H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, NULL, count, NULL);
    hid_t memspace = H5Screate_simple(result1.size(), count, NULL);
    for (int i = 0; i < rank; i++)
    {
        cumulative_product *= count[i];
    }
    std::stringstream ss;
    for (int i = 0; i < rank; i++)
    {
        ss << dims[i];
        if (i != rank - 1)
        {
            ss << ",";
        }
    }
    // this->Size = ss.str();

    hid_t datatype_id = H5Dget_type(dataset_id);

    size_t data_size = H5Tget_size(datatype_id);

    *buffer = malloc(cumulative_product * data_size);

    H5Dread(dataset_id, datatype_id, memspace, dataspace_id, H5P_DEFAULT, *buffer);
    // size_t type_size = H5Tget_size(datatype);

    *DataSize = cumulative_product * data_size;
    cout << "Data Size：" << *DataSize << endl;
    H5Tclose(datatype_id);
    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Fclose(file_id);
    return 1;
}

/* Create h5 file to server  */
group_info XrdHttpHandler::Xrd_create_file(std::string filepath, std::string *header)
{
    group_info root;
    hid_t file_id;
    // create new file
    file_id = H5Fcreate(filepath.c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);

    struct stat file_info;
    if (stat(filepath.c_str(), &file_info) != 0)
    {
        return root;
    }
    root.root = "g-82578030-0321709e-0adf-08b88b-a9f816";
    root.owner = "fsc";
    root.version = "0.8.4";
    // Obtain timestamps for creation time and last access time
    root.created = file_info.st_ctime;
    root.lastModified = file_info.st_atime;
    root.acls = "{\"fsc\": {\"create\": true, \"read\": true, \"update\": true, \"delete\": true, \"readACL\": true, \"updateACL\": true}}";
    // root.limits="{"min_chunk_size": 1048576, \"max_chunk_size\": 4194304, \"max_request_size\": 104857600}"
    root.compressors = "[\"blosclz\", \"lz4\", \"lz4hc\", \"gzip\", \"zstd\", \"deflate\"]";
    root.hrefs = "";
    // 关闭文件
    H5Fclose(file_id);
    return root;
}

bool XrdHttpHandler::Xrd_delete_file(std::string filepath)
{

    if (remove(filepath.c_str()) == 0)
    {
        printf("%s file delete successful !\n", filepath.c_str());
    }
    else
    {
        return 0;
    }
    return 1;
}

bool h5basetype_string_to_id(hid_t *datatype, std::string base_type)
{
    bool flag = true;
    if (base_type.compare("H5T_STD_I8LE") == 0)
    {
        *datatype = H5T_STD_I8LE;
    }
    else if (base_type.compare("H5T_STD_I16LE") == 0)
    {
        *datatype = H5T_STD_I16LE;
    }
    else if (base_type.compare("H5T_STD_I32LE") == 0)
    {
        *datatype = H5T_STD_I32LE;
    }
    else if (base_type.compare("H5T_STD_I64LE") == 0)
    {
        *datatype = H5T_STD_I64LE;
    }
    else if (base_type.compare("H5T_IEEE_F32LE") == 0)
    {
        *datatype = H5T_IEEE_F32LE;
    }
    else if (base_type.compare("H5T_IEEE_F64LE") == 0)
    {
        *datatype = H5T_IEEE_F64LE;
    }
    else if (base_type.compare("H5T_STD_U16LE") == 0)
    {
        *datatype = H5T_STD_U16LE;
    }
    else if (base_type.compare("H5T_STD_U32LE") == 0)
    {
        *datatype = H5T_STD_U32LE;
    }
    else if (base_type.compare("H5T_STD_U64LE") == 0)
    {
        *datatype = H5T_STD_U64LE;
    }
    else
    {
        flag = false;
    }

    return flag;
}

/* Create dataset of h5 file to server  */
dataset_info XrdHttpHandler::Xrd_create_dataset(std::string filepath, std::string dataset_1)
{
    dataset_info dataset;
    const char *dataset_info = dataset_1.c_str();
    printf("POST DATA BUFFER:%s\n", dataset_info);

    string h5path;
    string base_type;
    int rank = 0;
    std::vector<int> shape;
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    jsmn_init(&p);
    int r = jsmn_parse(&p, dataset_info, strlen(dataset_info), t,
                       sizeof(t) / sizeof(t[0]));

    if (r < 0)
    {
        printf("Failed to parse JSON: %d\n", r);
        return dataset;
    }
    printf("JSON data num:%d\n", r);

    int substringLength;
    for (int i = 1; i < r; i++)
    {
        if (jsoneq(dataset_info, &t[i], "name") == 0)
        {
            substringLength = t[i + 1].end - t[i + 1].start;
            std::string substring(dataset_info + t[i + 1].start, substringLength);
            std::cout << substring << endl;
            h5path = substring;
        }
        else if (jsoneq(dataset_info, &t[i], "shape") == 0)
        {
            if (t[i + 1].type != JSMN_ARRAY)
            {
                continue; /* We expect groups to be an array of strings */
            }
            rank = t[i + 1].size;
            for (int j = 0; j < t[i + 1].size; j++)
            {
                jsmntok_t *g = &t[i + j + 2];
                substringLength = g->end - g->start;
                std::string substring(dataset_info + g->start, substringLength);
                std::cout << substring << endl;
                shape.push_back(std::stoi(substring));
            }
        }
        else if (jsoneq(dataset_info, &t[i], "base") == 0)
        {
            substringLength = t[i + 1].end - t[i + 1].start;
            std::string substring(dataset_info + t[i + 1].start, substringLength);
            std::cout << substring << endl;
            base_type = substring;
        }
        i++;
    }
    printf("file name:%s, dataset name: %s ,base_type: %s ,rank: %d \n", filepath.c_str(), h5path.c_str(), base_type.c_str(), rank);
    if (h5path.empty() || base_type.empty() || rank == 0)
    {
        printf("dataset name: %s ,base_type: %s ,rank: %d \n", h5path.c_str(), base_type.c_str(), rank);
        return dataset;
    }
    hid_t file_id, dataset_id, dataspace_id, data_type;
    hsize_t dims[shape.size()];

    // 将 shape 中的元素复制到 dims 数组中
    for (size_t i = 0; i < shape.size(); i++)
    {
        dims[i] = static_cast<hsize_t>(shape[i]);
    }
    if (!h5basetype_string_to_id(&data_type, base_type))
    {
        return dataset;
    }
    file_id = H5Fopen(filepath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    dataspace_id = H5Screate_simple(rank, dims, NULL);

    dataset_id = H5Dcreate2(file_id, h5path.c_str(), data_type, dataspace_id, H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
    
    // std::string uuid="d-"+generate_uuid();
    // hid_t attribute_space_id = H5Screate(H5S_SCALAR);
    // hid_t attribute_id = H5Acreate2(dataset_id, "uuid", H5T_C_S1, attribute_space_id, H5P_DEFAULT, H5P_DEFAULT);

    // H5Awrite(attribute_id, H5T_C_S1, uuid.c_str());

    // H5Aclose(attribute_id);
    // H5Sclose(attribute_space_id);


    hid_t dcpl = H5Dget_create_plist(dataset_id);
    hsize_t chunk_dims[rank];
    bool chunk_flag = false;
    hid_t layout = H5Pget_layout(dcpl);
    if (layout == H5D_CHUNKED)
    {
        chunk_flag = true;
        H5Pget_chunk(dcpl, rank, chunk_dims);
    }

    std::stringstream ss;
    std::stringstream ss2;
    ss << "[";
    for (int i = 0; i < rank; i++)
    {
        ss << dims[i];
        if (i < rank - 1)
        {
            ss << ", ";
        }
    }
    ss << "]";
    if (chunk_flag)
    {
        ss2 << "[";
        for (int i = 0; i < rank; i++)
        {
            ss2 << chunk_dims[i];
            if (i < rank - 1)
            {
                ss2 << ", ";
            }
        }
        ss2 << "]";
    }
    hid_t datatype = H5Dget_type(dataset_id);
    dataset.shape = "{ \"class\":\"" + h5space_to_string(dataspace_id) + "\",\"dims\":" + ss.str() + ",\"maxdims\":" + ss.str() + "}";
    dataset.type = "{ \"class\":\"" + h5type_to_string(datatype) + "\",\"base\":\"" + base_type + "\"}";
    dataset.layout = chunk_flag ? "{ \"class\":\"H5D_CHUNKED\",\"dims\":" + ss2.str() + "}" : "{ \"class\":\"H5D_CHUNKED\",\"dims\":" + ss.str() + "}";

    H5Dclose(dataset_id);
    H5Sclose(dataspace_id);
    H5Pclose(dcpl);
    H5Tclose(datatype);
    H5Fclose(file_id);


    dataset.attributeCount = 0;
    struct stat file_info;
    if (stat(filepath.c_str(), &file_info) != 0)
    {
        std::cerr << "Error getting file information." << std::endl;
        return dataset;
    }
    dataset.created = file_info.st_ctime;
    dataset.lastModified = file_info.st_atime;
    dataset.id = "d-216ce93b-c0f0-4cfa-af65-15e59b4478ec";
    dataset.root = "g-009094ca-4e1594d1-8818-1c42c6-9d1c59";
    return dataset;
}

/* Write file to server  */
bool Xrd_write_to_dataset(std::string filepath, std::string h5path, void *buffer)
{
    hid_t type_id;

    hid_t file_id = H5Fopen(filepath.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    // from params get dataset name
    hid_t dataset_id = H5Dopen(file_id, h5path.c_str(), H5P_DEFAULT);
    type_id = H5Dget_type(dataset_id);

    H5Dwrite(dataset_id, type_id, H5S_ALL, H5S_ALL, H5P_DEFAULT, buffer);

    // size_t type_size = H5Tget_size(datatype);
    H5Tclose(type_id);
    H5Dclose(dataset_id);
    H5Fclose(file_id);
    return 1;
}

#define MAX_PARAMS 10
#define MAX_PARAM_LENGTH 10240

typedef struct
{
    char key[MAX_PARAM_LENGTH];
    char value[MAX_PARAM_LENGTH];
} Param;

bool is_hdf5_file(const char *file_path)
{
    unsigned char hdf5_magic_number[] = {0x89, 0x48, 0x44, 0x46, 0x0D, 0x0A, 0x1A, 0x0A};
    FILE *file = fopen(file_path, "rb");
    if (file == NULL)
    {
        return false; // no file
    }
    unsigned char file_header[sizeof(hdf5_magic_number)];
    fread(file_header, sizeof(hdf5_magic_number), 1, file);
    fclose(file);
    for (int i = 0; i < sizeof(hdf5_magic_number); i++)
    {
        if (file_header[i] != hdf5_magic_number[i])
        {
            return false; // file is not hdf5 file
        }
    }
    return true; // true hdf5 file
}

/* generate uuid for client */
std::string XrdHttpHandler::generate_uuid() {
    uuid_t uuid;
    uuid_generate(uuid);

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);

    return std::string(uuid_str);
}

std::string get_thread_id_str() {
    std::thread::id this_id = std::this_thread::get_id();
    std::ostringstream oss;
    oss << this_id;
    return oss.str();
}
//------------------------------------------------------------------------------
// Process current request
//------------------------------------------------------------------------------
int XrdHttpHandler::ProcessReq(XrdHttpExtReq &req)
{ 
    dataset_info dataset;
    auto domain = req.headers.find("X-Hdf-domain");         // get file path
    auto context_size = req.headers.find("Content-Length"); // get file path
    void *read_data;
    std::string verbs = req.verb;     // get or post or other methods
    std::string fpath = req.resource; // url path

    printf("\nverbs:%s\t, fpath: %s\tX-Hdf-domain:%s\tContent-Length:%s\n", verbs.c_str(), fpath.c_str(), domain->second.c_str(), context_size->second.c_str());

    std::string h5path;                    // dataset path
    std::string select;                    // dataset path
    std::string filepath = domain->second; // file path
    printf("file params: %s\n", req.headers.find("xrd-http-query")->second.c_str());
    //----------get params information---------
    std::vector<std::string> params;
    std::istringstream iss(req.headers.find("xrd-http-query")->second);
    std::string param;
    while (std::getline(iss, param, '&'))
    {
        if (!param.empty())
            params.push_back(param);
    }
    if (!params.empty())
    {
        for (const auto &p : params)
        {
            std::istringstream paramStream(p);
            std::string key, value;
            std::getline(paramStream, key, '=');
            std::getline(paramStream, value, '=');
            if (key.compare("h5path") == 0)
            {
                h5path = value;
            }
            if (key.compare("select") == 0)
            {
                select = value;
            }
            // std::cout << "Key: " << key << ", Value: " << value << std::endl;
        }
    }
    std::vector<std::string> parts;
    std::istringstream iss2(fpath);
    std::string part;
    while (std::getline(iss2, part, '/'))
    {
        parts.push_back(part);
    }
    // Extract ID information from the third section
    std::string id;
    size_t found = fpath.find("value");
    if (parts.size() >= 3)
    {
        id = parts[2];
    }
    printf("param %d,%d\n", h5path.empty(), id.empty());
    //---------------URI filter---------------
    /* Determine if the file exists */
    if ((!(verbs.compare("PUT") == 0)) && (!is_hdf5_file(filepath.c_str())))
        return req.SendSimpleResp(404, NULL, NULL, (filepath + " does not exist!!!").c_str(), 0);

    /* URI: PUT / HTTP/1.1 (create h5 file)*/
    if ((verbs.compare("PUT") == 0) && (fpath.compare("/") == 0))
    {
        std::string headers = "";
        char *put_data;
        req.BuffgetData(std::stoi(context_size->second), &put_data, false); // get request body
        // printf("POST DATA BUFFER:%s\n", put_data);
        group_info root=Xrd_create_file(filepath, &headers);
        // free(put_data);
        if (root.root.empty())
            return req.SendSimpleResp(400, NULL, NULL, (filepath + " open errors!!!").c_str(), 0);
        return req.SendSimpleResp(201, NULL, headers.c_str(), Xrd_convert_root_to_json(root).c_str(), 0);
    }
    /* URI: PUT /datasets/d-009094ca-4e1594d1-a0ea-ae1336-82a60c/value HTTP/1.1 (write data to dataset)*/
    if ((verbs.compare("PUT") == 0) && (!id.empty()))
    {
        std::string headers = "";
        char *write_data;
        req.BuffgetData(std::stoi(context_size->second), &write_data, false); // get request body
        write_data[std::stoi(context_size->second)] = '\0';
        int status = Xrd_write_to_dataset(filepath, h5path, (void *)write_data);
        // free(put_data);
        if (!status)
            return req.SendSimpleResp(400, NULL, NULL, (filepath + " open errors!!!").c_str(), 0);
        return req.SendSimpleResp(200, NULL, NULL, NULL, 0);
    }
    /* URI: DELETE / HTTP/1.1 */
    if ((verbs.compare("DELETE") == 0) && (fpath.compare("/") == 0))
    {
        std::string headers = "";
        int status = Xrd_delete_file(filepath);
        if (!status)
            return req.SendSimpleResp(400, NULL, NULL, (filepath + " delete errors!!!").c_str(), 0);
        return req.SendSimpleResp(200, NULL, NULL, NULL, 0);
    }
     /* URI: POST /datasets HTTP/1.1 (create datasets)*/
    if ((verbs.compare("POST") == 0) && (fpath.compare("/datasets") == 0))
    {
        std::string headers = "";
        char *post_data;
        req.BuffgetData(std::stoi(context_size->second), &post_data, false); // get request body
        post_data[std::stoi(context_size->second)] = '\0';
        std::string str_post = post_data;
        dataset_info dataset = Xrd_create_dataset(filepath, str_post);
        // free(post_data);
        if (dataset.id.empty())
            return req.SendSimpleResp(400, NULL, NULL, (filepath + " open errors!!!").c_str(), 0);
        printf("POST return dataset info !!");
        return req.SendSimpleResp(201, headers.c_str(), headers.c_str(), Xrd_convert_dataset_to_json(dataset).c_str(), 0);
    }
    if (verbs.compare("GET") == 0)
    {
        /* URI: GET / HTTP/1.1 (get root or group information)*/
        if (h5path.empty() & id.empty())
        {
            // root*、class*、owner*、created、compressors、version*、lastModified、hrefs
            std::string headers = "";
            printf(" GET / HTTP/1.1  path: %s\n", filepath.c_str());
            group_info root = Xrd_get_file_info(filepath, &headers);
            if (root.root.empty())
                return req.SendSimpleResp(400, NULL, NULL, (filepath + " open errors!!!").c_str(), 0);
            printf("response json: %s\n", Xrd_convert_root_to_json(root).c_str());
            return req.SendSimpleResp(200, NULL, headers.c_str(), Xrd_convert_root_to_json(root).c_str(), 0);
        }
        std::string shm_name = "/data_shm_" + generate_uuid();
        std::cout << "shm_name : " << shm_name.c_str() << std::endl;
        /* URI: GET /?h5path=/IntArray&follow_soft_links=1&follow_external_links=1 HTTP/1.1 (get dataset information)*/
        if (id.empty())
        {
            // int pipefd[2];
            size_t DataSize = 0;
            // root*、class*、owner*、created、compressors、version*、lastModified、hrefs
            void *dataset_info_str;
             // create subprocess
            // if (pipe(pipefd) == -1) {
            //      return req.SendSimpleResp(400, NULL, NULL, (filepath + "subprocess create errors!!!").c_str(), 0);
            // }
            //share memory
            // std::string shm_name = "/datasetinfo_shm_" + generate_uuid();
            // std::cout<<"thread_id:  "<< shm_name <<endl;

            int shm_fd = shm_open(shm_name.c_str(), O_CREAT | O_RDWR, 0666);
            if (shm_fd == -1) {
                return req.SendSimpleResp(400, NULL, NULL, (filepath + " shm_open errors!!!").c_str(), 0);
            }

            pid_t pid = fork();

            if (pid < 0)
            {
                return req.SendSimpleResp(400, NULL, NULL, (filepath + "Fork errors!!!").c_str(), 0);
            }
            else if (pid == 0)
            {
                std::string received_data;

                // child process get parameter of parent process (filepath , h5path)
                dataset_info dataset = Xrd_get_dataset_info(filepath, h5path, &received_data);
                std::string data= Xrd_convert_dataset_to_json(dataset);
                const char* data_str =data.c_str();
                size_t Size=data.size();

                if (ftruncate(shm_fd, Size + sizeof(size_t)) == -1)
                {
                    perror("ftruncate");
                    close(shm_fd);
                    exit(1);
                }
                void* shm_ptr = mmap(nullptr, Size + sizeof(size_t), PROT_WRITE, MAP_SHARED, shm_fd, 0);
                if (shm_ptr == MAP_FAILED)
                {
                    perror("mmap");
                    close(shm_fd);
                    exit(1);
                }
                memcpy(shm_ptr, &Size, sizeof(size_t));
                memcpy((char*)shm_ptr + sizeof(size_t), data_str, Size);

                // std::cout << "Child process over, data size:"<< Size<< std::endl;
                munmap(shm_ptr, Size + sizeof(size_t));
                close(shm_fd);
                exit(0);
            }
            else
            {
                int status;
                waitpid(pid, &status, 0);

                if (WIFEXITED(status)) {
                    std::cout << "Child process 2 has exited with status: " << WEXITSTATUS(status) << std::endl;
                }
                // std::cout << "Parent process start"<< std::endl;
                DataSize = *((size_t*)mmap(nullptr, sizeof(size_t), PROT_READ, MAP_SHARED, shm_fd, 0));
                // std::cout << "Parent process received data with size: " << DataSize << std::endl;

                void* shm_ptr = mmap(0, DataSize + sizeof(size_t), PROT_READ, MAP_SHARED, shm_fd, 0);
                dataset_info_str = malloc(DataSize);
                memcpy(dataset_info_str, (char*)shm_ptr + sizeof(size_t), DataSize);

                // std::cout << "Parent process: received data of size " << DataSize << std::endl;

                munmap(shm_ptr, DataSize + sizeof(size_t));
                close(shm_fd);
                shm_unlink(shm_name.c_str());
            }
            close(shm_fd);
            // std::cout<<"dataset_info: "<<(const char*)dataset_info_str<<endl;
            return req.SendSimpleResp(200, NULL, NULL,  reinterpret_cast<const char*>(dataset_info_str), DataSize);
        }
        else if (select.empty())
        {   
            int pipefd[2];
    
            /* URI: GET /datasets/d-009094ca-4e1594d1-a0ea-ae1336-82a60c/value HTTP/1.1 (get dataset all data)*/
            std::string headers = "Content-Type: application/octet-stream";
            // std::cout << "dataset id:" << dataset.id << endl;
            size_t DataSize = 0;
            // create subprocess
            if (pipe(pipefd) == -1) {
                 return req.SendSimpleResp(400, NULL, NULL, (filepath + "subprocess create errors!!!").c_str(), 0);
            }
            pid_t pid = fork();

            if (pid < 0)
            {
                return req.SendSimpleResp(400, NULL, NULL, (filepath + "Fork errors!!!").c_str(), 0);

            }
            else if (pid == 0)
            {
                close(pipefd[0]);
                void* received_data;
                size_t Size = 0;
                bool status = Xrd_read_dataset_info(filepath, h5path, &received_data, &Size);

                write(pipefd[1], &received_data, sizeof(void *));
                write(pipefd[1], &Size, sizeof(size_t));
            }
            else
            {
                close(pipefd[1]);
                read(pipefd[0], &read_data, sizeof(void *));
                read(pipefd[0], &DataSize, sizeof(size_t));

                wait(NULL);
            }
        
            if (DataSize > MAX_DATA_BUFFER)
            {
                // chunk response
                req.StartChunkedResp(200, NULL, headers.c_str());
                int numChunks = DataSize / MAX_DATA_BUFFER + 1;

                for (int i = 0; i < numChunks; i++)
                {
                    size_t offset = i * MAX_DATA_BUFFER;
                    size_t chunkSize = (i == numChunks - 1) ? DataSize - offset : MAX_DATA_BUFFER;
                    void *chunkData = (char *)read_data + offset;
                    // std::cout << "chunk szie:" << chunkSize << endl;
                    req.ChunkResp((const char *)chunkData, chunkSize);
                }
                bool sta=req.ChunkResp(NULL, 0);
                free(read_data);
                return sta;
            }
            bool sta=req.SendSimpleResp(200, NULL, headers.c_str(), (const char *)read_data, DataSize);
            free(read_data);
            return sta;
        }
        else
        {
            /* URI: GET /datasets/d-009094ca-4e1594d1-a0ea-ae1336-82a60c/value?h5path=/entry/data&select=[0:1:1,0:2048:1,0:2048:1]  HTTP/1.1 */
            // chunked read data
            std::string headers = "Content-Type: application/octet-stream";
            // std::cout << "dataset id:" << dataset.id << ", select: " << select << endl;
            size_t DataSize = 0;
            // int pipefd[2];
            // if (pipe(pipefd) == -1) {
            //      return req.SendSimpleResp(400, NULL, NULL, (filepath + "subprocess create errors!!!").c_str(), 0);
            // }
            int pipefd[2]; // 用于读写的文件描述符

            if (pipe(pipefd) == -1) {
                std::cerr << "Failed to create pipe" << std::endl;
                return 1;
            }
            fcntl(pipefd[0], F_SETPIPE_SZ, 8388608000);
            pid_t pid = fork();

            if (pid < 0) {
                std::cerr << "Fork failed" << std::endl;
                return 1;
            } else if (pid == 0) {
                // 子进程
                close(pipefd[0]); // 关闭读端

                void* received_data;
                size_t Size = 0;
                bool status = Xrd_read_dataset_info_by_chunked(filepath, h5path, select, &received_data, &Size);
                std::cout << "Child process received data with size: " << Size << std::endl;
  
                // 写入数据大小
                if (write(pipefd[1], &Size, sizeof(size_t)) == -1) {
                    std::cerr << "Failed to write size to pipe" << std::endl;
                    free(received_data);
                    close(pipefd[1]);
                    _exit(1);
                }

                size_t bytes_written = 0;
                while (bytes_written < Size) {
                    ssize_t result = write(pipefd[1], (char*)received_data + bytes_written, Size - bytes_written);
                    if (result == -1) {
                        std::cerr << "Failed to write to pipe" << std::endl;
                        free(received_data);
                        close(pipefd[1]);
                        _exit(1);
                    }
                    bytes_written += result;
                }

                free(received_data); // 释放数据
                close(pipefd[1]); // 关闭写端
                _exit(0);
            } else {
                // 父进程
                close(pipefd[1]); // 关闭写端

                if (read(pipefd[0], &DataSize, sizeof(size_t)) == -1) {
                    std::cerr << "Failed to read size from pipe" << std::endl;
                    close(pipefd[0]);
                    return 1;
                }

                std::cout << "Parent process received data size: " << DataSize << std::endl;

                read_data = malloc(DataSize);
                if (read_data == nullptr) {
                    std::cerr << "Failed to allocate memory" << std::endl;
                    close(pipefd[0]);
                    return 1;
                }
                size_t bytes_read = 0;
                while (bytes_read < DataSize) {
                    ssize_t result = read(pipefd[0], (char*)read_data + bytes_read, DataSize - bytes_read);
                    if (result == -1) {
                        std::cerr << "Failed to read from pipe" << std::endl;
                        free(read_data);
                        close(pipefd[0]);
                        return 1;
                    }
                    bytes_read += result;
                }
                if (bytes_read != DataSize) {
                    std::cerr << "Incomplete read from pipe: expected " << DataSize << " bytes, but got " << bytes_read << " bytes" << std::endl;
                }
                
                // 使用接收到的数据
                // std::cout << "Parent process received data: " << static_cast<char*>(read_data) << std::endl;

                // free(read_data);
                close(pipefd[0]); // 关闭读端

                int status;
                waitpid(pid, &status, 0); // 等待子进程结束
            }
            
            
            const char* compression_way = std::getenv("COMPRESSION");
            const char* compression_level_std = std::getenv("COMPRESSION_LEVEL");
            int compression_level = std::stoi(compression_level_std);
            
            std::string Compression(compression_way);
            std::cout << "COMPRESSION TYPE:" << Compression << endl;
            std::cout << "COMPRESSION LEVEL:" << compression_level << endl;
            if(Compression=="ZSTD"){
                // compression data
                size_t max_compressed_size = ZSTD_compressBound(DataSize);
                void* compressed_data = malloc(max_compressed_size);
                size_t compressed_size = ZSTD_compress(compressed_data, max_compressed_size, read_data, DataSize, compression_level);
                DataSize=compressed_size;
                free(read_data);
                read_data=compressed_data;
                std::cout <<max_compressed_size<< "  压缩后大小:" << DataSize << endl;
                //set headers
                headers+="\nContent-Encoding:zstd\nContent-Length:"+std::to_string(DataSize);
            } else if(Compression=="ZLIB"){
                size_t max_compressed_size = compressBound(DataSize);
                void* compressed_data = malloc(max_compressed_size);
                uLongf compressed_size = max_compressed_size; // 压缩后实际大小
                int ret = compress2((Bytef *)compressed_data, &compressed_size, (const Bytef *)read_data, DataSize, compression_level);
                if (ret != Z_OK) {
                    fprintf(stderr, "Compression failed with error code: %d\n", ret);
                    free(compressed_data);
                    return 1;
                }
                DataSize=compressed_size;
                free(read_data);
                read_data=compressed_data;
                std::cout << "压缩后大小:" << DataSize << endl;
                //set headers
                headers+="\nContent-Encoding:zlib\nContent-Length:"+std::to_string(DataSize);
            }else if(Compression=="LZ4"){
                size_t max_compressed_size =  LZ4_compressBound(DataSize);
                void* compressed_data = malloc(max_compressed_size);
                int compressed_size  = LZ4_compress_HC((const char*)read_data, (char*)compressed_data, DataSize, max_compressed_size, compression_level);
                if (compressed_size <= 0) {
                    fprintf(stderr, "压缩失败\n");
                    free(compressed_data);
                    return 1;
                }
                DataSize=compressed_size;
                free(read_data);
                read_data=compressed_data;
                std::cout << "压缩后大小:" << DataSize << endl;
                //set headers
                headers+="\nContent-Encoding:lz4\nContent-Length:"+std::to_string(DataSize);
            }

            if (DataSize > MAX_DATA_BUFFER)
            {
                // chunk response
                req.StartChunkedResp(200, NULL, headers.c_str());
                size_t numChunks = DataSize / MAX_DATA_BUFFER + 1;

                for (int i = 0; i < numChunks; i++)
                {
                    size_t offset = i * (size_t)MAX_DATA_BUFFER;
                    size_t chunkSize = (i == numChunks - 1) ? DataSize - offset : MAX_DATA_BUFFER;
                    void *chunkData = (char *)read_data + offset;
                    
                    req.ChunkResp((const char *)chunkData, chunkSize);
                }
                bool sta=req.ChunkResp(NULL, 0);
                free(read_data);
                return sta;
            }
            bool sta=req.SendSimpleResp(200, NULL, headers.c_str(), (const char *)read_data, DataSize);
            free(read_data);
            return sta;
        }
    }
    return req.SendSimpleResp(400, NULL, NULL, "parameter error !!!", 0);
}
//----------------------------------------------------------------------------
/******************************************************************************/
/*                          Xrd_get_file_info                                 */
/******************************************************************************/
group_info XrdHttpHandler::Xrd_get_file_info(std::string filepath, string *headers)
{
    // set headers
    group_info root;
    // set root info
    root.class_name = "domain";
    printf("get file created time\n");
    // root*、class*、owner*、version*、created、lastModified、compressors、hrefs
    root.owner = "fsc";
    root.version = "0.8.1";
    printf("get file created time\n");
    struct stat file_info;
    if (stat(filepath.c_str(), &file_info) != 0)
    {
        return root;
    }

    // Obtain timestamps for creation time and last access time
    root.created = file_info.st_ctime;
    root.lastModified = file_info.st_atime;
    // set hrefs
    root.hrefs = "[{\"rel\": \"self\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/\"}, {\"rel\": \"database\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/datasets\"}, {\"rel\": \"groupbase\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/groups\"}, {\"rel\": \"typebase\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/datatypes\"}, {\"rel\": \"root\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/groups/g-009094ca-4e1594d1-8818-1c42c6-9d1c59\"}, {\"rel\": \"acls\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/acls\"}, {\"rel\": \"parent\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/?domain=hsds.test/home/fsc\"}]";
    root.compressors = "[\"blosclz\", \"lz4\", \"lz4hc\", \"gzip\", \"zstd\", \"deflate\"]";
    root.root = "g-009094ca-4e1594d1-8818-1c42c6-9d1c59";
    
    return root;
}
//----------------------------------------------------------------------------
/******************************************************************************/
/*                          Xrd_get_dataset_info                              */
/**********************************************************·********************/
dataset_info XrdHttpHandler::Xrd_get_dataset_info(std::string filepath, std::string h5path, std::string *headers)
{
    // set headers
    dataset_info dataset;
    // set root info
    dataset.class_name = "dataset";
    dataset.domain = filepath;
    dataset.root = "g-009094ca-4e1594d1-8818-1c42c6-9d1c59";
    dataset.dataset_path = h5path;
    // id*、class*、root*、domain*、created、lastModified、type、shape、layout、hrefs
    dataset.attributeCount = 0;

    struct stat file_info;
    if (stat(filepath.c_str(), &file_info) != 0)
    {
        std::cerr << "Error getting file information." << std::endl;
        return dataset;
    }
    printf("dataset path:%s\n", filepath.c_str());
    // get type、shape、layout、hrefs
     // 打开 HDF5 文件
    hid_t file_id = H5Fopen(filepath.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        std::cout << "Failed to open file." << std::endl;
    }

    // 打开数据集
    hid_t dataset_id = H5Dopen(file_id, h5path.c_str(), H5P_DEFAULT);
    if (dataset_id < 0) {
        std::cout << "Failed to open dataset." << std::endl;
        H5Fclose(file_id);
    }

    dataset.id = "d-216ce93b-c0f0-4cfa-af65-15e59b4478ec";

    // shape
    hid_t dataspace = H5Dget_space(dataset_id);
    int rank = H5Sget_simple_extent_ndims(dataspace);
    hsize_t dims[rank];
    H5Sget_simple_extent_dims(dataspace, dims, NULL);
    // type
    hid_t datatype = H5Dget_type(dataset_id);
    // layout
    hid_t dcpl = H5Dget_create_plist(dataset_id);
    hsize_t chunk_dims[rank];
    bool chunk_flag = false;
    hid_t layout = H5Pget_layout(dcpl);
    if (layout == H5D_CHUNKED)
    {
        chunk_flag = true;
        H5Pget_chunk(dcpl, rank, chunk_dims);
    }

    std::stringstream ss;
    std::stringstream ss2;
    ss << "[";
    for (int i = 0; i < rank; i++)
    {
        ss << dims[i];
        if (i < rank - 1)
        {
            ss << ", ";
        }
    }
    ss << "]";
    if (chunk_flag)
    {
        ss2 << "[";
        for (int i = 0; i < rank; i++)
        {
            ss2 << chunk_dims[i];
            if (i < rank - 1)
            {
                ss2 << ", ";
            }
        }
        ss2 << "]";
    }
    string base_type_name = h5basetype_to_string(datatype);
    dataset.shape = "{ \"class\":\"" + h5space_to_string(dataspace) + "\",\"dims\":" + ss.str() + ",\"maxdims\":" + ss.str() + "}";
    dataset.type = base_type_name.compare("") ? "{ \"class\":\"" + h5type_to_string(datatype) + "\",\"base\":\"" + base_type_name + "\"}" : "{ \"class\":\"" + h5type_to_string(datatype) + "\"}";
    dataset.layout = chunk_flag ? "{ \"class\":\"H5D_CHUNKED\",\"dims\":" + ss2.str() + "}" : "{ \"class\":\"H5D_CHUNKED\",\"dims\":" + ss.str() + "}";

    // 关闭资源
    H5Pclose(dcpl);
    H5Sclose(dataspace);
    H5Tclose(datatype);
    H5Dclose(dataset_id);
    H5Fclose(file_id);

    // Obtain timestamps for creation time and last access time
    dataset.created = file_info.st_ctime;
    dataset.lastModified = file_info.st_atime;
    // set hrefs
    dataset.hrefs = "[{\"rel\": \"self\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/\"}, {\"rel\": \"database\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/datasets\"}, {\"rel\": \"groupbase\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/groups\"}, {\"rel\": \"typebase\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/datatypes\"}, {\"rel\": \"root\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/groups/g-009094ca-4e1594d1-8818-1c42c6-9d1c59\"}, {\"rel\": \"home\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/\"}, {\"rel\": \"acls\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/acls\"}, {\"rel\": \"parent\", \"href\": \"https://hepslustretest02.ihep.ac.cn:1094/?domain=hsds.test/home/fsc\"}]";
    return dataset;
}

std::string XrdHttpHandler::h5type_to_string(hid_t type)
{
    H5T_class_t class_id = H5Tget_class(type);
    if (class_id == H5T_INTEGER)
    {
        return "H5T_INTEGER";
    }
    else if (class_id == H5T_FLOAT)
    {
        return "H5T_FLOAT";
    }
    else if (class_id == H5T_TIME)
    {
        return "H5T_TIME";
    }
    else if (class_id == H5T_STRING)
    {
        return "H5T_STRING";
    }
    else if (class_id == H5T_BITFIELD)
    {
        return "H5T_BITFIELD";
    }
    else
    {
        return "";
    }
}

std::string XrdHttpHandler::h5space_to_string(hid_t dataspace)
{
    H5S_class_t space_class = H5Sget_simple_extent_type(dataspace);
    std::string space_class_string;

    // 判断数据空间的类别并返回相应的字符串表示
    switch (space_class)
    {
    case H5S_SCALAR:
        space_class_string = "H5S_SCALAR";
        break;
    case H5S_SIMPLE:
        space_class_string = "H5S_SIMPLE";
        break;
    case H5S_NULL:
        space_class_string = "H5S_NULL";
        break;
    case H5S_NO_CLASS:
        space_class_string = "H5S_NO_CLASS";
        break;
    default:
        space_class_string = "";
        break;
    }

    return space_class_string;
}

std::string XrdHttpHandler::h5basetype_to_string(hid_t datatype)
{
    hid_t base_type = H5Tget_native_type(datatype, H5T_DIR_DEFAULT);
    std::string base_type_string;

    if (H5Tequal(base_type, H5T_STD_I8LE))
    {
        base_type_string = "H5T_STD_I8LE";
    }
    else if (H5Tequal(base_type, H5T_STD_I16LE))
    {
        base_type_string = "H5T_STD_I16LE";
    }
    else if (H5Tequal(base_type, H5T_STD_I32LE))
    {
        base_type_string = "H5T_STD_I32LE";
    }
    else if (H5Tequal(base_type, H5T_STD_I64LE))
    {
        base_type_string = "H5T_STD_I64LE";
    }
    else if (H5Tequal(base_type, H5T_IEEE_F32LE))
    {
        base_type_string = "H5T_IEEE_F32LE";
    }
    else if (H5Tequal(base_type, H5T_IEEE_F64LE))
    {
        base_type_string = "H5T_IEEE_F64LE";
    }
    else if (H5Tequal(base_type, H5T_STD_U16LE))
    {
        base_type_string = "H5T_STD_U16LE";
    }
    else if (H5Tequal(base_type, H5T_STD_U32LE))
    {
        base_type_string = "H5T_STD_U32LE";
    }
    else if (H5Tequal(base_type, H5T_STD_U64LE))
    {
        base_type_string = "H5T_STD_U64LE";
    }
    else
    {
        base_type_string = "";
    }
    // 关闭数据类型
    H5Tclose(base_type);

    return base_type_string;
}

std::string timeToString(std::time_t time)
{
    return std::to_string(static_cast<float>(time));
}
std::string XrdHttpHandler::Xrd_convert_root_to_json(const group_info &info)
{
    std::stringstream ss;
    ss << "{";
    ss << "\"root\": \"" << info.root << "\", ";
    if (!info.class_name.empty())
    {
        ss << "\"class\": \"" << info.class_name << "\", ";
    }
    ss << "\"owner\": \"" << info.owner << "\", ";
    if (!info.acls.empty())
    {
        ss << "\"acls\": " << info.acls << ", ";
    }
    if (!info.compressors.empty())
    {
        ss << "\"compressors\": " << info.compressors << ", ";
    }
    if (!info.hrefs.empty())
    {
        ss << "\"hrefs\": " << info.hrefs << ", ";
    }
    ss << "\"version\": \"" << info.version << "\", ";
    ss << "\"created\": " << timeToString(info.created) << ", ";
    ss << "\"lastModified\": " << timeToString(info.lastModified);

    ss << "}";
    return ss.str();
}

std::string XrdHttpHandler::Xrd_convert_dataset_to_json(const dataset_info &info)
{
    std::stringstream ss;
    ss << "{";
    ss << "\"id\": \"" << info.id << "\", ";
    ss << "\"root\": \"" << info.root << "\", ";
    ss << "\"attributeCount\": " << info.attributeCount << ", ";
    ss << "\"created\": " << timeToString(info.created) << ", ";
    if (!info.domain.empty())
    {
        ss << "\"domain\": \"" << info.domain << "\", ";
    }
    if (!info.class_name.empty())
    {
        ss << "\"class\": \"" << info.class_name << "\", ";
    }
    if (!info.hrefs.empty())
    {
        ss << "\"hrefs\": " << info.hrefs << ", ";
    }
    ss << "\"shape\": " << info.shape << ", ";
    ss << "\"type\": " << info.type << ", ";
    ss << "\"layout\": " << info.layout << ",";
    ss << "\"creationProperties\": {}, "; //  must
    ss << "\"lastModified\": " << timeToString(info.lastModified);
    ss << "}";
    return ss.str();
}

//----------------------------------------------------------------------------
/******************************************************************************/
/*                    X r d H t t p G e t E x t H a n d l e +r               */
/******************************************************************************/

class XrdSysError;
class XrdOucEnv;
#define XrdHttpExtHandlerArgs XrdSysError *eDest, \
                              const char *confg,  \
                              const char *parms,  \
                              XrdOucEnv *myEnv

extern "C" XrdHttpExtHandler *XrdHttpGetExtHandler(XrdHttpExtHandlerArgs)
{
    XrdHttpExtHandler *handler = new XrdHttpHandler(eDest, confg, myEnv);
    handler->Init(confg);
    return handler;
}