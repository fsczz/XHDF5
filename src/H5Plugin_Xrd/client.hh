#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <algorithm> 
#include <sys/mman.h>  // 共享内存
#include <fcntl.h>     // O_* constants
#include <thread>
#define BUFFER_SIZE 2048
#define CHUNK_SIZE 65536 
#define BUFFER_SIZE1 10240  
/**
 * auther@fsc
 * 2023.10.23
 * Connect Xrootd Http Server Class * 
*/
class XrdClient
{
    private:
        // socket
        char *SERVER_IP;  
        int SERVER_PORT;  //port
        char *FILE_PATH;  
        int sockfd;             //socket 
        //dataset information
        std::string tokens;  
        std::string username;  

        struct sockaddr_in server_addr;   
        struct hostent *host;        
    public:
        XrdClient(/* args */){};
        ~XrdClient(){};
        void initSocket(char *tokens);
	    void client();
        std::string  response();
        void closeAll();
};