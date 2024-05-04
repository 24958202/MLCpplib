#include <iostream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> // Include this header for the close function

int main() {
    // Initialize OpenSSL
    SSL_library_init();
    SSL_load_error_strings();
    ERR_load_BIO_strings();
    OpenSSL_add_all_algorithms();

    // Create an SSL context
    SSL_CTX *ctx = SSL_CTX_new(SSLv23_client_method());
    SSL_CTX_load_verify_locations(ctx, "/home/ronnieji/lib/MLCpplib-main/ca_certificates/ca.crt", NULL);

    if (!ctx) {
        std::cerr << "Error creating SSL context" << std::endl;
        return 1;
    }

    // Create a socket
    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket < 0) {
        std::cerr << "Error creating client socket" << std::endl;
        return 1;
    }

    // Connect to the server
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080); // Example server port
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Example server IP address

    if (connect(clientSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Error connecting to the server" << std::endl;
        return 1;
    }

    // Create an SSL object
    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientSocket);

    // Perform SSL handshake
    if (SSL_connect(ssl) <= 0) {
        fprintf(stderr, "Error performing SSL handshake\n");
        SSL_free(ssl);
        close(clientSocket);
        return 1;
    }

    // Send a message to the server
    std::string message = "Hello from the client!";
    SSL_write(ssl, message.c_str(), message.length());

    // Receive a response from the server
    char buffer[1024];
    int bytesRead = SSL_read(ssl, buffer, sizeof(buffer));
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::cout << "Received from server: " << buffer << std::endl;
    } else {
        std::cerr << "Error receiving response from server" << std::endl;
    }

    // Clean up
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(clientSocket);
    SSL_CTX_free(ctx);
    EVP_cleanup();
    ERR_free_strings();

    return 0;
}