#ifndef HASFUNCTION_H
#define HASFUNCTION_H

#include <string>

const std::string SECRET = "MySecretKey123";

inline std::string simpleHash(const std::string& message) {
    const size_t length = 25; // Fixed output length
    unsigned long hash = 5381; // Initialize hash value
    std::string input = message + SECRET; // Combine message with constant secret

    for (char c : input) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }

    // Convert hash to string
    std::string hashStr = std::to_string(hash);

    // Ensure the hash string has the desired length
    if (hashStr.size() > length) {
        return hashStr.substr(0, length);
    }

    // Pad with zeros if shorter than desired length
    while (hashStr.size() < length) {
        hashStr += '0';
    }

    return hashStr;
}

#endif //HASFUNCTION_H
