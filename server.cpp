#include <iostream>
#include <unordered_map>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 1024

struct Entity {
    std::string ip;
    int port;
};

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrLen = sizeof(clientAddr);

    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);

    if (bind(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    std::unordered_map<std::string, Entity> entityRegistry;

    std::cout << "Server is running on port 8080..." << std::endl;

    while (true) {
        char buffer[BUFFER_SIZE];
        memset(buffer, 0, BUFFER_SIZE);

        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';
        std::string message(buffer);

        std::string clientIP = inet_ntoa(clientAddr.sin_addr);
        int clientPort = ntohs(clientAddr.sin_port);

        if (message == "REGISTER") {
            std::cout << "New entity registered: " << clientIP << ":" << clientPort << std::endl;

            // Add the entity to the registry
            entityRegistry[clientIP + ":" + std::to_string(clientPort)] = {clientIP, clientPort};

            // Notify all entities (excluding the new one) about the new entity
            for (const auto &[key, entity] : entityRegistry) {
                if (entity.ip != clientIP || entity.port != clientPort) {
                    std::string notification = "NEW_ENTITY " + clientIP + " " + std::to_string(clientPort);

                    struct sockaddr_in entityAddr;
                    memset(&entityAddr, 0, sizeof(entityAddr));
                    entityAddr.sin_family = AF_INET;
                    entityAddr.sin_port = htons(entity.port);
                    entityAddr.sin_addr.s_addr = inet_addr(entity.ip.c_str());

                    sendto(sockfd, notification.c_str(), notification.size(), 0,
                           (struct sockaddr *)&entityAddr, sizeof(entityAddr));
                }
            }
        }
    }

    close(sockfd);
    return 0;
}
