#ifndef NEMS_SERVER_H
#define NEMS_SERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <netinet/in.h>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <curl/curl.h>

void handleClientConnectionSSL(SSL* ssl);
void createServer(const uint16_t& portNum, const std::string& crt_file_path, const std::string& key_file_path, const uint16_t& maxConnectionNum);

#endif // NEMS_SERVER_H
