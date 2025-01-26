#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include "HasFunction.h"

#define SERVER_PORT 9999
#define BUFFER_SIZE 4096

const std::string SERVICE_IP = "127.0.0.1";

void renderMenu();

void priceInquiry();

void getExchangesList(int sockfd, struct sockaddr_in sockaddr_in);

void buy_sellCryptocurrency(int assigned_port, std::string buy_sell);

void viewWalletBalance(int sockfd, struct sockaddr_in sockaddr_in, int assigned_port);

void increaseWalletBalance(int sockfd, struct sockaddr_in sockaddr_in, int assigned_port);

void viewTransactionHistory(int sockfd, struct sockaddr_in sockaddr_in);

void handleClient(int sockfd, int bankSocketFd, struct sockaddr_in addr, struct sockaddr_in bank_server_addr,
                  int assigned_port);

void registerWithBank(const std::string &name, const std::string &server_ip);

int main() {
    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    registerWithBank(name, "127.0.0.1");
    return 0;
}

void priceInquiry() {
    std::string exchangePort, cryptocurrencyName;
    std::cout << "Enter exchange port: ";
    std::getline(std::cin, exchangePort);
    std::cout << "Enter Cryptocurrency name: ";
    std::getline(std::cin, cryptocurrencyName);

    int sockfd;
    struct sockaddr_in addr{};
    try {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(std::stoi(exchangePort));
        inet_pton(AF_INET, SERVICE_IP.c_str(), &addr.sin_addr);

        // Create a UDP socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        // Set 2-second timeout for recvfrom
        timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Failed to set socket timeout");
            close(sockfd);
            return;
        }

        const std::string message = "INQUIRY | " + cryptocurrencyName;
        const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
        sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &addr,
               sizeof(addr));
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(addr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &addr, &len);
        if (n < 0) {
            perror("Receive failed or timeout occurred");
        } else {
            buffer[n] = '\0';
            std::cout << "Response: " << buffer << std::endl;
        }
    } catch (...) {
        std::cout << "An exception has occured!\n";
    }
}

void getExchangesList(int sockfd, struct sockaddr_in sockaddr_in) {
    const std::string message = "GET_EXCHANGE_LIST";
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &sockaddr_in,
           sizeof(sockaddr_in));

    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(sockaddr_in);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sockaddr_in, &len);
    if (n < 0) {
        perror("Receive failed");
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }
}

void buy_sellCryptocurrency(int assigned_port, std::string buy_sell) {
    std::string exchangePort, cryptocurrencyName, count;
    std::cout << "Enter exchange port: ";
    std::getline(std::cin, exchangePort);
    std::cout << "Enter Cryptocurrency name: ";
    std::getline(std::cin, cryptocurrencyName);
    std::cout << "Enter Cryptocurrency count: ";
    std::getline(std::cin, count);

    int sockfd;
    struct sockaddr_in addr{};
    try {
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons(std::stoi(exchangePort));
        inet_pton(AF_INET, SERVICE_IP.c_str(), &addr.sin_addr);

        // Create a UDP socket
        if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }
        // Set 2-second timeout for recvfrom
        timeval timeout;
        timeout.tv_sec = 2;
        timeout.tv_usec = 0;
        if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
            perror("Failed to set socket timeout");
            close(sockfd);
            return;
        }

        const std::string message = buy_sell + " | " + cryptocurrencyName + " | COUNT | " + count + " | PORT | " + std::to_string(assigned_port);
        const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
        sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &addr,
               sizeof(addr));
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(addr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &addr, &len);
        if (n < 0) {
            perror("Receive failed or timeout occurred");
        } else {
            buffer[n] = '\0';
            std::cout << "Response: " << buffer << std::endl;
        }
    } catch (...) {
        std::cout << "An exception has occured!\n";
    }
}


void viewWalletBalance(int sockfd, struct sockaddr_in sockaddr_in, int assigned_port) {
    const std::string message = "GET_ACCOUNT_BALANCE | " + std::to_string(assigned_port);
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &sockaddr_in,
           sizeof(sockaddr_in));
    // Receive acknowledgment
    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(sockaddr_in);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sockaddr_in, &len);
    if (n < 0) {
        perror("Receive failed");
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }
}

void increaseWalletBalance(int sockfd, struct sockaddr_in sockaddr_in, int assigned_port) {
    std::cout << "Enter increase amount" << std::endl;
    std::string amountStr;
    std::getline(std::cin, amountStr);
    try {
        std::stoi(amountStr);
        const std::string message = "INCREASE_ACCOUNT_BALANCE | " + std::to_string(assigned_port) + " | " + amountStr;
        const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
        sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
        // Receive acknowledgment
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(sockaddr_in);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sockaddr_in, &len);
        if (n < 0) {
            perror("Receive failed");
        } else {
            buffer[n] = '\0';
            std::cout << "Bank response: " << buffer << "\n";
        }
    } catch (...) {
        std::cout << "Invalid input" << std::endl;
    }
}


void viewTransactionHistory(int sockfd, struct sockaddr_in sockaddr_in, int assigned_port) {
    const std::string message = "GET_ACCOUNT_HISTORY | " + std::to_string(assigned_port);
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &sockaddr_in,
           sizeof(sockaddr_in));

    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(sockaddr_in);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sockaddr_in, &len);
    if (n < 0) {
        perror("Receive failed");
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }
}

void renderMenu() {
    std::cout << "0: Cryptocurrency price inquiry" << std::endl;
    std::cout << "1: Get exchanges list" << std::endl;
    std::cout << "2: Buy cryptocurrency" << std::endl;
    std::cout << "3: Sell cryptocurrency" << std::endl;
    std::cout << "4: View wallet balance" << std::endl;
    std::cout << "5: Requesting a balance increase from the bank" << std::endl;
    std::cout << "6: View transaction history" << std::endl;
    std::cout << "7: CLS" << std::endl;
    std::cout << "8: Exit" << std::endl;
    std::cout << "Enter your choice : ";
}

void handleClient(int sockfd, int bankSocketFd, struct sockaddr_in addr, struct sockaddr_in bank_server_addr,
                  int assigned_port) {
    bool isRunning = true;
    std::string choice;
    while (isRunning) {
        renderMenu();
        std::getline(std::cin, choice);
        if (choice == "0") {
            priceInquiry();
        } else if (choice == "1") {
            getExchangesList(bankSocketFd, bank_server_addr);
        } else if (choice == "2") {
            buy_sellCryptocurrency(assigned_port, "BUY");
        } else if (choice == "3") {
            buy_sellCryptocurrency(assigned_port, "SELL");
        } else if (choice == "4") {
            viewWalletBalance(bankSocketFd, bank_server_addr, assigned_port);
        } else if (choice == "5") {
            increaseWalletBalance(bankSocketFd, bank_server_addr, assigned_port);
        } else if (choice == "6") {
            viewTransactionHistory(bankSocketFd, bank_server_addr, assigned_port);
        } else if (choice == "7") {
            system("clear");
        } else if (choice == "8") {
            isRunning = false;
        } else {
            std::cout << "Invalid choice!" << std::endl;
        }
    }
}

void registerWithBank(const std::string &name, const std::string &server_ip) {
    int sockfd;
    struct sockaddr_in addr{};
    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure the address structure with port 0 (system assigns a port)
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(0); // OS assigns an available port

    // Bind the socket
    if (bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Retrieve the assigned port
    socklen_t addr_len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr *) &addr, &addr_len) < 0) {
        perror("getsockname failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int assigned_port = ntohs(addr.sin_port);
    std::cout << "Listening on assigned port: " << assigned_port << std::endl;

    int bankSocketFd;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in bank_server_addr{};

    // Create a UDP socket
    if ((bankSocketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&bank_server_addr, 0, sizeof(bank_server_addr));
    bank_server_addr.sin_family = AF_INET;
    bank_server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip.c_str(), &bank_server_addr.sin_addr);

    // Set 2-second timeout for recvfrom
    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(bankSocketFd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
        close(bankSocketFd);
        exit(EXIT_FAILURE);
    }

    // Send registration message
    const std::string message = "REGISTER | " + name + " | " + std::to_string(assigned_port) + " | " + "CLIENT";
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    sendto(bankSocketFd, messageToServer.c_str(), messageToServer.size(), 0,
           (const struct sockaddr *) &bank_server_addr, sizeof(bank_server_addr));

    // Receive acknowledgment
    socklen_t len = sizeof(bank_server_addr);
    int n = recvfrom(bankSocketFd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &bank_server_addr, &len);
    if (n < 0) {
        perror("Receive failed");
        perror("Can not connect to bank");
        exit(EXIT_FAILURE);
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }
    handleClient(sockfd, bankSocketFd, addr, bank_server_addr, assigned_port);

    close(sockfd);
}
