#include <iostream>
#include <unordered_map>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>

#define BUFFER_SIZE 1024

struct Entity {
    std::string ip;
    int port;
};

void handleNotifications(int sockfd);
void sendRegistration(int sockfd, const std::string &serverIP, int serverPort);

int main() {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    struct sockaddr_in entityAddr;
    memset(&entityAddr, 0, sizeof(entityAddr));
    entityAddr.sin_family = AF_INET;
    entityAddr.sin_addr.s_addr = INADDR_ANY;
    entityAddr.sin_port = htons(0); // Dynamically bind to any available port

    if (bind(sockfd, (struct sockaddr *)&entityAddr, sizeof(entityAddr)) < 0) {
        perror("Bind failed");
        return 1;
    }

    struct sockaddr_in localAddr;
    socklen_t addrLen = sizeof(localAddr);
    getsockname(sockfd, (struct sockaddr *)&localAddr, &addrLen);

    std::string localIP = inet_ntoa(localAddr.sin_addr);
    int localPort = ntohs(localAddr.sin_port);

    std::cout << "Entity running at " << localIP << ":" << localPort << std::endl;

    std::thread notificationThread(handleNotifications, sockfd);

    // Send registration to the server
    sendRegistration(sockfd, "127.0.0.1", 8080);

    notificationThread.join();
    close(sockfd);

    return 0;
}

void handleNotifications(int sockfd) {
    std::unordered_map<std::string, Entity> knownEntities;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in senderAddr, newEntityAddr;
    socklen_t addrLen = sizeof(senderAddr);

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&senderAddr, &addrLen);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';
        std::string message(buffer);

        if (message.rfind("NEW_ENTITY ", 0) == 0) {
            // Extract new entity's IP and port
            size_t spacePos = message.find(' ', 11);
            std::string newIP = message.substr(11, spacePos - 11);
            int newPort = std::stoi(message.substr(spacePos + 1));

            std::cout << "Notified of new entity at " << newIP << ":" << newPort << std::endl;

            // Add the new entity to known entities
            knownEntities[newIP + ":" + std::to_string(newPort)] = {newIP, newPort};

            // Prepare the address of the new entity
            memset(&newEntityAddr, 0, sizeof(newEntityAddr));
            newEntityAddr.sin_family = AF_INET;
            newEntityAddr.sin_port = htons(newPort);
            newEntityAddr.sin_addr.s_addr = inet_addr(newIP.c_str());

            // Send a HELLO message to the new entity
            std::string selfIntroduction = "HELLO_FROM_EXISTING_ENTITY " + newIP + " " + std::to_string(newPort);
            if (sendto(sockfd, selfIntroduction.c_str(), selfIntroduction.size(), 0,
                       (struct sockaddr *)&newEntityAddr, sizeof(newEntityAddr)) < 0) {
                perror("Failed to send HELLO to new entity");
            } else {
                std::cout << "Sent HELLO to new entity at " << newIP << ":" << newPort << std::endl;
            }
        } else if (message.rfind("HELLO_FROM_EXISTING_ENTITY ", 0) == 0) {
            // Extract sender's IP and port
            size_t spacePos = message.find(' ', 27);
            std::string senderIP = message.substr(27, spacePos - 27);
            int senderPort = std::stoi(message.substr(spacePos + 1));

            std::cout << "Received HELLO from " << senderIP << ":" << senderPort << std::endl;

            // Add the sender to known entities
            knownEntities[senderIP + ":" + std::to_string(senderPort)] = {senderIP, senderPort};
        }
    }
}


void sendRegistration(int sockfd, const std::string &serverIP, int serverPort) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());

    std::string registrationMessage = "REGISTER";
    sendto(sockfd, registrationMessage.c_str(), registrationMessage.size(), 0,
           (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    std::cout << "Registration sent to server at " << serverIP << ":" << serverPort << std::endl;
}
