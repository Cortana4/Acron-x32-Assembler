#pragma once
#include <string>
#include <vector>
#include <functional>

#include "sourceFileManager.h"
#include "objectCode.h"
#include "converter.h"

class Compiler
{
public:
	ObjectCode objectCode;

	Compiler();
	~Compiler();

	void reset();
	bool compileSource(std::string path);

private:
	SourceFileManager sourceFileManager;

	std::string line;
	std::vector<std::string> tokens;
	std::vector<std::pair<std::string, std::string>> defines;

	int errorCount;
	int warningCount;

	void error(std::string message);
	void warning(std::string message);

	void addInst_noOperands(uint8_t opcode, uint8_t func = 0x00);
	void addInst_dstA_imm(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toWord);
	void addInst_srcB_dstA(uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcB_addr(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
	void addInst_dstA_addr(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
	void addInst_srcB(uint8_t opcode, uint8_t func = 0x00);
	void addInst_dstA(uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB_dstA(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
	void addInst_srcA_srcB_dstA_dstB(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
	void addInst_srcA_dstA(uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB_dstA_RM(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toFloat);
	void addInst_srcA_dstA_RM(uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
	void addInst_addr(uint8_t opcode, uint8_t func = 0x00, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate = toInt);
};
