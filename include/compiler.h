#pragma once
#include <string>
#include <vector>

#include "sourceFileManager.h"
#include "objectCode.h"

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

	int errorCount;
	int warningCount;

	void error(std::string message);
	void warning(std::string message);

	void addInst_noOperands(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_dst_imm(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcB_dst(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcB_addr(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_dst_addr(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcB(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_dst(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB_dst(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_dst(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB_dst_RM(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_dst_RM(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_srcA_srcB(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
	void addInst_addr(std::vector<std::string>& tokens, uint8_t opcode, uint8_t func = 0x00);
};
