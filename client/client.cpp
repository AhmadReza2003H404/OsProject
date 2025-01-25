#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_PORT 9999
#define BUFFER_SIZE 1024

void renderMenu() {
    std::cout << "0: Cryptocurrency price inquiry" << std::endl;
    std::cout << "1: Get exchanges list" << std::endl;
    std::cout << "2: Buy cryptocurrency" << std::endl;
    std::cout << "3: Sell cryptocurrency" << std::endl;
    std::cout << "4: View wallet balance" << std::endl;
    std::cout << "5: Requesting a balance increase from the bank" << std::endl;
    std::cout << "6: View transaction history" << std::endl;
    std::cout << "7: Exit" << std::endl;

}

void handleClient(int sockfd, struct sockaddr_in sockaddr_in) {
    bool isRunning = true;
    std::string choice;
    while (isRunning) {
        renderMenu();
        std::getline(std::cin, choice);
        if (choice == "0") {
            priceInquiry(sockfd, sockaddr_in);
        } else if (choice == "1") {
            getExchangesList(sockfd, sockaddr_in);
        } else if (choice == "2") {
            buyCryptocurrency(sockfd, sockaddr_in);
        } else if (choice == "3") {
            sellCryptocurrency(sockfd, sockaddr_in);
        } else if (choice == "4") {
            viewWalletBallance(sockfd, sockaddr_in);
        } else if (choice == "5") {
            increaseWalletBallance(sockfd, sockaddr_in);
        } else if (choice == "6") {
            viewTransactionHistory(sockfd, sockaddr_in);
        } else if (choice == "7") {
            isRunning = false;
        } else {
            std::cout << "Invalid choice!" << std::endl;
        }
    }
}


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

    // Todo: format this message
    // Send registration message
    sendto(sockfd, name.c_str(), name.size(), 0, (const struct sockaddr *) &server_addr, sizeof(server_addr));

    // Todo: check message format
    // Receive acknowledgment
    socklen_t len = sizeof(server_addr);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &server_addr, &len);
    if (n < 0) {
        perror("Receive failed");
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }

    handleClient(sockfd, server_addr);

    close(sockfd);
}

int main() {
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    registerWithBank(name, "127.0.0.1");
    return 0;
}
