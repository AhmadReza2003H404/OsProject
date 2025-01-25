#ifndef BANK_REGEX_H
#define BANK_REGEX_H
#include <iostream>
#include <regex>

const std::regex clientRegisterRegex(R"(^(REGISTER \| ([a-zA-Z0-9_]+) \| (\d{1,5}) \| CLIENT) \| TOKEN \| (.+)$)");
const std::regex exchangeRegisterRegex(R"(^(REGISTER \| ([a-zA-Z0-9_]+) \| (\d{1,5}) \| CLIENT) \| TOKEN \| (.+)$)");

#endif //BANK_REGEX_H
