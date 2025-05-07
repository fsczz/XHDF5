#include <iostream>
#include <string>
#include <hdf5.h>

void readHDF5Attribute(const std::string& file_path, const std::string& dataset_name, const std::string& attribute_name, std::string& attribute_value) {
    // 打开HDF5文件
    hid_t file_id = H5Fopen(file_path.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "无法打开文件: " << file_path << std::endl;
        return;
    }

    // 打开数据集
    hid_t dataset_id = H5Dopen(file_id, dataset_name.c_str(), H5P_DEFAULT);
    if (dataset_id < 0) {
        std::cerr << "无法打开数据集: " << dataset_name << std::endl;
        H5Fclose(file_id);
        return;
    }

    // 打开属性
    hid_t attr_id = H5Aopen(dataset_id, attribute_name.c_str(), H5P_DEFAULT);
    if (attr_id < 0) {
        std::cerr << "无法打开属性: " << attribute_name << std::endl;
        H5Dclose(dataset_id);
        H5Fclose(file_id);
        return;
    }

    // 获取属性类型
    hid_t attr_type = H5Aget_type(attr_id);
    hid_t attr_native_type = H5Tget_native_type(attr_type, H5T_DIR_DEFAULT);

    // 获取属性大小
    size_t attr_size = H5Aget_storage_size(attr_id)+50;

    // 读取属性值
    char* attr_value = new char[attr_size + 1];  // 加1用于字符串终止符
    H5Aread(attr_id, attr_native_type, attr_value);
    attr_value[attr_size] = '\0';  // 添加字符串终止符

    // 转换为std::string
    attribute_value = std::string(attr_value);

    // 输出属性值
    std::cout << "属性值: " << attribute_value << std::endl;

    // 清理资源
    delete[] attr_value;
    H5Tclose(attr_native_type);
    H5Tclose(attr_type);
    H5Aclose(attr_id);
    H5Dclose(dataset_id);
    H5Fclose(file_id);
}

int main() {
    std::string file_path = "SDS2.h5";
    std::string dataset_name = "data";
    std::string attribute_name = "uuid";
    std::string uuid;

    readHDF5Attribute(file_path, dataset_name, attribute_name, uuid);

    return 0;
}
