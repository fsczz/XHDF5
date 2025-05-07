cmake_minimum_required(VERSION 2.8) 

project(XrdExtHttp)
# std=c++11 表示设置C++标准是11
set(CMAKE_CXX_FLAGS "-fPIC  -std=c++11 ${CMAKE_CXX_FLAGS}")
set(mylib_dir /root/fsclib/Httplib)
# 生成链接库
add_library(XrdExtHttp SHARED ${mylib_dir}/EosFstHttpHandler.hh  ${mylib_dir}/EosFstHttpHandler.cc)

# 设置头文件搜索路径
target_include_directories(XrdExtHttp PUBLIC ${PROJECT_BINARY_DIR})
target_include_directories(XrdExtHttp PUBLIC /usr/hdf5/include)
target_include_directories(XrdExtHttp PUBLIC /usr/include/xrootd)
target_include_directories(XrdExtHttp PUBLIC /usr/include/xrootd/private)

# 在项目中添加链接库
# target_link_libraries(XrdExtHttp PUBLIC mylib_math)
# target_link_libraries(XrdExtHttp PUBLIC hdf5)

# 修改bug