#pragma once
#include <map>
#include <string>
#include "XrdHttp/XrdHttpExtHandler.hh"
#include "XrdSys/XrdSysPthread.hh"
#include "XrdSfs/XrdSfsInterface.hh"
#include "XrdVersion.hh"
#include "XrdTls/XrdTlsTempCA.hh"
#include "hdf5.h"
#include <curl/curl.h>
#include <iostream>
#include <uuid/uuid.h>
#include <vector>
#include <sys/stat.h>
#include <ctime>
#include <zstd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <cstring>
#include <zlib.h>
#include <lz4.h>
#include <lz4hc.h>

#define ID_MAX_LENGTH 128
#define NAME_MAX_LENGTH 256
#define URI_MAX_LENGTH 256
#define MAX_DATA_BUFFER 1024000000

class XrdLink;
class XrdSecEntity;
class XrdHttpReq;
class XrdHttpProtocol;

typedef struct
{
  std::string id;
  std::string root;
  int attributeCount;
  std::time_t created;
  std::time_t lastModified;
  std::string dataset_path;
  std::string class_name;
  std::string domain;

  std::string hrefs;
  std::string shape;
  std::string type;
  std::string layout;
} dataset_info;


// typedef struct
// {
//   int min_chunk_size=1048576;
//   int max_chunk_size=4194304;
//   int max_request_size=104857600;
// } limit;
//data of open file operation 
typedef struct
{
  std::string root;
  std::string class_name;
  std::string owner;
  std::string version;
  std::string acls;
  std::time_t created;
  std::time_t lastModified;

  std::string compressors; // 字符串链表
  std::string hrefs;
  std::string limits;
} group_info;


//------------------------------------------------------------------------------
//! Class XrdHttpHandler
//------------------------------------------------------------------------------
class XrdHttpHandler : public XrdHttpExtHandler
{
public:
  //----------------------------------------------------------------------------
  //! Constructor
  //----------------------------------------------------------------------------
  XrdHttpHandler(XrdSysError *log, const char *config, XrdOucEnv *myEnv);
  //----------------------------------------------------------------------------
  //! Destructor
  //----------------------------------------------------------------------------
  virtual ~XrdHttpHandler() = default;
  bool MatchesPath(const char *verb, const char *path);
  int ProcessReq(XrdHttpExtReq &);
  virtual int Init(const char *cfgfile) { return 0; }

private:
  bool Configure(const char *configfn, XrdOucEnv *myEnv);
  // bool ConfigureLogger(XrdOucStream &Config);

  static int m_marker_period;
  static size_t m_block_size;
  static size_t m_small_block_size;
  bool m_desthttps;
  int m_timeout;        // the 'timeout interval'; if no bytes have been received during this time period, abort the transfer.
  int m_first_timeout;  // the 'first timeout interval'; the amount of time we're willing to wait to get the first byte.
                        // Unless explicitly specified, this is 2x the timeout interval.
  std::string m_cadir;  // The directory to use for CAs.
  std::string m_cafile; // The file to use for CAs in libcurl
  static XrdSysMutex m_monid_mutex;
  static uint64_t m_monid;
  XrdSysError m_log;
  XrdSfsFileSystem *m_sfs;
  bool usingEC; // indicate if XrdEC is used
  bool ChunkedRespFlag =false; // Enable chunking response
  // std::shared_ptr<XrdTlsTempCA> m_ca_file;
  std::shared_ptr<XrdTlsTempCA> m_ca_file;

  void ReadHdf5(size_t offset,
                char **buffer,
                size_t size, char *fpath, char *dbname);
  int WriteHdf5(char *fpath, char *dbname, char *dims, char *data, char *datatype);
  group_info Xrd_create_file(std::string filepath,std::string * header);

  std::string generate_uuid();
  bool Xrd_read_dataset_info(std::string filepath,std::string h5path,void **buffer,size_t *data_size);
  bool Xrd_read_dataset_info_by_chunked(std::string filepath,std::string h5path,std::string select,void **buffer,size_t *data_size);
  dataset_info Xrd_create_dataset(std::string filepath,std::string dataset_1);
  bool Xrd_delete_file(std::string filepath);
  std::string h5type_to_string(hid_t type);
  std::string h5space_to_string(hid_t dataspace);
  std::string h5basetype_to_string(hid_t datatype);
  group_info Xrd_get_file_info(std::string filepath,std::string *headers);
  dataset_info Xrd_get_dataset_info(std::string filepath,std::string h5path, std::string *headers);
  std::string Xrd_convert_root_to_json(const group_info &info);
  std::string Xrd_convert_dataset_to_json(const dataset_info &info);
 
  
};
