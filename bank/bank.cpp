#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <string>

#define PORT 9999
#define BUFFER_SIZE 1024

struct EntityInfo {
    std::string name;
    std::string ip;
    int port;
};

std::vector<EntityInfo> entities;

void registerEntity(const std::string &name, const std::string &ip, int port) {
    entities.push_back({name, ip, port});
    std::cout << "Registered: " << name << " (IP: " << ip << ", Port: " << port << ")\n";
}

[[noreturn]] void setUpBank() {
    std::cout << "Entering bank setup...\n";

    int sockfd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr{}, client_addr{};

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind the socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Bank listening on port " << PORT << " (UDP)\n";

    while (true) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';

        // Extract ip and port of client
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        int client_port = ntohs(client_addr.sin_port);

        std::string message(buffer);

        // extract message -->
    }

    close(sockfd);
}

int main() {
    //setUpBank();
}
