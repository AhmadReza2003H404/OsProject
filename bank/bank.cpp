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
#define MAX_BALANCE 5000

using namespace std;
const string SERVICE_IP = "127.0.0.1";

struct Cryptocurrencies {
    std::string name;
    long firstPrice;
    long countExist;
    bool isAvailable;
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
    vector<string> historyList;
};

struct Exchange {
    std::string name;
    int port;
};


std::vector<Client *> clients;
std::vector<Exchange *> exchanges;
vector<Cryptocurrencies *> cryptocurrencieses;
pthread_mutex_t mutex;

void *syncThread(void * args);

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

struct HandleMessageStruct {
    std::string message;
    int sockfd;
    struct sockaddr_in sockaddr_in;
};

void *handleMessage(void * arg);

// void handleMessage(const std::string &message, int sockfd, struct sockaddr_in sockaddr_in);

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

        pthread_t updaterThread;
        HandleMessageStruct* messageStruct = new HandleMessageStruct();
        messageStruct->message = message;
        messageStruct->sockfd = sockfd;
        messageStruct->sockaddr_in = client_addr;
        if (pthread_create(&updaterThread, nullptr, handleMessage, messageStruct) != 0) {
            std::cerr << "Error: Failed to create thread" << std::endl;
            std::string response = "Error: Failed to create thread for request";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &client_addr, len);
        }
    }

    close(sockfd);
}


int main() {
    pthread_t sync;
    if (pthread_create(&sync, nullptr, syncThread, nullptr) != 0) {
        std::cerr << "Error: Failed to create thread" << std::endl;
        return 1;
    }
    setUpBank();
}


void registerClient(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    std::string name = match[2];
    int port = std::stoi(match[3]);
    Client *client = new Client;
    client->name = name;
    client->port = port;
    client->balance = 0;
    // pthread_mutex_lock(&mutex);
    clients.push_back(client);
    //pthread_mutex_unlock(&mutex);
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

    //pthread_mutex_lock(&mutex);
    exchanges.push_back(exchange);
    const std::string response = "REGISTER SUCCESSFUL";
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
    std::cout << "New exchange registered: " << name << " " << port << std::endl;
}

bool isAuthorized(const std::smatch &match, int i) {
    std::string hash = match[i];
    std::string message = match[1];
    return simpleHash(message) == hash;
}

void getAccountBalance(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    std::string port = match[2];
    Client *c = nullptr;
    for (Client *client: clients) {
        if (client->port == std::stoi(port)) {
            c = client;
            break;
        }
    }
    if (c) {
        std::string response = "ACCOUNT BALANCE | " + std::to_string(c->balance);
        std::cout << "Get Account balance of : " << port << " is " << c->balance << std::endl;
        for(ClientCryptocurrency * CC : c->cryptocurrencies) {
            response += "\n Crypto: " + CC->name + " | Count: " + to_string(CC->balance);
        }
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    } else {
        std::cout << "Account not found " << port << std::endl;
        const std::string response = "ACCOUNT NOT FOUND";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void addCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    long count = stoi(match[3]);
    string curencyName = match[2];
    long firstPrice = stoi(match[4]);

    bool validate = true;
    for (auto curency: cryptocurrencieses) {
        if (curency->name == curencyName) {
            validate = false;
            break;
        }
    }

    if (validate) {
        const std::string response = "CRYPTO CREATE | " + curencyName;
        Cryptocurrencies *cr = new Cryptocurrencies();
        cr->name = curencyName;
        cr->firstPrice = firstPrice;
        cr->countExist = count;
        cr->isAvailable = false;
        cryptocurrencieses.push_back(cr);
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
        std::cout << "Crypto Create : " << curencyName << std::endl;
    } else {
        std::cout << "This Crypto Exist Already. " << std::endl;
        const std::string response = "Crypto Exist Already. Try with another name";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void addAccountBalance(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    std::string port = match[2];
    std::string increaseAmount = match[3];
    Client *c = nullptr;
    for (Client *client: clients) {
        if (client->port == std::stoi(port)) {
            c = client;
            break;
        }
    }
    if (c) {
        std::string response;
        if (c->balance + std::stoi(increaseAmount) < MAX_BALANCE) {
            c->balance += std::stoi(increaseAmount);
            std::cout << "Increase balance of " << port << " is " << increaseAmount << std::endl;
            response = "AMOUNT INCREASED";
            c->historyList.push_back("AddAcountBalance Transaction: " + increaseAmount);
        } else {
            std::cout << "Can't Increase balance of " << port << std::endl;
            response = "YOU REACH TO TOP OF MAX BALANCE";
        }
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    } else {
        std::cout << "Account not found " << port << std::endl;
        const std::string response = "ACCOUNT NOT FOUND";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void getExchangeList(int sockfd, struct sockaddr_in sockaddr_in) {
    std::string response = "ExchangeList \n size: " + to_string(exchanges.size());
    for (auto ex: exchanges) {
        std::string message = "\nName: " + ex->name + " Port: " + to_string(ex->port);
        response = response + message;
    }


    socklen_t len = sizeof(sockaddr_in);
    std::cout << "Send Exchange List" << endl;
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
}

// void getExchangeListPort(int sockfd, struct sockaddr_in sockaddr_in){
//     std::string response = "";
//     for (auto ex: exchanges) {
//         if(response.size() > 0) response = response + ',';
//         std::string message = to_string(ex->port);
//         response = response + message;
//     }


//     socklen_t len = sizeof(sockaddr_in);
//     std::cout << "Send Exchange List" << endl;
//     sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
// }

void getExchangeListPort(int sockfd, struct sockaddr_in sockaddr_in) {
    std::string response = "";
    for (auto ex: exchanges) {
        if(response.size() > 0) response = response + ',';
        std::string message = to_string(ex->port);
        response = response + message;
    }


    socklen_t len = sizeof(sockaddr_in);
    std::cout << "Send Exchange List" << endl;
    sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in, sizeof(sockaddr_in));
}

void getAccountHistory(int sockfd, struct sockaddr_in sockaddr_in, const std::smatch &match) {
    std::string port = match[2];
    Client *c = nullptr;
    for (Client *client: clients) {
        if (client->port == std::stoi(port)) {
            c = client;
            break;
        }
    }
    if (c) {
        std::string response = "HistoryList \n size: " + to_string(c->historyList.size());
        for (auto history: c->historyList) {
            std::string message = "\nName: " + history;
            response = response + message;
        }

        socklen_t len = sizeof(sockaddr_in);
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));

        std::cout << "Send History List" << endl;
    }
}


void releaseCryptocurrency(const smatch &match) {
    for (Cryptocurrencies *c: cryptocurrencieses) {
        if (match[2] == c->name) {
            pthread_mutex_unlock(&mutex);
            c->isAvailable = true;
            pthread_mutex_unlock(&mutex);
            cout << "Cryptocurrency " << c->name << " is released" << endl;
            break;;
        }
    }
}

void buyCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const smatch & match) {
    std::string cryptoName = match[2];
    int count = stoi(match[3]);
    int port = stoi(match[4]);
    int price = stoi(match[5]);
    Client *c = nullptr;
    for (Client *client: clients) {
        if (client->port == port) {
            c = client;
            break;
        }
    }
    if (c) {
        std::string response;
        for(Cryptocurrencies * cryptocurrencies : cryptocurrencieses) {
            if(cryptocurrencies->name == cryptoName) {
                response = "AVAILABLE";
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        if(c->balance >= price*count && response == "AVAILABLE") {
            ClientCryptocurrency * c2 = nullptr;
            for (ClientCryptocurrency *client_cryptocurrency: c->cryptocurrencies) {
                if(client_cryptocurrency->name == cryptoName) {
                    c2 = client_cryptocurrency;
                    break;
                }
            }
            if(c2) {
                c2->balance += count;
            } else {
                c2 = new ClientCryptocurrency;
                c2->name = cryptoName;
                c2->balance = count;
                c->cryptocurrencies.push_back(c2);
            }
            c->balance -= count*price;
            std::string message = "BUY CRYPTO | Buy " + to_string(count) + " of " + cryptoName + " with price " +  to_string(price);
            c->historyList.push_back(message);
            response = "SUCCESSES";
        } else {
            response = "LOW BALANCE | this account does not have enough balance";
        }
        pthread_mutex_unlock(&mutex);
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
       sizeof(sockaddr_in));
    } else {
        std::cout << "Account not found " << port << std::endl;
        const std::string response = "ACCOUNT NOT FOUND";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void sellCryptocurrency(int sockfd, struct sockaddr_in sockaddr_in, const smatch & match) {
     std::string cryptoName = match[2];
    int count = stoi(match[3]);
    int port = stoi(match[4]);
    int price = stoi(match[5]);
    Client *c = nullptr;
    for (Client *client: clients) {
        if (client->port == port) {
            c = client;
            break;
        }
    }
    if (c) {
        std::string response;
        for(Cryptocurrencies * cryptocurrencies : cryptocurrencieses) {
            if(cryptocurrencies->name == cryptoName) {
                if(cryptocurrencies->isAvailable) {
                    response = "AVAILABLE";
                } else {
                    response = "NOT AVAILABLE";
                }
                break;
            }
        }
        pthread_mutex_lock(&mutex);
        if(response == "AVAILABLE") {
            ClientCryptocurrency * c2 = nullptr;
            for (ClientCryptocurrency *client_cryptocurrency: c->cryptocurrencies) {
                if(client_cryptocurrency->name == cryptoName) {
                    c2 = client_cryptocurrency;
                    break;
                }
            }
            if(c2) {
                if(c2->balance >= count) {
                    c2->balance -= count;
                    c->balance += count*price;
                    std::string message = "SELL CRYPTO | Sell " + to_string(count) + " of " + cryptoName + " with price " +  to_string(price);
                    c->historyList.push_back(message);
                    response = "SUCCESSES";
                } else {
                    response = "NOT ENOUGH | This user not have enough count of this crypto";
                }
            } else {
                response = "NOT HAVE | This user not have this crypto";
            }
        } else {
            response = "NOT AVAILABLE | This crypto is not available for sell";
        }
        pthread_mutex_unlock(&mutex);
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
       sizeof(sockaddr_in));
    } else {
        std::cout << "Account not found " << port << std::endl;
        const std::string response = "ACCOUNT NOT FOUND";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
}

void *handleMessage(void *arg) {
    HandleMessageStruct * handle_message_struct = static_cast<HandleMessageStruct *>(arg);
    std::string message = handle_message_struct->message;
    int sockfd = handle_message_struct->sockfd;
    struct sockaddr_in sockaddr_in = handle_message_struct->sockaddr_in;
    std::smatch match; // Object to hold the match results
    std::cout << "Receive: " << message << std::endl;
    if (std::regex_match(message, match, clientRegisterRegex)) {
        if (isAuthorized(match, 4)) {
            registerClient(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, exchangeRegisterRegex)) {
        if (isAuthorized(match, 4)) {
            registerExchange(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, getAccountBalanceRegex)) {
        if (isAuthorized(match, 3)) {
            getAccountBalance(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, increaseAccountBalanceRegex)) {
        if (isAuthorized(match, 4)) {
            addAccountBalance(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, getExchangeListRegex)) {
        if (isAuthorized(match, 2)) {
            getExchangeList(sockfd, sockaddr_in);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, getAccountHistoryRegex)) {
        if (isAuthorized(match, 3)) {
            getAccountHistory(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, addCryptocurrencyRegex)) {
        if (isAuthorized(match, 5)) {
            addCryptocurrency(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, releaseCryptocurrencyRegex)) {
        if (isAuthorized(match, 3)) {
            releaseCryptocurrency(match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, buyCryptocurrencyRegex)) {
        if(isAuthorized(match, 6)) {
            buyCryptocurrency(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if (std::regex_match(message, match, sellCryptocurrencyRegex)) {
        if(isAuthorized(match, 6)) {
            sellCryptocurrency(sockfd, sockaddr_in, match);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    } else if(std::regex_match(message , match , getExchangeListPortRegex)){
        if (isAuthorized(match, 2)) {
            getExchangeListPort(sockfd, sockaddr_in);
        } else {
            const std::string response = "NOT AUTHORIZED";
            sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
                   sizeof(sockaddr_in));
        }
    }
    
    else {
        std::cout << "Request not found: " << message << std::endl;
        const std::string response = "Request not found";
        sendto(sockfd, response.c_str(), response.size(), 0, (const struct sockaddr *) &sockaddr_in,
               sizeof(sockaddr_in));
    }
    delete handle_message_struct;
    return nullptr;
}

void sendMessageToExchange(Exchange * exchange, const string & message) {
    int sockfd;
    struct sockaddr_in exchange_server_addr{};

    // Create a UDP socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
    }

    // Configure server address
    memset(&exchange_server_addr, 0, sizeof(exchange_server_addr));
    exchange_server_addr.sin_family = AF_INET;
    exchange_server_addr.sin_port = htons(exchange->port);
    inet_pton(AF_INET, SERVICE_IP.c_str(), &exchange_server_addr.sin_addr);
    sendto(sockfd, message.c_str(), message.size(), 0, (const struct sockaddr *) &exchange_server_addr,
       sizeof(exchange_server_addr));
}

void *syncThread(void *args) {
    while (true) {
        sleep(30); // Sleep for 2 minute
        std::cout << "Sync thread started" << std::endl;
        for(Cryptocurrencies* cryptocurrencies : cryptocurrencieses ) {
            if(cryptocurrencies->isAvailable) {
                int totalSold = 0;
                for(Client *client : clients) {
                    for(ClientCryptocurrency * c: client->cryptocurrencies) {
                        if(c->name == cryptocurrencies->name) {
                            totalSold += c->balance;
                        }
                    }
                }
                int newPrice = cryptocurrencies->firstPrice + ((totalSold*100)/cryptocurrencies->countExist)*3*cryptocurrencies->firstPrice/5;
                const std::string message = "SYNC | " + cryptocurrencies->name + " | NEW_PRICE | " + std::to_string(newPrice);
                std::cout << message << std::endl;
                const std::string messageToServer = message + " | TOKEN | " + simpleHash(message);
                for(Exchange * exchange: exchanges) {
                    sendMessageToExchange(exchange, messageToServer);
                }
            }
        }
    }
    return nullptr;
}

