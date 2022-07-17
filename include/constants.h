#pragma once
#include <iostream>

const uint32_t memorySize = 4096;
const uint8_t SP = 61;
const uint8_t SR = 62;
const uint8_t PC = 63;

extern uint32_t basePtr;

enum class INST : uint8_t
{
	NOP=0,	INR,	MOV,	STM,
	LDM,	PUSH,	POP,	ALU,
	MUL,	DIV,	FPU,	JMP,
	IEN,	IDI,	WAIT,	RETI,
	CALL,	RET
};

enum class ALU_FUNC : uint8_t
{
	ADD=0,	ADC,	SUB,	SBC,
	INC,	DEC,	NEG,	AND,
	OR,		XOR,	NOT,	LSL,
	LSR,	ASR,	ROR,	RRX
};

enum class MUL_FUNC : uint8_t
{
	UMULL=0,
	UMULH,
	SMULL,
	SMULH
};

enum class DIV_FUNC : uint8_t
{
	UDIV=0,
	SDIV,
	UMOD,
	SMOD
};

enum class FPU_FUNC : uint8_t
{
	ADD=0,	SUB,	MUL,	DIV,
	SQRT,	NEG,	ABS,	CVTFI,
	CVTFU,	CVTIF,	CVTUF,	CMP
};

enum class FPU_RM : uint8_t
{
	RNE=0x00,
	RMM=0x10,
	RTZ=0x20,
	RDN=0x30,
	RUP=0x40
};

enum class JMP_FUNC : uint8_t
{
	JEQ=1,	JNE,	JHI,	JSH,
	JSL,	JLO,	JGT,	JGE,
	JLE,	JLT,	JMI,	JPL,
	JVS,	JVC,	JAL
};