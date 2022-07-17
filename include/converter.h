#pragma once
#include <string>
#include <vector>
#include <functional>

void removeQuotes(std::string& str);

bool isDec(std::string str);
bool isHex(std::string str);
bool isBin(std::string str);
bool isInt(std::string str);
bool isFloat(std::string str);
bool isChar(std::string str);
bool isString(std::string str);
bool isRegister(std::string str);

uint32_t toInt(std::string str, std::function<void(std::string)> errorFunc = nullptr);
uint32_t toFloat(std::string str, std::function<void(std::string)> errorFunc = nullptr);
uint32_t toChar(std::string str, std::function<void(std::string)> errorFunc = nullptr);
std::vector<uint32_t> toString(std::string str, std::function<void(std::string)> errorFunc = nullptr);
uint32_t toWord(std::string str, std::function<void(std::string)> errorFunc = nullptr);
std::vector<uint32_t> toWordArray(std::string str, std::function<void(std::string)> errorFunc = nullptr);
uint8_t toRegister(std::string str, std::function<void(std::string)> errorFunc = nullptr);
uint8_t toRoundingMode(std::string str, std::function<void(std::string)> errorFunc = nullptr);

uint32_t getMachineCode(uint8_t opcode, bool fetchImmediate, uint8_t func, uint8_t srcA, uint8_t srcB, uint8_t dst);
