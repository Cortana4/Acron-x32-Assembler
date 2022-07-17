#pragma once
#include <vector>
#include <string>

void parseLine(std::string line, std::vector<std::string>& tokens);
bool parseAddress(std::string address, std::string& baseReg, std::string& offset);
bool contains(std::string str, const char c);
bool endsWith(std::string str, std::string end);
size_t find_first_of_outside_str(std::string str, std::string charsToFind);
