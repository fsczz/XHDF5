#include <stdio.h>
#include <uuid/uuid.h>
#include <iostream>
#include <string>
#include "hdf5.h"

void add_attribute_to_dataset(hid_t file_id, const std::string& dataset_name, const std::string& attribute_name, const std::string& attribute_value) {
    hid_t dataset_id = H5Dopen2(file_id, dataset_name.c_str(), H5P_DEFAULT);
    hid_t attribute_space_id = H5Screate(H5S_SCALAR);
     hid_t str_type = H5Tcopy(H5T_C_S1);
    H5Tset_size(str_type, attribute_value.size());
    hid_t attribute_id = H5Acreate2(dataset_id, attribute_name.c_str(), str_type, attribute_space_id, H5P_DEFAULT, H5P_DEFAULT);
   
    printf("attribute value: %s",attribute_value.c_str());
    H5Awrite(attribute_id, str_type, attribute_value.c_str());

    H5Aclose(attribute_id);
    H5Sclose(attribute_space_id);
    H5Dclose(dataset_id);
}
int main() {
    uuid_t uuid;
    uuid_generate(uuid);

    char uuid_str[37];
    uuid_unparse(uuid, uuid_str);


    const std::string file_name = "SDS2.h5";
    const std::string dataset_name = "data";
    const std::string attribute_name = "uuid";
    const std::string attribute_value = uuid_str;

    hid_t file_id = H5Fopen(file_name.c_str(), H5F_ACC_RDWR, H5P_DEFAULT);
    if (file_id < 0) {
        std::cerr << "Error opening file." << std::endl;
        return -1;
    }

    add_attribute_to_dataset(file_id, dataset_name, attribute_name, attribute_value);

    H5Fclose(file_id);

    printf("Generated UUID: %s\n", uuid_str);

    return 0;
}
