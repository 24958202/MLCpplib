#ifndef NEMS_CLIENT_H
#define NEMS_CLIENT_H
#include <iostream>
#include <string>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // Include this header for the close function
std::string send_to_server(const uint16_t& portNum, const std::string& serverip,const std::string& crt_file_path, const std::string& strMsg);
#endif
