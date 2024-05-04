#include "nems_client.h"
std::string send_to_server(const uint16_t& portNum, const std::string& serverip,const std::string& crt_file_path,const std::string& strMsg){
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    // Create an SSL context
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    SSL_CTX_load_verify_locations(ctx, crt_file_path.c_str(), NULL);//"/home/ronnieji/lib/MLCpplib-main/ca_certificates/ca.crt"
    
    std::string strMsgReturn;

    if (!ctx) {
        std::cerr << "Error creating SSL context" << std::endl;
        return strMsgReturn;
    }

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return strMsgReturn;
    }

    // Connect to the server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum); // Example server port 8080
    serverAddr.sin_addr.s_addr = inet_addr(serverip.c_str()); // Example server IP address 127.0.0.1

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error connecting to the server" << std::endl;
        return strMsgReturn;
    }

    // Create an SSL object
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientSocket);

    // Perform SSL handshake
    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "Error performing SSL handshake\n");
        SSL_free(ssl);
        close(clientSocket);
        return strMsgReturn;
    }

    // Send a message to the server
    SSL_write(ssl, strMsg.c_str(), strMsg.length());

    // Receive a response from the server
    char buffer[1024];
    int bytesRead = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received from server: " << buffer << std::endl;
    } else {
        std::cerr << "Error receiving response from server" << std::endl;
    }
    strMsgReturn = std::string(buffer); // Convert char array to std::string
    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(clientSocket);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    ERR_free_strings();

    return strMsgReturn;
}
