#include "nems_server.h"

void handleClientConnectionSSL(SSL* ssl) {
    char buffer[1024];
    int bytesRead;
    // Read data from the client
    while ((bytesRead = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
        // Process the received data
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        std::cout << "Received data from client: " << buffer << std::endl;
        // Send a response back to the client
        SSL_write(ssl, "Server received your message", strlen("Server received your message"));
        /*
            process buffer 
        */
        // Notify listeners that data has been received
        if (onDataReceived) {
            onDataReceived(buffer);
        }
        /*
            end processing
        */
    }
    // Handle SSL_read errors or client disconnection
    if (bytesRead < 0) {
        std::cerr << "Error reading data from client" << std::endl;
    }
    // Clean up SSL connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void createServer(const uint16_t& portNum, const std::string& crt_file_path, const std::string& key_file_path, const uint16_t& maxConnectionNum) {
    // Initialize cURL and OpenSSL
    curl_global_init(CURL_GLOBAL_ALL);
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    // Create an SSL context
    SSL_CTX* ctx = SSL_CTX_new(SSLv23_server_method());
    if (!ctx) {
        std::cerr << "Error creating SSL context" << std::endl;
        return;
    }

    // Set up SSL certificate and key
    SSL_CTX_use_certificate_file(ctx, crt_file_path.c_str(), SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, key_file_path.c_str(), SSL_FILETYPE_PEM);

    // Bind the server socket to a port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, serverSocket);
    SSL_accept(ssl);

    if (serverSocket < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return;
    }

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket to port" << std::endl;
        return;
    }

    if (listen(serverSocket, maxConnectionNum) < 0) {
        std::cerr << "Error listening for incoming connections" << std::endl;
        return;
    }

    std::cout << "Waiting for the connection..." << '\n';
    std::vector<std::thread> threads;

    while (true) {
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);

        if (clientSocket < 0) {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, clientSocket);

        if (SSL_accept(ssl) <= 0) {
            std::cerr << "Error performing SSL handshake" << std::endl;
            SSL_free(ssl);
            close(clientSocket);
            continue;
        }

        threads.emplace_back(handleClientConnectionSSL, ssl);
    }

    for (auto& t : threads) {
        t.join();
    }

    // Cleanup
    curl_global_cleanup();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    ERR_free_strings();
}