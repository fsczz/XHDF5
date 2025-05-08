#include "client.hh"
#include <fstream>
#include <string>
#include <map>
#include <utility>
#include <sstream>
#include <vector>
#include <json/json.h>
using namespace std;
void XrdClient::initSocket(char *tokens)
{
    char *server_ip ="identificationtgt.ihep.ac.cn";
    int server_port=80;

    this->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->sockfd < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_port = htons(server_port);
    this->host = gethostbyname(server_ip);
    this->server_addr.sin_addr = *((struct in_addr *)host->h_addr_list[0]);
    bzero(&(this->server_addr.sin_zero), 8);
    // init 
    this->tokens = tokens;
    this->SERVER_PORT = server_port;
    this->SERVER_IP = server_ip;
}
void XrdClient::client()
{
    if (connect(this->sockfd, (struct sockaddr *)&(this->server_addr), sizeof(this->server_addr)) < 0)
    {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }
    char request[BUFFER_SIZE];

    const char *FILE_PATH = "/chktgtapi";
    std::string result = "type=chktgt&tokens=";
    result+=this->tokens;
    const char *PARAMS = result.c_str();
    snprintf(request, BUFFER_SIZE, "POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: Custom User-Agent\r\nAccept-Encoding: gzip, deflate\r\nAccept: */*\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s", FILE_PATH, this->SERVER_IP, strlen(PARAMS), PARAMS);
    if (send(this->sockfd, request, strlen(request), 0) < 0)
    {
        perror("Send failed");
        exit(EXIT_FAILURE);
    }
}
std::string extractUsername(const std::string& jsonStr) {
    std::size_t startPos = jsonStr.find("\"user\": \"");
    if (startPos == std::string::npos) {
        return "";  // User name field not found
    }
    
    startPos += 9;  // Skip field names and colons
    std::size_t endPos = jsonStr.find("\"", startPos);
    if (endPos == std::string::npos) {
        return "";  //End quote not found for field value
    }
    
    return jsonStr.substr(startPos, endPos - startPos);
}

std::string XrdClient::response()
{

    char response[BUFFER_SIZE1];
    int bytes_read;
    int content_length = -1;
    int body_start = -1;
    int body_total = 0;
    std::map<std::string, std::string> headers;
    std::string response_data = "";
    // Loop reading return value
    if ((bytes_read = recv(this->sockfd, response, BUFFER_SIZE1 - 1, 0)) > 0)
    {
        response[bytes_read] = '\0';
        
        if (body_start == -1)
        {
            
            char *header_data = new char[strlen(response) + 1]; 
            strcpy(header_data, response);

            char *body_start_ptr = strstr(response, "\r\n\r\n");
            if (body_start_ptr != NULL)
            {
                body_start = body_start_ptr - response + 4;
            }

            
            char *line = strtok(header_data, "\r\n");
            while (line != nullptr)
            {
                std::string line1(line);
                size_t colonPos = line1.find(":");
                if (colonPos != std::string::npos)
                {
                    std::string key = line1.substr(0, colonPos);
                    std::string value = line1.substr(colonPos + 2); // Skip the space after colon
                    headers[key] = value;
                }
                line = strtok(nullptr, "\n");
            }
            free(header_data);
        }
        else
        {
            body_start = 0;
        }
    
        if (body_start != -1)
        {
            std::string jsonStr = response + body_start;
            std::string user = extractUsername(jsonStr);

            if(!user.empty()){
                return user;
            }
        }
        if (content_length == -1)
        {
            char *content_length_ptr = strstr(response, "Content-Length:");
            if (content_length_ptr != NULL)
            {
                content_length = atoi(content_length_ptr + 15);
            }
        }
        body_total += bytes_read;
    }

    if (bytes_read < 0)
    {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }
    return "";
}
void XrdClient::closeAll()
{
    close(this->sockfd);
}