#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
// #include <wslay/wslay.h>

// Error handling (still minimal)
void handle_error(const std::string& msg) {
    std::cerr << "Error: " << msg << std::endl;
    exit(1);
}

//Simplified Echo Handler for a single client connection
void handle_client(int client_sockfd) {
  //This is a VERY simplified example.  Real-world usage requires significantly more error handling and proper WebSocket handling with wslay.

    char buffer[1024];
    while (true) {
        int bytes_received = recv(client_sockfd, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) break; // Connection closed

        if (send(client_sockfd, buffer, bytes_received, 0) < 0) {
            handle_error("Could not send data");
        }
    }
    close(client_sockfd);
    std::cout << "Client disconnected." << std::endl;
}


int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) handle_error("Could not create socket");

    sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(8080);

    if (bind(sockfd, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
        handle_error("Could not bind socket");

    if (listen(sockfd, 10) < 0) handle_error("Could not listen on socket");


    std::vector<std::thread> clients;
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_sockfd = accept(sockfd, (sockaddr*)&client_addr, &client_len);
        if (client_sockfd < 0) {
            std::cerr << "Error accepting connection" << std::endl; //Don't exit on accept error
            continue;
        }
        clients.emplace_back(handle_client, client_sockfd);
    }

    //This will never be reached in this simplified example.  In a real app you'd need to manage client threads gracefully.
    for (auto& t : clients) {
        t.join();
    }

    close(sockfd);
    return 0;
}