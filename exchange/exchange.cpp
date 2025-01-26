#include <atomic>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctime>
#include <vector>
#include<mutex>
#include <regex>

#include "exchange_regex.h"
#include "HasFunction.h"

#define SERVER_PORT 9999
#define BUFFER_SIZE 4096


using namespace std;

const string SERVICE_IP = "127.0.0.1";

std::mutex mtx;

struct Cryptocurrency {
    string name;
    bool isAvailable;
    int initPrice;
    int totalCounts;
    int count;
    int price;
    std::time_t createTime;
    int realizeAfterSeconds;
    bool isOwner;
};


vector<Cryptocurrency *> currencyList;

struct ProviderThreadStruct {
    atomic<long> * exchangeBalance;
    int socket;
    int port;
    struct sockaddr_in address;
};

bool isAuthorized(const std::smatch &match, int i) {
    std::string hash = match[i];
    std::string message = match[1];
    return simpleHash(message) == hash;
}


void inquiryCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const smatch &match) {
    Cryptocurrency *cryptocurrency = nullptr;
    std::string name = match[2];
    for (auto *currency: currencyList) {
        if (currency->name == name) {
            cryptocurrency = currency;
            break;
        }
    }
    std::string response;
    if (cryptocurrency) {
        response = "Cryptocurrency: " + cryptocurrency->name + " price is: " + std::to_string(cryptocurrency->price);
    } else {
        response = "NOT FOUND | Cryptocurrency " + name + " is not found in this exchange";
    }
    std::cout << response << std::endl;
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
           sizeof(sockaddr_in));
}

std::string sendMessageToBank(std::string message) {
    int sockfd;
    struct sockaddr_in bank_server_addr{};

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        return "Socket creation failed";
    }

    // Configure server address
    memset(&bank_server_addr, 0, sizeof(bank_server_addr));
    bank_server_addr.sin_family = AF_INET;
    bank_server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVICE_IP.c_str(), &bank_server_addr.sin_addr);

    timeval timeout;
    timeout.tv_sec = 2;
    timeout.tv_usec = 0;
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Failed to set socket timeout");
        close(sockfd);
        return "Failed to set socket timeout";
    }
    sendto(sockfd, message.c_str(), message.size(), 0, (const struct sockaddr *) &bank_server_addr,
       sizeof(bank_server_addr));
    char buffer[BUFFER_SIZE];
    socklen_t len = sizeof(bank_server_addr);
    int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &bank_server_addr, &len);
    if (n < 0) {
        perror("Receive failed or timeout occurred");
        return "Receive failed or timeout occurred";
    }
    buffer[n] = '\0';
    return string(buffer);
}

std::string submitBuyRequestInBank(const string & name, Cryptocurrency * cryptocurrency, int port, long count, atomic<long> * exchangeBalance) {
    int price = cryptocurrency->price;
    const std::string message = "BUY | " + name + " | COUNT | " + to_string(count)  + " | PORT | " + to_string(port) + " | PRICE | " + to_string(price);
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    std:: string response = sendMessageToBank(messageToServer);
    if(response == "SUCCESSES") {
        mtx.lock();
        cryptocurrency->count -= count;
        mtx.unlock();
        exchangeBalance->store( exchangeBalance->load() - (count * price));
    }
    return response;
}

void buy_sellCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const smatch & match, atomic<long> * exchangeBalance) {
    std::string name = match[2];
    long count = std::stoi(match[3]);
    int port = std::stoi(match[4]);
    Cryptocurrency  * cryptocurrency = nullptr;
    for (auto *currency: currencyList) {
        if(currency->name == name) {
            cryptocurrency = currency;
            break;
        }
    }
    std::string response;
    if (cryptocurrency) {
        if(cryptocurrency->count >= count) {
            if(cryptocurrency->isAvailable) {
                response = submitBuyRequestInBank(name, cryptocurrency , port, count, exchangeBalance);
            } else {
                response = "NOT AVAILABLE | This Cryptocurrency: " + name + " is not available";
            }
        } else {
            response = "NOT ENOUGH | This exchange does not have this count of " +  name;
        }
    } else {
        response = "NOT FOUND | Cryptocurrency " + name + " is not found in this exchange";
    }
    std::cout << response << std::endl;
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
           sizeof(sockaddr_in));
}

void sellCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const smatch & match) {

}

void handleMessage(const std::string &message, int sockfd, struct sockaddr_in sockaddr_in, atomic<long> * exchangeBalance) {
    std::smatch match; // Object to hold the match results
    std::cout << "Receive: " << message << std::endl;
    if (std::regex_match(message, match, inquiryCryptocurrencyRegex)) {
        if (isAuthorized(match, 3)) {
            inquiryCryptocurrency(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if(std::regex_match(message, match, buyCryptocurrencyRegex)) {
        if(isAuthorized(match, 5)) {
            buy_sellCryptocurrency(sockfd, sockaddr_in, match, exchangeBalance);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if(std::regex_match(message, match, sellCryptocurrencyRegex)) {
        if(isAuthorized(match, 5)) {
            sellCryptocurrency(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else {
        std::cout << "Request not found: " << message << std::endl;
        const std::string response = "Request not found";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void *provider(void *arg) {
    auto *provider = static_cast<ProviderThreadStruct *>(arg);
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr{};

    while (true) {
        socklen_t len = sizeof(client_addr);
        int n = recvfrom(provider->socket, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &len);
        if (n < 0) {
            perror("Receive failed");
            continue;
        }

        buffer[n] = '\0';

        std::string message(buffer);

        handleMessage(message, provider->socket, client_addr, provider->exchangeBalance);
    }
}

void *availabilityUpdater(void *arg) {
    int bankSocketFd;
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
    inet_pton(AF_INET, SERVICE_IP.c_str(), &bank_server_addr.sin_addr);
    vector<Cryptocurrency *> *list = static_cast<vector<Cryptocurrency *> *>(arg);
    while (true) {
        sleep(5); // Sleep for 1 second
        {
            cout << "Starting thread to check release currency" << endl;
            mtx.lock();

            for (auto currency: *list) {
                auto elapsedTime = std::difftime(std::time(nullptr), currency->createTime);
                if (elapsedTime >= currency->realizeAfterSeconds) {
                    if (!currency->isAvailable) {
                        currency->isAvailable = true;
                        const string message = "RELEASE_CRYPTO_CURRENCY | " + currency->name;
                        const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
                        sendto(bankSocketFd, messageToServer.c_str(), messageToServer.size(), 0,
                               (const struct sockaddr *) &bank_server_addr, sizeof(bank_server_addr));
                        cout << "Current currency: " << currency->name << " released" << endl;
                    }
                }
            }
            mtx.unlock();
        }
    }
    return nullptr;
}


void addCrypto(int sockfd, struct sockaddr_in sockaddr_in) {
    string cryptoName;
    string cryptoPrice;
    string cryptoCount;
    string realizeSec;
    cout << "Enter crypto name : " << endl;
    getline(std::cin, cryptoName);
    cout << "Enter crypto price : " << endl;
    getline(std::cin, cryptoPrice);
    cout << "Enter crypto count : " << endl;
    getline(std::cin, cryptoCount);
    cout << "Enter crypto realizeSec : " << endl;
    getline(std::cin, realizeSec);
    try {
        stoi(cryptoPrice);
        stoi(cryptoCount);
        stoi(realizeSec);
        const std::string message = "CREATE_CRYPTO_CURRENCY | " + cryptoName + " | " + cryptoCount + " | " +
                                    cryptoPrice;
        const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
        sendto(sockfd, messageToServer.c_str(), messageToServer.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));

        // SERVER RESPONSE
        char buffer[BUFFER_SIZE];
        socklen_t len = sizeof(sockaddr_in);
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &sockaddr_in, &len);
        if (n < 0) {
            perror("Receive failed");
        } else {
            buffer[n] = '\0';
            std::cout << "Bank response: " << buffer << "\n";
            if (string(buffer) != "Crypto Exist Already. Try with another name") {
                Cryptocurrency *cryptocurrency = new Cryptocurrency;
                cryptocurrency->name = cryptoName;
                cryptocurrency->isAvailable = false;
                cryptocurrency->count = stoi(cryptoCount);
                cryptocurrency->createTime = std::time(nullptr);
                cryptocurrency->realizeAfterSeconds = stoi(realizeSec);
                cryptocurrency->price = stoi(cryptoPrice);
                cryptocurrency->initPrice = stoi(cryptoPrice);
                cryptocurrency->isOwner = true;
                cryptocurrency->totalCounts = stoi(cryptoCount);
                mtx.lock();
                currencyList.push_back(cryptocurrency);
                mtx.unlock();
            }
        }
    } catch (...) {
    }
}

void renderMenu() {
    std::cout << "0: Add Crypto" << std::endl;
    std::cout << "1: Command2" << std::endl;
    std::cout << "2: Command3" << std::endl;
    std::cout << "3: Command4" << std::endl;
    std::cout << "4: Command5" << std::endl;
    std::cout << "5: Command6" << std::endl;
    std::cout << "6: Command7" << std::endl;
    std::cout << "7: Exit" << std::endl;
    std::cout << "Enter your choice : ";
}

void handleExchange(int sockfd, int bankSocketFd, struct sockaddr_in addr, struct sockaddr_in bank_server_addr, atomic<long> * exchangeBalance) {
    bool isRunning = true;
    std::string choice;
    while (isRunning) {
        renderMenu();
        std::getline(std::cin, choice);

        if (choice == "0") {
            addCrypto(bankSocketFd, bank_server_addr);
        } else if (choice == "1") {
            //function Call
        } else if (choice == "2") {
            //function Call
        } else if (choice == "3") {
            //function Call
        } else if (choice == "4") {
            //function Call
        } else if (choice == "5") {
            //function Call
        } else if (choice == "6") {
            //function Call
        } else if (choice == "7") {
            isRunning = false;
        } else {
            std::cout << "Invalid choice!" << std::endl;
        }
    }
}

void registerWithBank(const std::string &name, const std::string &server_ip) {
    atomic<long> exchangeBalance;
    exchangeBalance.store(10000000);
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

    // Send registration message
    const std::string message = "REGISTER | " + name + " | " + std::to_string(assigned_port) + " | " + "EXCHANGE";
    const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
    sendto(bankSocketFd, messageToServer.c_str(), messageToServer.size(), 0,
           (const struct sockaddr *) &bank_server_addr, sizeof(bank_server_addr));

    // Receive acknowledgment
    socklen_t len = sizeof(bank_server_addr);
    int n = recvfrom(bankSocketFd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &bank_server_addr, &len);
    if (n < 0) {
        perror("Receive failed");
    } else {
        buffer[n] = '\0';
        std::cout << "Bank response: " << buffer << "\n";
    }
    ProviderThreadStruct *providerThreadStruct = new ProviderThreadStruct();
    providerThreadStruct->socket = sockfd;
    providerThreadStruct->port = assigned_port;
    providerThreadStruct->address = addr;
    providerThreadStruct->exchangeBalance = &exchangeBalance;

    pthread_t readerThread;
    if (pthread_create(&readerThread, nullptr, provider, providerThreadStruct) != 0) {
        std::cerr << "Error: Failed to create thread" << std::endl;
        exit(EXIT_FAILURE);
    }
    handleExchange(sockfd, bankSocketFd, addr, bank_server_addr, &exchangeBalance);
}


int main() {
    std::string name;
    std::cout << "Enter your exchange name: ";
    std::getline(std::cin, name);
    pthread_t updaterThread;
    if (pthread_create(&updaterThread, nullptr, availabilityUpdater, &currencyList) != 0) {
        std::cerr << "Error: Failed to create thread" << std::endl;
        return 1;
    }
    registerWithBank(name, SERVICE_IP);

    return 0;
}
