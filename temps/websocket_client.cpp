#include <iostream>
#include <string>
#include <unistd.h> // for close
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
// #include "wslay.h"


void handle_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) handle_error("Could not create socket");

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Connect to localhost
    serv_addr.sin_port = htons(8080);

    if (connect(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("Could not connect to server");

    //Simplified WebSocket handshake (missing crucial parts)
    //In a real application, you'd use a proper WebSocket library to handle this.
    //For now we'll just send a dummy handshake.  This will likely fail with a real server.
    std::string handshake = "GET / HTTP/1.1\r\n"
                            "Host: localhost:8080\r\n"
                            "Upgrade: websocket\r\n"
                            "Connection: Upgrade\r\n"
                            "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n" // Replace with a proper key
                            "\r\n";
    send(sockfd, handshake.c_str(), handshake.length(), 0);


    char buffer[1024];
    std::string message;
    while (true) {
        std::cout << "Enter message (or type 'quit' to exit): ";
        std::getline(std::cin, message);
        if (message == "quit") break;

        send(sockfd, message.c_str(), message.length(), 0);

        int bytes_received = recv(sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break; //Connection closed
        buffer[bytes_received] = '\0'; //Null-terminate the received data
        std::cout << "Received: " << buffer << std::endl;
    }

    close(sockfd);
    return 0;
}