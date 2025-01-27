#ifndef BANK_REGEX_H
#define BANK_REGEX_H
#include <iostream>
#include <regex>

const std::regex clientRegisterRegex(R"(^(REGISTER \| ([a-zA-Z0-9_]+) \| (\d{1,5}) \| CLIENT) \| TOKEN \| (.+)$)");
const std::regex exchangeRegisterRegex(R"(^(REGISTER \| ([a-zA-Z0-9_]+) \| (\d{1,5}) \| EXCHANGE) \| TOKEN \| (.+)$)");
const std::regex getAccountBalanceRegex(R"(^(GET_ACCOUNT_BALANCE \| (\d{1,5})) \| TOKEN \| (.+)$)");
const std::regex increaseAccountBalanceRegex(R"(^(INCREASE_ACCOUNT_BALANCE \| (\d{1,5}) \| (\d+)) \| TOKEN \| (.+)$)");
const std::regex getExchangeListRegex(R"(^(GET_EXCHANGE_LIST) \| TOKEN \| (.+)$)");
const std::regex getAccountHistoryRegex(R"(^(GET_ACCOUNT_HISTORY \| (\d{1,5})) \| TOKEN \| (.+)$)");
const std::regex addCryptocurrencyRegex(R"(^(CREATE_CRYPTO_CURRENCY \| ([a-zA-Z0-9_]+) \| (\d+) \| (\d+)) \| TOKEN \| (.+)$)");
const std::regex releaseCryptocurrencyRegex(R"(^(RELEASE_CRYPTO_CURRENCY \| ([a-zA-Z0-9_]+)) \| TOKEN \| (.+)$)");
const std::regex buyCryptocurrencyRegex(R"(^(BUY \| ([a-zA-Z0-9_]+) \| COUNT \| (\d+) \| PORT \| (\d{1,5}) \| PRICE \| (\d+)) \| TOKEN \| (.+)$)");
const std::regex sellCryptocurrencyRegex(R"(^(SELL \| ([a-zA-Z0-9_]+) \| COUNT \| (\d+) \| PORT \| (\d{1,5}) \| PRICE \| (\d+)) \| TOKEN \| (.+)$)");
const std::regex getExchangeListPortRegex(R"(^(GET_EXCHANGE_LIST_PORT) \| TOKEN \| (.+)$)");

#endif //BANK_REGEX_H
