#ifndef EXCHANGE_REGEX_H
#define EXCHANGE_REGEX_H
#include <iostream>
#include <regex>

const std::regex inquiryCryptocurrencyRegex(R"(^(INQUIRY \| ([a-zA-Z0-9_]+)) \| TOKEN \| (.+)$)");
const std::regex sellCryptocurrencyRegex(R"(^(SELL \| ([a-zA-Z0-9_]+) \| COUNT \| (\d+) \| PORT \| (\d{1,5})) \| TOKEN \| (.+)$)");
const std::regex buyCryptocurrencyRegex(R"(^(BUY \| ([a-zA-Z0-9_]+) \| COUNT \| (\d+) \| PORT \| (\d{1,5})) \| TOKEN \| (.+)$)");
const std::regex syncCryptocurrencyRegex(R"(^(SYNC \| ([a-zA-Z0-9_]+) \| NEW_PRICE \| (\d+)) \| TOKEN \| (.+)$)");

const std::regex buyCryptocurrencyExchangeRegex(R"(^(BUY_CRYPTO_EXCHANGE \| ([a-zA-Z0-9_]+)  \| (\d+) \| (\d+)) \| TOKEN \| (.+)$)");




#endif //EXCHANGE_REGEX_H
