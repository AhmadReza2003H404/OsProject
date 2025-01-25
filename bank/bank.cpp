#include <iostream>
#include <cstring>
#include <cstdlib>
#include <regex>
#include <unistd.h>
#include <arpa/inet.h>
#include <vector>
#include <string>
#include "back_regex.h"
#include "HasFunction.h"

#define PORT 9999
#define BUFFER_SIZE 1024

struct Cryptocurrencies {
    std::string name;
};

struct ClientCryptocurrency {
    std::string name;
    long balance;
};

struct Client {
    std::string name;
    int port;
    long balance;
    std::vector<ClientCryptocurrency *> cryptocurrencies;
};

struct Exchange {
    std::string name;
    int port;
};


std::vector<Client *> clients;
std::vector<Exchange *> exchanges;
pthread_mutex_t mutex;

std::string trim(const std::string &str) {
    // Find the first non-whitespace character
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return ""; // String is all whitespace
    }

    // Find the last non-whitespace character
    size_t end = str.find_last_not_of(" \t\n\r");

    // Return the trimmed string
    return str.substr(start, end - start + 1);
}

void handleMessage(const std::string &message, int sockfd, struct sockaddr_in sockaddr_in);

[[noreturn]] void setUpBank() {
    std::cout << "Entering bank setup...\n";
    pthread_mutex_init(&mutex, NULL);

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
    if (bind(sockfd, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Bank listening on port " << PORT << " (UDP)\n";

    while (true) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';

        std::string message(buffer);

        handleMessage(message, sockfd, client_addr);
    }

    close(sockfd);
}

int main() {
    setUpBank();
}


void registerClient(int sockfd, struct sockaddr_in sockaddr_in,  const std::smatch &match) {
    std::string name = match[2];
    int port = std::stoi(match[3]);
    Client *client = new Client;
    client->name = name;
    client->port = port;
    client->balance = 0;
    pthread_mutex_lock(&mutex);
    clients.push_back(client);
    pthread_mutex_unlock(&mutex);
    const std::string response = "REGISTER SUCCESSFUL";
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
    std::cout << "New client registered: " << name << " " << port << std::endl;
}

void registerExchange(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    std::string name = match[2];
    int port = std::stoi(match[3]);
    Exchange *exchange = new Exchange;
    exchange->name = name;
    exchange->port = port;
    pthread_mutex_lock(&mutex);
    exchanges.push_back(exchange);
    pthread_mutex_unlock(&mutex);
    const std::string response = "REGISTER SUCCESSFUL";
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
    std::cout << "New exchange registered: " << name << " " << port << std::endl;
}

bool isAuthorized(const std::smatch & match, int i) {
    std::string hash = match[i];
    std::string message = match[1];
    return simpleHash(message) == hash;
}

void handleMessage(const std::string &message, int sockfd, struct sockaddr_in sockaddr_in) {
    std::smatch match; // Object to hold the match results

    if (std::regex_match(message, match, clientRegisterRegex)) {
        if(isAuthorized(match, 4)) {
            registerClient(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, exchangeRegisterRegex)) {
        registerExchange(sockfd, sockaddr_in, match);
    } else {
        std::cout << "Name not found: " << message << std::endl;
    }
}
