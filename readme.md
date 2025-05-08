## XHDF5 ：A High-performance HDF5 Remote Data Access System For HEPS

The High-Energy Photon Source (HEPS) is is the first fourth-generation synchrotron light source facility in China, which will generate huge amounts of scientific data. HDF5 is a widely used file format for scientific data storage and management, which has been established as the standard data format by domestic synchrotron facilities. As the storage scale of the HEPS data center expands and international collaboration increases, remote access systems have become essential for remote users. Therefore, it is urgently needed to develop an efficient and adaptable remote access solution based on the HDF5 data format to improve the efficiency and performance of scientific computing and data analysis. Based on in-depth research into HDF5 technology and the XRootD distributed file system, we designed and implemented the XHDF5 system. Compared with the traditional NFS access mode, XHDF5 has higher efficiency and stability in cross-region data sharing and collaboration, and provides a more reliable remote data access platform for researchers.

## XHDF5 INSTALL

Installation example, the environment is ubuntu 24.0. 

### 1、VOL_REST  INSTALL

#### （1）HDF5 install

~~~shell
# install gcc / g++
tar -xzvf hdf5-1.14.2.tar.gz
cd hdf5-1.14.2
./configure --prefix=/usr/hdf5 \
--enable-fortran \
--enable-shared \
--enable-threadsafe \
--enable-unsupported 
make -j$(nproc)
make install
~~~

#### （2）HDF5 Environment variable Settings

~~~shell
# env 
echo -e "#hdf5_env\nHDF5_ROOT=/usr/hdf5" >> ~/.bashrc
echo -e 'export PATH=$HDF5_ROOT:$PATH' >> ~/.bashrc
echo -e 'export PATH=$HDF5_ROOT/bin:$PATH' >> ~/.bashrc
echo -e 'export LD_LIBRARY_PATH=$HDF5_ROOT/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc
~~~

#### （3）Dependent installation （libcurl、libyajl）

~~~shell
# libcurl（7.79.0）
sudo apt install -y build-essential autoconf automake libtool
# Dependent
sudo apt install -y libssl-dev zlib1g-dev
tar -xzvf curl-7.79.0.tar.gz
cd curl-7.79.0
./configure --prefix=/usr/local \
            --with-openssl \
            --with-zlib
# --enable-http2       # enable HTTP/2
# --disable-shared     # only static library
# --enable-debug       # debug 
make -j$(nproc)
sudo make install
curl --version
# libyajl
sudo apt install libyajl-dev
# zstd and lz4 library install
sudo apt install libzstd-dev
sudo apt install liblz4-dev
~~~

#### （4）Vol-rest plugin Install

~~~shell
sudo tar -xvf beifen_vol.tar.gz
cd rest-vol
scl enable devtoolset-9 bash

./configure  CFLAGS="-std=c99" --prefix=/usr/vol-rest --with-hdf5=/usr/hdf5 --with-openssl=/usr
make -j$(nproc)
make install
~~~

#### （5）Environment variable Settings

~~~shell
vim ~/.bashrc
# vol-rest enable 
export HDF5_PLUGIN_PATH=/usr/vol-rest/lib
export HDF5_VOL_CONNECTOR=REST

export SN_PORT=80
export HSDS_ENDPOINT=https://10.5.125.25:1091

export SERVER_NUM=3
export XRDMULTI_ENDPOINT="{https://10.5.125.25:1091,https://10.5.125.25:1092,https://10.5.125.25:1093,https://10.5.125.25:1094,https://10.5.125.25:1095,https://10.5.125.25:1096}"

# resolve libhdf5_vol_rest.so.0 => not found question
ln -s /usr/vol-rest/lib/libhdf5_vol_rest.so.0 /usr/lib/libhdf5_vol_rest.so.0  
ldconfig
cp vol-rest/src/thread_pool.h /usr/vol-rest/include
~~~

 ubuntu error：configure: error: cannot compute sizeof (off_t) ;

 Solution:  Manually bypass it. Add "ac_cv_sizeof_off_t=8"  in configure file

### 2、XRootD Plugin INSTALL

#### （1）Dependent installation

~~~shell
（1）zstd and lz4 library install
sudo apt install libzstd-dev
sudo apt install liblz4-dev
# gcc/g++
sudo apt install build-essential
# xrootd
sudo apt install -y xrootd-server xrootd-client
sudo apt install libxrootd-dev # Or download the source code and install it
（2）HDF5 install
tar -xzvf hdf5-1.14.2.tar.gz
cd hdf5-1.14.2
./configure --prefix=/usr/hdf5 \
--enable-fortran \
--enable-shared \
--enable-threadsafe \
--enable-unsupported
make -j$(nproc)
make install
# env
echo -e "#hdf5_env\nHDF5_ROOT=/usr/hdf5" >> ~/.bashrc
echo -e 'export PATH=$HDF5_ROOT:$PATH' >> ~/.bashrc
echo -e 'export PATH=$HDF5_ROOT/bin:$PATH' >> ~/.bashrc
echo -e 'export LD_LIBRARY_PATH=$HDF5_ROOT/lib:$LD_LIBRARY_PATH' >> ~/.bashrc
source ~/.bashrc

（3）libcurl （7.69.0以上）
sudo apt install -y build-essential autoconf automake libtool
# Dependency library
sudo apt install -y libssl-dev zlib1g-dev
tar -xzvf curl-7.79.0.tar.gz
cd curl-7.79.0
./configure --prefix=/usr/local \
            --with-openssl \
            --with-zlib

# --enable-http2       # enable HTTP/2
# --disable-shared     # only static library
# --enable-debug       # debug 
make -j$(nproc)
sudo make install
curl --version
# uuid error
sudo apt install uuid-dev
~~~

#### （2）Plugin install


~~~shell
tar -xvf H5Plugin_Xrd.tar
sudo ./start.sh
~~~

#### （3）Configuration file (xrootd-http.cfg)

~~~shell
all.export /tmp
# http service startup component
xrd.protocol http libXrdHttp.so
# https Certificate
http.cert /etc/pki/tls/certs/vbrid.crt
http.key  /etc/pki/tls/certs/vbrid.key
# http.cafile /home/fsc/ca/myCA
# The plugin library generated in Step 2
http.exthandler xrdtpc /etc/xrootd/libXrdExtHttp-5.so

#http.secxtractor /usr/lib64/libXrdLcmaps.so
http.listingdeny yes
http.desthttps yes
# Pass the bearer token to the Xrootd authorization framework.
#http.header2cgi Authorization authz
ofs.authlib libxrdlacaroons.so 
~~~

#### （4）Start Service

~~~shell
# Start directly through the command line
xrootd -c xrootd-http.cfg
~~~

~~~shell
# service start
cp xrootd-http.cfg /etc/xrootd/xrootd-http.cfg

# Start the service
sudo systemctl start xrootd@http.service
# Check the status of the service
sudo systemctl status xrootd@http.service
# View the log file
cat /var/log/xrootd/http/xrootd.log

# 问题1  -lXrdHttpUtils 该库在不同的系统位于不同位置
# 在最新的ubuntu中在/usr/lib/x86_64-linux-gnu/  centos中位于/usr/lib64下，根据不同的位置进行修改

# 问题2  libXrdExtHttp.so库需要libhdf5库，在xrootd用户中没有配置的话是找不到libhdf5库
# 此时会报
Plugin /usr/lib64/libXrdExtHttp.so: undefined symbol: _ZN13XrdHttpExtReq11BuffgetDataEiPPcb exthandlerlib libXrdExtHttp.so
# 解决方案：
sudo vim /etc/systemd/system/xrootd@http.service.d/override.conf
添加以下内容：(/usr/hdf5为hdf5库的安装位置)
[Service]
Environment="LD_LIBRARY_PATH=/usr/lib64:/usr/lib:/lib64:/lib"
Environment="HDF5_ROOT=/usr/hdf5"
Environment="PATH=/usr/hdf5:/usr/hdf5/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
Environment="LD_LIBRARY_PATH=/usr/hdf5/lib:/usr/lib64:/usr/lib:/lib64:/lib"
~~~




