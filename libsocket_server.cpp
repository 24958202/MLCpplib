#include <iostream>
#include <thread>
#include <vector>
#include <curl/curl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>

// const options = {
//   key: fs.readFileSync('/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.key'),
//   cert: fs.readFileSync('/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.crt')
// };
void handleClientConnectionSSL(SSL* ssl) {
    char buffer[1024];
    int bytesRead;

    // Read data from the client
    while ((bytesRead = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
        // Process the received data (e.g., perform some operation)
        // Here, we simply print the received data
        buffer[bytesRead] = '\0'; // Null-terminate the buffer
        std::cout << "Received data from client: " << buffer << std::endl;

        // Send a response back to the client
        SSL_write(ssl, "Server received your message", strlen("Server received your message"));
    }

    // Handle SSL_read errors or client disconnection
    if (bytesRead < 0) {
        std::cerr << "Error reading data from client" << std::endl;
    }

    // Clean up SSL connection
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

int main() {
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
        return 1;
    }

    // Set up SSL certificate and key (replace with your own certificate and key setup)
    // Bind the server socket to a port
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Bind to port 8080
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Accept connections on any interface
    SSL_CTX_use_certificate_file(ctx, "/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.crt", SSL_FILETYPE_PEM);
    SSL_CTX_use_PrivateKey_file(ctx, "/home/ronnieji/lib/MLCpplib-main/ca_certificates/server.key", SSL_FILETYPE_PEM);
    // Create a server socket
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    // Create an SSL object
    SSL *ssl = SSL_new(ctx);
    SSL_set_fd(ssl, serverSocket);

    // Perform SSL handshake
    SSL_accept(ssl);
    
    if (serverSocket < 0) {
        std::cerr << "Error creating server socket" << std::endl;
        return 1;
    }
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error binding server socket to port" << std::endl;
        return 1;
    }
    // Start listening for incoming connections 5 is the pending connections for the server socket may grow.
    if (listen(serverSocket, 5) < 0) {
        std::cerr << "Error listening for incoming connections" << std::endl;
        return 1;
    }

    std::vector<std::thread> threads;

    while (true) {
        // Accept incoming client connections
        struct sockaddr_in clientAddr;
        socklen_t clientAddrLen = sizeof(clientAddr);
        int clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket < 0) {
            std::cerr << "Error accepting client connection" << std::endl;
            continue;
        }

        // Create an SSL object
        SSL* ssl = SSL_new(ctx);
        SSL_set_fd(ssl, clientSocket);

        // Perform SSL handshake
        if (SSL_accept(ssl) <= 0) {
            std::cerr << "Error performing SSL handshake" << std::endl;
            SSL_free(ssl);
            close(clientSocket);
            continue;
        }

        // Create a new thread to handle the SSL client connection
        threads.emplace_back(handleClientConnectionSSL, ssl);
    }

    // Join all threads
    for (auto& t : threads) {
        t.join();
    }

    // Cleanup cURL and OpenSSL
    curl_global_cleanup();
   // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    ERR_free_strings();

    return 0;
}