#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include "hdf5.h"

#define H5FILE_NAME "/home/fsc/H5data/ID21_scan_Dhyana_0000_projection_0000_data.h5"
#define FILE_NAME_MAX_LENGTH 256
#define DATASET_NAME "/entry/data"
void appendDataToFile(const char* filename, uint16_t* data,int block_z,int block_rows, int block_cols) {
    FILE *file = fopen(filename, "ab"); // 以追加模式打开文件
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    // 写入数据
    fwrite(data, sizeof(uint16_t),block_z* block_rows * block_cols, file);

    fclose(file);
}
double DataBuffer(int data_size_z, hsize_t block_rows, hsize_t block_cols, int offset_size)
{
    hid_t fapl;
    const char *username;

    hid_t file_id, dataset_id, dataspace_id;
    herr_t status;
    // int data[5][6]; // 5x6 的整数数据数组

    uint16_t *data = (uint16_t *)malloc(data_size_z*block_rows * block_cols * sizeof(uint16_t));

    hsize_t count[3] = {data_size_z, block_rows, block_cols};
    // 打开HDF5文件
    file_id = H5Fopen(H5FILE_NAME, H5F_ACC_RDONLY, H5P_DEFAULT);

    // 打开数据集
    dataset_id = H5Dopen2(file_id, DATASET_NAME, H5P_DEFAULT);
    hid_t datatype_id = H5Dget_type(dataset_id);
    // 获取数据集的数据空间
    dataspace_id = H5Dget_space(dataset_id);
    hid_t memspace_id = H5Screate_simple(3, count, NULL);
    hsize_t offset[3] = {0, offset_size, offset_size}; // 从数据集的偏移位置开始读取
    H5Sselect_hyperslab(dataspace_id, H5S_SELECT_SET, offset, NULL, count, NULL);
    /*--------数据传输---------*/
    clock_t start, end;
    double cpu_time_used;

    start = clock(); // 获取程序开始执行的时间

    // 读取数据集的数据
    status = H5Dread(dataset_id, datatype_id, memspace_id, dataspace_id, H5P_DEFAULT, data);

    end = clock(); // 获取程序结束执行的时间

    cpu_time_used = ((double)(end - start)) / CLOCKS_PER_SEC; // 计算程序执行时间
    printf("程序执行时间: %f 秒\n", cpu_time_used);

    /*------数据传输 end--------*/
    // 打印前十个数据
    for (int i = 0; i < 10; i++)
    {
        printf("%zu ", data[i]);
    }
    printf("\n");
    // 关闭数据空间
    H5Sclose(dataspace_id);

    // 关闭数据集
    H5Dclose(dataset_id);

    // 关闭文件
    H5Fclose(file_id);
    appendDataToFile("data.txt", data, data_size_z,block_rows, block_cols);
    
    free(data);
    return cpu_time_used;
}
int main(void)
{
    // 清空文件内容
    const char* filename = "data.txt";

    FILE *file = fopen(filename, "w"); // 以写入模式打开文件，这会清空文件内容
    if (file == NULL) {
        perror("Error opening file");
        return 1;
    }

    fclose(file);
    int array_size = 200;

    double time = DataBuffer(400, 2048, 2048, 0);
    printf("读取时间：%.5f\n", time);


    return 0;
}