#include "compiler.h"
#include "constants.h"
#include "parser.h"
#include "converter.h"

#include <iostream>
#include <utility>

Compiler::Compiler() : errorCount{ 0 }, warningCount{ 0 }
{

}

Compiler::~Compiler()
{
	reset();
}

void Compiler::reset()
{

	sourceFileManager.closeAll();
	objectCode.clear();
	line.clear();
	tokens.clear();
	defines.clear();
	errorCount = 0;
	warningCount = 0;
}

bool Compiler::compileSource(std::string path)
{
	reset();

	// try to open source file
	if (!sourceFileManager.addFile(path))
	{
		std::cout << "Fatal: cannot open source file '" << path << "'!" << std::endl;
		sourceFileManager.closeAll();
		return false;
	}

	// iterate over every line in all source files
	while (sourceFileManager.getLine(line))
	{
		// replace defines
		for (std::pair<std::string, std::string>& define : defines)
		{
			size_t pos = line.find(define.first);

			while (pos != std::string::npos)
			{
				line.replace(pos, define.first.length(), define.second);
				pos = line.find(define.first, pos + define.second.length());
			}
		}

		// parse tokens from line
		parseLine(line, tokens);

		// skip empty lines
		if (tokens.empty())
			continue;

		// label
		if (tokens.at(0).back() == ':')
		{
			tokens.at(0).pop_back();
			objectCode.addDereference(tokens.at(0));
			tokens.erase(tokens.begin());
			if (tokens.empty())
				continue;
		}
		// directives
		if (tokens.at(0) == ".inc" || tokens.at(0) == ".INC")
		{
			if (tokens.size() != 2)
				error("invalid number of operands to " + tokens.at(0) + " directive.");
			if (!sourceFileManager.addFile(tokens.at(1)))
				error("cannot open source file '" + tokens.at(1) + "'.");
		}
		else if (tokens.at(0) == ".org" || tokens.at(0) == ".ORG")
		{
			if (tokens.size() != 2)
			{
				error("invalid number of operands to " + tokens.at(0) + " directive.");
				continue;
			}

			std::string baseReg;
			std::string offset;

			if (!parseAddress(tokens.at(1), baseReg, offset))
			{
				error("invalid address '" + tokens.at(1) + "'.");
				continue;
			}
			if (!baseReg.empty() || offset.empty())
			{
				error(tokens.at(0) + " directive only supports direct addressing.");
				continue;
			}
			// if .org is used before any instruction, it overwrites the base pointer
			if (objectCode.empty())
				basePtr = toInt(offset, std::bind(&Compiler::error, this, std::placeholders::_1));
			else
			{
				int32_t n = toInt(offset, std::bind(&Compiler::error, this, std::placeholders::_1)) - basePtr;
				if (n < static_cast<int32_t>(objectCode.size()))
					error("overwriting existing object code.");
				else
					objectCode.resize(n, 0);
			}
		}
		else if (tokens.at(0) == ".def" || tokens.at(0) == ".DEF")
		{
			// todo: auf redefines prüfen 
			if (tokens.size() != 3)
				error("invalid number of operands to " + tokens.at(0) + " directive.");
			else
			{
				// redefinition can never happen, because define replaces the identifier for all following defines to the same identifier
				defines.push_back(std::make_pair(tokens.at(1), tokens.at(2)));
			}
		}
		else if (tokens.at(0) == ".dw" || tokens.at(0) == ".DW")
		{
			if (tokens.size() <= 1)
			{
				error("invalid number of operands to " + tokens.at(0) + " directive.");
				continue;
			}

			std::vector<uint32_t> vecA, vecB;

			for (size_t i = 1; i < tokens.size(); i++)
			{
				vecA = toWordArray(tokens.at(i), std::bind(&Compiler::error, this, std::placeholders::_1));
				vecB.insert(vecB.end(), vecA.begin(), vecA.end());
			}

			objectCode.append(vecB);
		}
		// instructions
		else if (tokens.at(0) == "nop" || tokens.at(0) == "NOP")
			addInst_noOperands(static_cast<uint8_t>(INST::NOP));

		else if (tokens.at(0) == "inr" || tokens.at(0) == "INR")
			addInst_dstA_imm(static_cast<uint8_t>(INST::INR), 0, toWord);

		else if (tokens.at(0) == "mov" || tokens.at(0) == "MOV")
			addInst_srcB_dstA(static_cast<uint8_t>(INST::MOV));

		else if (tokens.at(0) == "stm" || tokens.at(0) == "STM")
			addInst_srcB_addr(static_cast<uint8_t>(INST::STM));

		else if (tokens.at(0) == "ldm" || tokens.at(0) == "LDM")
			addInst_dstA_addr(static_cast<uint8_t>(INST::LDM));

		else if (tokens.at(0) == "push" || tokens.at(0) == "PUSH")
			addInst_srcB(static_cast<uint8_t>(INST::PUSH));

		else if (tokens.at(0) == "pop" || tokens.at(0) == "POP")
			addInst_dstA(static_cast<uint8_t>(INST::POP));

		else if (tokens.at(0) == "add" || tokens.at(0) == "ADD")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::ADD));

		else if (tokens.at(0) == "adc" || tokens.at(0) == "ADC")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::ADC));

		else if (tokens.at(0) == "sub" || tokens.at(0) == "SUB")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::SUB));

		else if (tokens.at(0) == "sbc" || tokens.at(0) == "SBC")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::SBC));
// todo: change mul instructions to 64 bit res dstA and dstB
		else if (tokens.at(0) == "umull" || tokens.at(0) == "UMULL")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::MUL), static_cast<uint8_t>(MUL_FUNC::UMULL));

		else if (tokens.at(0) == "umulh" || tokens.at(0) == "UMULH")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::MUL), static_cast<uint8_t>(MUL_FUNC::UMULH));

		else if (tokens.at(0) == "smull" || tokens.at(0) == "SMULL")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::MUL), static_cast<uint8_t>(MUL_FUNC::SMULL));

		else if (tokens.at(0) == "smulh" || tokens.at(0) == "SMULH")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::MUL), static_cast<uint8_t>(MUL_FUNC::SMULH));

		else if (tokens.at(0) == "udiv" || tokens.at(0) == "UDIV")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::DIV), static_cast<uint8_t>(DIV_FUNC::UDIV));

		else if (tokens.at(0) == "sdiv" || tokens.at(0) == "SDIV")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::DIV), static_cast<uint8_t>(DIV_FUNC::SDIV));

		else if (tokens.at(0) == "umod" || tokens.at(0) == "UMOD")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::DIV), static_cast<uint8_t>(DIV_FUNC::UMOD));

		else if (tokens.at(0) == "smod" || tokens.at(0) == "SMOD")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::DIV), static_cast<uint8_t>(DIV_FUNC::SMOD));

		else if (tokens.at(0) == "inc" || tokens.at(0) == "INC")
			addInst_srcA_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::INC));

		else if (tokens.at(0) == "dec" || tokens.at(0) == "DEC")
			addInst_srcA_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::DEC));

		else if (tokens.at(0) == "neg" || tokens.at(0) == "NEG")
			addInst_srcA_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::NEG));

		else if (tokens.at(0) == "and" || tokens.at(0) == "AND")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::AND));

		else if (tokens.at(0) == "or" || tokens.at(0) == "OR")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::OR));

		else if (tokens.at(0) == "xor" || tokens.at(0) == "XOR")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::XOR));

		else if (tokens.at(0) == "not" || tokens.at(0) == "NOT")
			addInst_srcA_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::NOT));

		else if (tokens.at(0) == "lsl" || tokens.at(0) == "LSL")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::LSL));

		else if (tokens.at(0) == "lsr" || tokens.at(0) == "LSR")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::LSR));

		else if (tokens.at(0) == "asr" || tokens.at(0) == "ASR")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::ASR));

		else if (tokens.at(0) == "ror" || tokens.at(0) == "ROR")
			addInst_srcA_srcB_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::ROR));

		else if (tokens.at(0) == "rrx" || tokens.at(0) == "RRX")
			addInst_srcA_dstA(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::RRX));

		else if (tokens.at(0) == "fadd" || tokens.at(0) == "FADD")
			addInst_srcA_srcB_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::ADD) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "fsub" || tokens.at(0) == "FSUB")
			addInst_srcA_srcB_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::SUB) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "fmul" || tokens.at(0) == "FMUL")
			addInst_srcA_srcB_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::MUL) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "fdiv" || tokens.at(0) == "FDIV")
			addInst_srcA_srcB_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::DIV) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "fsqrt" || tokens.at(0) == "FSQRT")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::SQRT) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "fneg" || tokens.at(0) == "FNEG")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::NEG));

		else if (tokens.at(0) == "fabs" || tokens.at(0) == "FABS")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::ABS));

		else if (tokens.at(0) == "cvtfi" || tokens.at(0) == "CVTFI")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::CVTFI) | static_cast<uint8_t>(FPU_RM::RTZ));

		else if (tokens.at(0) == "cvtfu" || tokens.at(0) == "CVTFU")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::CVTFU) | static_cast<uint8_t>(FPU_RM::RTZ));

		else if (tokens.at(0) == "cvtif" || tokens.at(0) == "CVTIF")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::CVTIF) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "cvtuf" || tokens.at(0) == "CVTUF")
			addInst_srcA_dstA_RM(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::CVTUF) | static_cast<uint8_t>(FPU_RM::RNE));

		else if (tokens.at(0) == "cmp" || tokens.at(0) == "CMP")
			addInst_srcA_srcB(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::SUB));

		else if (tokens.at(0) == "cpc" || tokens.at(0) == "CPC")
			addInst_srcA_srcB(static_cast<uint8_t>(INST::ALU), static_cast<uint8_t>(ALU_FUNC::SBC));

		else if (tokens.at(0) == "fcmp" || tokens.at(0) == "FCMP")
			addInst_srcA_srcB(static_cast<uint8_t>(INST::FPU), static_cast<uint8_t>(FPU_FUNC::CMP), toFloat);

		else if (tokens.at(0) == "jeq" || tokens.at(0) == "JEQ")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JEQ));

		else if (tokens.at(0) == "jne" || tokens.at(0) == "JNE")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JNE));

		else if (tokens.at(0) == "jhi" || tokens.at(0) == "JHI")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JHI));

		else if (tokens.at(0) == "jsh" || tokens.at(0) == "JSH")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JSH));

		else if (tokens.at(0) == "jsl" || tokens.at(0) == "JSL")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JSL));

		else if (tokens.at(0) == "jlo" || tokens.at(0) == "JLO")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JLO));

		else if (tokens.at(0) == "jgt" || tokens.at(0) == "JGT")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JGT));

		else if (tokens.at(0) == "jge" || tokens.at(0) == "JGE")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JGE));

		else if (tokens.at(0) == "jle" || tokens.at(0) == "JLE")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JLE));

		else if (tokens.at(0) == "jlt" || tokens.at(0) == "JLT")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JLT));

		else if (tokens.at(0) == "jmi" || tokens.at(0) == "JMI")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JMI));

		else if (tokens.at(0) == "jpl" || tokens.at(0) == "JPL")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JPL));

		else if (tokens.at(0) == "jvs" || tokens.at(0) == "JVS")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JVS));

		else if (tokens.at(0) == "jvc" || tokens.at(0) == "JVC")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JVC));

		else if (tokens.at(0) == "jmp" || tokens.at(0) == "JMP")
			addInst_addr(static_cast<uint8_t>(INST::JMP), static_cast<uint8_t>(JMP_FUNC::JAL));

		else if (tokens.at(0) == "ien" || tokens.at(0) == "IEN")
			addInst_noOperands(static_cast<uint8_t>(INST::IEN));

		else if (tokens.at(0) == "idi" || tokens.at(0) == "IDI")
			addInst_noOperands(static_cast<uint8_t>(INST::IDI));

		else if (tokens.at(0) == "wait" || tokens.at(0) == "WAIT")
			addInst_noOperands(static_cast<uint8_t>(INST::WAIT));

		else if (tokens.at(0) == "reti" || tokens.at(0) == "RETI")
			addInst_noOperands(static_cast<uint8_t>(INST::RETI));

		else if (tokens.at(0) == "call" || tokens.at(0) == "CALL")
			addInst_addr(static_cast<uint8_t>(INST::CALL));

		else if (tokens.at(0) == "ret" || tokens.at(0) == "RET")
			addInst_noOperands(static_cast<uint8_t>(INST::RET));

		// unknown instruction
		else
			error("unknown instruction '" + tokens.at(0) + "'.");
	}

	objectCode.resolveReferences(errorCount);
	
	if (objectCode.size() > memorySize)
	{
		std::cout << "object code exceeds memory size by " << objectCode.size() - memorySize << " words." << std::endl;
		errorCount++;
	}
	else if (objectCode.size() < memorySize)
		objectCode.resize(memorySize, 0);

	sourceFileManager.closeAll();

	if (errorCount == 0)
	{
		std::cout << "Compilation succeeded with " << warningCount << " warning(s)!" << std::endl;
		return true;
	}
	else
	{
		std::cout << "Compilation failed with " << errorCount << " error(s) and " << warningCount << " warning(s)!" << std::endl;
		objectCode.clear();
		return false;
	}
}

void Compiler::error(std::string message)
{
	std::cout << sourceFileManager.getPath() << ": line: " << sourceFileManager.getLineNumber() << ": error: " << message << std::endl;
	errorCount++;
}

void Compiler::warning(std::string message)
{
	std::cout << sourceFileManager.getPath() << ": line: " << sourceFileManager.getLineNumber() << ": warning: " << message << std::endl;
	warningCount++;
}

void Compiler::addInst_noOperands(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 1)
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
	else
		objectCode.append(getMachineCode(opcode, false, func, 0x00, 0x00,0x00));
}

void Compiler::addInst_dstA_imm(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t dstA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint32_t immediate = toImmediate(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
	objectCode.append(getMachineCode(opcode, true, func, 0x00, 0x00, dstA));
	objectCode.append(immediate);
}

void Compiler::addInst_srcB_dstA(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcB = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
	objectCode.append(getMachineCode(opcode, false, func, 0x00, srcB, dstA));
}

void Compiler::addInst_srcB_addr(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	std::string baseReg;
	std::string offset;

	if (!parseAddress(tokens.at(2), baseReg, offset))
	{
		error("invalid address '" + tokens.at(2) + "'.");
		return;
	}

	uint8_t srcA = baseReg.empty() ? 0 : toRegister(baseReg, std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t srcB = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));

	if (offset.empty())
		objectCode.append(getMachineCode(opcode, false, func, srcA, srcB, 0x00));
	else
	{
		uint32_t immediate = toImmediate(offset, std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, true, func, srcA, srcB, 0x00));
		objectCode.append(immediate);
	}
}

void Compiler::addInst_dstA_addr(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}
	
	std::string baseReg;
	std::string offset;

	if (!parseAddress(tokens.at(2), baseReg, offset))
	{
		error("invalid address '" + tokens.at(2) + "'.");
		return;
	}

	uint8_t srcA = baseReg.empty() ? 0 : toRegister(baseReg, std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));

	if (offset.empty())
		objectCode.append(getMachineCode(opcode, false, func, srcA, 0x00, dstA));
	else
	{
		uint32_t immediate = toImmediate(offset, std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, true, func, srcA, 0x00, dstA));
		objectCode.append(immediate);
	}
}

void Compiler::addInst_srcB(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 2)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcB = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	objectCode.append(getMachineCode(opcode, false, func, 0x00, srcB, 0x00));
}

void Compiler::addInst_dstA(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 2)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t dstA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	objectCode.append(getMachineCode(opcode, false, func, 0x00, 0x00, dstA));
}

void Compiler::addInst_srcA_srcB_dstA(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3 && tokens.size() != 4)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA = tokens.size() == 4 ? toRegister(tokens.at(3), std::bind(&Compiler::error, this, std::placeholders::_1)) : srcA;

	if (isRegister(tokens.at(2)))
	{
		uint8_t srcB = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, false, func, srcA, srcB, dstA));
	}
	else
	{
		uint32_t immediate = toImmediate(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, true, func, srcA, 0x00, dstA));
		objectCode.append(immediate);
	}
}

void Compiler::addInst_srcA_dstA(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 2 && tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA = tokens.size() == 3 ? toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1)) : srcA;
	objectCode.append(getMachineCode(opcode, false, func, srcA, 0x00, dstA));
}

void Compiler::addInst_srcA_srcB_dstA_RM(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3 && tokens.size() != 4 && tokens.size() != 5)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA;

	// src_a, src_b, dstA, RM
	if (tokens.size() == 5)
	{
		// default round mode is given as parameter
		// it gets overwritten when tokens contain a specific rounding mode
		func = (func & 0x0F) | toRoundingMode(tokens.at(4), std::bind(&Compiler::error, this, std::placeholders::_1));
		dstA = toRegister(tokens.at(3), std::bind(&Compiler::error, this, std::placeholders::_1));
	}
	else if (tokens.size() == 4)
	{
		// src_a, src_b, dstA
		if (isRegister(tokens.at(3)))
			dstA = toRegister(tokens.at(3), std::bind(&Compiler::error, this, std::placeholders::_1));

		// src_a, src_b, RM
		else
		{
			func = (func & 0x0F) | toRoundingMode(tokens.at(3), std::bind(&Compiler::error, this, std::placeholders::_1));
			dstA = srcA;
		}
	}
	// src_a, src_b
	else
		dstA = srcA;

	if (isRegister(tokens.at(2)))
	{
		uint8_t srcB = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, false, func, srcA, srcB, dstA));
	}
	else
	{
		uint32_t immediate = toImmediate(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, true, func, srcA, 0x00, dstA));
		objectCode.append(immediate);
	}
}

void Compiler::addInst_srcA_dstA_RM(uint8_t opcode, uint8_t func)
{
	if (tokens.size() != 2 && tokens.size() != 3 && tokens.size() != 4)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));
	uint8_t dstA;

	// src_a, dstA, RM
	if (tokens.size() == 4)
	{
		func = (func & 0x0F) | toRoundingMode(tokens.at(3), std::bind(&Compiler::error, this, std::placeholders::_1));
		dstA = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
	}	
	else if (tokens.size() == 3)
	{
		// src_a, dstA
		if (isRegister(tokens.at(2)))
			dstA = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));

		// src_a, RM
		else
		{
			func = (func & 0x0F) | toRoundingMode(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
			dstA = srcA;
		}
	}
	// src_a
	else
		dstA = srcA;

	objectCode.append(getMachineCode(opcode, false, func, srcA, 0x00, dstA));
}

void Compiler::addInst_srcA_srcB(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 3)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	uint8_t srcA = toRegister(tokens.at(1), std::bind(&Compiler::error, this, std::placeholders::_1));

	if (isRegister(tokens.at(2)))
	{
		uint8_t srcB = toRegister(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, false, func, srcA, srcB, 0x00));
	}
	else
	{
		uint32_t immediate = toImmediate(tokens.at(2), std::bind(&Compiler::error, this, std::placeholders::_1));
		objectCode.append(getMachineCode(opcode, true, func, srcA, 0x00, 0x00));
		objectCode.append(immediate);
	}
}

void Compiler::addInst_addr(uint8_t opcode, uint8_t func, std::function<uint32_t(std::string, std::function<void(std::string)>)> toImmediate)
{
	if (tokens.size() != 2)
	{
		error("invalid number of operands to " + tokens.at(0) + " instruction.");
		return;
	}

	std::string baseReg;
	std::string offset;

	if (parseAddress(tokens.at(1), baseReg, offset))
	{
		uint8_t srcA = baseReg.empty() ? 0 : toRegister(baseReg, std::bind(&Compiler::error, this, std::placeholders::_1));

		// [src_a]
		if (offset.empty())
			objectCode.append(getMachineCode(opcode, false, func, srcA, 0x00, 0x00));
		// [src_a=R0 + immediate]
		else
		{
			uint32_t immediate = toImmediate(offset, std::bind(&Compiler::error, this, std::placeholders::_1));
			objectCode.append(getMachineCode(opcode, true, func, srcA, 0x00,0x00));
			objectCode.append(immediate);
		}
	}
	// label
	else
	{
		objectCode.append(getMachineCode(opcode, true, func, 0x00, 0x00, 0x00));
		objectCode.append(0x00000000);
		objectCode.addReference(tokens.at(1), sourceFileManager.getPath(), sourceFileManager.getLineNumber());
	}
}