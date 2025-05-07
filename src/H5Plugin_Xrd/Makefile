PROG=XrdTpcConfigure.o H5PluginHandle.o client.o libXrdExtHttp.so
LIBS=-lXrdCl 
SLIBS=-lXrdHttpUtils -lXrdUtils -lXrdFfs  -lhdf5 -lssl -lcrypto -luuid -lzstd -lz -llz4
CPPFLAGS= -std=c++11 -g -fPIC -I/usr/include -I/usr/include/xrootd -I/usr/include/xrootd/private -I/usr/hdf5/include -L/usr/hdf5/lib -L/usr/lib64
LAG=XrdTpcConfigure.o H5PluginHandle.o client.o
all: ${PROG} 
# 生成目标文件

%.o: %.c
	g++ $(CPPFLAGS) -c $< -o $@  ${SLIBS} 
	
%.o: %.cc
	g++ $(CPPFLAGS) -c $< -o $@  ${SLIBS} 

libXrdExtHttp.so:
	g++ -shared -o libXrdExtHttp.so  ${CPPFLAGS} ${LAG} ${SLIBS}
clean:
	rm -rf *.o *a.out ${PROG}
