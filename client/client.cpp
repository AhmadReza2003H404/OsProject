#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 9999
#define BUFFER_SIZE 1024

void registerWithBank(const std::string &name, const std::string &server_ip) {
    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr{};

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr);

    // Send registration message
    sendto(sockfd, name.c_str(), name.size(), 0, (const struct sockaddr *)&server_addr, sizeof(server_addr));

    bool isRunning = true;
    while (isRunning) {

    }
    // Receive acknowledgment
    // socklen_t len = sizeof(server_addr);
    // int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &len);
    // if (n < 0) {
    //     perror("Receive failed");
    // } else {
    //     buffer[n] = '\0';
    //     std::cout << "Bank response: " << buffer << "\n";
    // }
    //
    // close(sockfd);
}

int main() {
    std::string name;
    std::cout << "Enter your name: ";
    std::cin >> name;

    registerWithBank(name, "127.0.0.1");
    return 0;
}
