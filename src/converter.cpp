#include "converter.h"
#include "constants.h"

#include <limits>

void removeQuotes(std::string& str)
{
	if (str.size() >= 2)
	{
		if (str.front() == '\"' && str.back() == '\"')
		{
			str.erase(0, 1);
			str.pop_back();
		}
	}
}

bool isDec(std::string str)
{
	size_t startAt = 0;

	if (str.empty())
		return false;

	if (str.at(0) == '+' || str.at(0) == '-')
	{
		if (str.size() < 2)
			return false;

		startAt = 1;
	}

	if (str.find_first_not_of("0123456789", startAt) != std::string::npos)
		return false;

	return true;
}

bool isHex(std::string str)
{
	size_t startAt = 0;

	if (str.size() < 3)
		return false;

	if (str.at(0) == '+' || str.at(0) == '-')
	{
		if (str.size() < 4)
			return false;

		startAt = 1;
	}

	if (str.substr(startAt, 2) != "0x")
		return false;

	if (str.find_first_not_of("0123456789aAbBcCdDeEfF", startAt + 2) != std::string::npos)
		return false;

	return true;
}

bool isBin(std::string str)
{
	size_t startAt = 0;

	if (str.size() < 3)
		return false;

	if (str.at(0) == '+' || str.at(0) == '-')
	{
		if (str.size() < 4)
			return false;

		startAt = 1;
	}

	if (str.substr(startAt, 2) != "0b")
		return false;

	if (str.find_first_not_of("01", startAt + 2) != std::string::npos)
		return false;

	return true;
}

bool isInt(std::string str)
{
	return isDec(str) || isHex(str) || isBin(str);
}

bool isFloat(std::string str)
{
	if (str.empty() || isInt(str))
		return false;

	try
	{
		size_t len;
		long double f = std::stold(str, &len);
		return len == str.size();
	}
	catch (std::invalid_argument&) { return false; }
	catch (std::out_of_range&) { return false; }
}

bool isChar(std::string str)
{
	if (str.size() != 3 && str.size() != 4)
		return false;

	if (str.front() != '\'' || str.back() != '\'')
		return false;

	if (str.size() == 4)
		return str.at(1) == '\\';

	return str != "\'\\\'";
}

bool isString(std::string str)
{
	bool escapeChar = false;

	if (str.size() < 2)
		return false;

	if (str.front() != '\"' || str.back() != '\"')
		return false;

	removeQuotes(str);

	for (const char& c : str)
	{
		if (c == '\\' && !escapeChar)
			escapeChar = true;
		else
			escapeChar = false;
	}

	return !escapeChar;
}

bool isRegister(std::string str)
{
	if (str.size() < 2)
		return false;

	if (str.at(0) == 'r' || str.at(0) == 'R')
	{
		int index = std::stoi(str.substr(1));
		return index >= 0 && index <= 63;
	}
	else
		return str == "sp" || str == "SP" ||
			str == "sr" || str == "SR" ||
			str == "pc" || str == "PC";
}

uint32_t toInt(std::string str, std::function<void(std::string)> errorFunc)
{
	int64_t _val;

	try
	{
		if (isDec(str))
		{
			_val = std::stoll(str, nullptr, 10);
			if (_val < std::numeric_limits<int32_t>::min() || _val > std::numeric_limits<uint32_t>::max())
				throw std::out_of_range{ "out of range" };
			return static_cast<uint32_t>(_val);
		}

		if (isHex(str))
		{
			_val = std::stoll(str, nullptr, 16);
			if (_val < std::numeric_limits<int32_t>::min() || _val > std::numeric_limits<uint32_t>::max())
				throw std::out_of_range{ "out of range" };
			return static_cast<uint32_t>(_val);
		}

		if (isBin(str))
		{
			std::string noSuffix = str;
			size_t startAt = 0;
			if (noSuffix.front() == '+' || noSuffix.front() == '-')
				startAt = 1;
			noSuffix.erase(startAt, 2);

			_val = std::stoll(noSuffix, nullptr, 2);
			if (_val < std::numeric_limits<int32_t>::min() || _val > std::numeric_limits<uint32_t>::max())
				throw std::out_of_range{ "out of range" };
			return static_cast<uint32_t>(_val);
		}

		return toChar(str);
	}
	catch (std::invalid_argument&)
	{
		if (errorFunc)
			errorFunc("cannot convert '" + str + "' to int.");
		else
			throw;
	}
	catch (std::out_of_range&)
	{
		if (errorFunc)
			errorFunc("'" + str + "' cannot be represented with 32 bit.");
		else
			throw;
	}

	return 0;
}

uint32_t toFloat(std::string str, std::function<void(std::string)> errorFunc)
{
	union Float
	{
		float val;
		uint32_t hex;
	} f;

	try
	{
		if (str.empty() || isInt(str))
			throw std::invalid_argument{ "invalid argument" };

		size_t len;
		f.val = std::stof(str, &len);
		if (len != str.size())
			throw std::invalid_argument{ "invalid argument" };

		return f.hex;
	}
	catch (std::invalid_argument&)
	{
		if (errorFunc)
			errorFunc("cannot convert '" + str + "' to float.");
		else
			throw;
	}
	catch (std::out_of_range&)
	{
		if (errorFunc)
			errorFunc("'" + str + "' cannot be represented with 32 bit.");
		else
			throw;
	}

	return 0;
}

uint32_t toChar(std::string str, std::function<void(std::string)> errorFunc)
{
	try
	{
		if (!isChar(str))
			throw std::invalid_argument{ "invalid argument" };

		if (str.size() != 4)
			return static_cast<uint32_t>(str.at(1));

		if (str == "\'\\'\'")
			return static_cast<uint32_t>('\'');

		if (str == "\'\\\"\'")
			return static_cast<uint32_t>('\"');

		if (str == "\'\\?\'")
			return static_cast<uint32_t>('\?');

		if (str == "\'\\\\\'")
			return static_cast<uint32_t>('\\');

		if (str == "\'\\a\'")
			return static_cast<uint32_t>('\a');

		if (str == "\'\\b\'")
			return static_cast<uint32_t>('\b');

		if (str == "\'\\f\'")
			return static_cast<uint32_t>('\f');

		if (str == "\'\\n\'")
			return static_cast<uint32_t>('\n');

		if (str == "\'\\r\'")
			return static_cast<uint32_t>('\r');

		if (str == "\'\\t\'")
			return static_cast<uint32_t>('\t');

		if (str == "\'\\v\'")
			return static_cast<uint32_t>('\v');

		if (str == "\'\\0\'")
			return static_cast<uint32_t>('\0');

		throw std::invalid_argument{ "invalid argument" };
	}
	catch (std::invalid_argument&)
	{
		if (errorFunc)
			errorFunc("cannot convert '" + str + "' to char.");
		else
			throw;
	}

	return 0;
}

uint32_t toWord(std::string str, std::function<void(std::string)> errorFunc)
{
	try { return toInt(str); }
	catch (std::invalid_argument&)
	{
		try { return toFloat(str); }
		catch (std::invalid_argument&) { if (!errorFunc) throw; }
		catch (std::out_of_range&) { if (!errorFunc) throw; }
	}
	catch (std::out_of_range&) { if (!errorFunc) throw; }

	errorFunc("cannot convert '" + str + "' to 32 bit word.");
	return 0;
}

std::vector<uint32_t> toString(std::string str, std::function<void(std::string)> errorFunc)
{
	std::string currentChar;
	bool escapeChar = false;
	std::vector<uint32_t> chars;

	try
	{
		if (!isString(str))
			throw std::invalid_argument{ "invalid argument" };

		for (const char& c : str.substr(1, str.size() - 2))
		{
			currentChar += c;
			if (currentChar == "\\" && !escapeChar)
				escapeChar = true;
			else
			{
				escapeChar = false;

				if (currentChar.size() == 1)
					chars.push_back(static_cast<uint32_t>(currentChar.front()));

				else
				{
					if (currentChar == "\\'")
						chars.push_back(static_cast<uint32_t>('\''));

					else if (currentChar == "\\\"")
						chars.push_back(static_cast<uint32_t>('\"'));

					else if (currentChar == "\\?")
						chars.push_back(static_cast<uint32_t>('\?'));

					else if (currentChar == "\\\\")
						chars.push_back(static_cast<uint32_t>('\\'));

					else if (currentChar == "\\a")
						chars.push_back(static_cast<uint32_t>('\a'));

					else if (currentChar == "\\b")
						chars.push_back(static_cast<uint32_t>('\b'));

					else if (currentChar == "\\f")
						chars.push_back(static_cast<uint32_t>('\f'));

					else if (currentChar == "\\n")
						chars.push_back(static_cast<uint32_t>('\n'));

					else if (currentChar == "\\r")
						chars.push_back(static_cast<uint32_t>('\r'));

					else if (currentChar == "\\t")
						chars.push_back(static_cast<uint32_t>('\t'));

					else if (currentChar == "\\v")
						chars.push_back(static_cast<uint32_t>('\v'));

					else if (currentChar == "\\0")
						chars.push_back(static_cast<uint32_t>('\0'));

					else
						throw std::invalid_argument{ "invalid argument" };
				}

				currentChar.clear();
			}
		}

		if (escapeChar)
			throw std::invalid_argument{ "invalid argument" };

		if (chars.empty())
			chars.push_back(static_cast<uint32_t>('\0'));

		else if (chars.back() != static_cast<uint32_t>('\0'))
			chars.push_back(static_cast<uint32_t>('\0'));
	}
	catch (std::invalid_argument&)
	{
		if (errorFunc)
			errorFunc("cannot convert '" + str + "' to char array.");
		else
			throw;

		chars.clear();
	}

	return chars;
}

std::vector<uint32_t> toWordArray(std::string str, std::function<void(std::string)> errorFunc)
{
	try { return std::vector<uint32_t>{1, toWord(str)}; }
	catch (std::invalid_argument&)
	{
		try { return toString(str); }
		catch (std::invalid_argument&) { if (!errorFunc) throw; }
		catch (std::out_of_range&) { if (!errorFunc) throw; }
	}
	catch (std::out_of_range&) { if (!errorFunc) throw; }

	errorFunc("cannot convert '" + str + "' to word array.");
	return std::vector<uint32_t>{};
}

uint8_t toRegister(std::string str, std::function<void(std::string)> errorFunc)
{
	try
	{
		if (str.size() < 2)
			throw std::invalid_argument{ "invalid argument" };

		if (str.at(0) == 'r' || str.at(0) == 'R')
		{
			int index = std::stoi(str.substr(1));
			if (index < 0 || index > 63)
				throw std::invalid_argument{ "invalid argument" };

			return static_cast<uint8_t>(index);
		}

		if (str == "sp" || str == "SP")
			return SP;

		if (str == "sr" || str == "SR")
			return SR;

		if (str == "pc" || str == "PC")
			return PC;

		throw std::invalid_argument{ "invalid argument" };
	}
	catch (std::invalid_argument&)
	{
		if (errorFunc)
			errorFunc("unknown register '" + str + "'.");
		else
			throw;
	}

	return 0;
}

uint8_t toRoundingMode(std::string str, std::function<void(std::string)> errorFunc)
{

	if (str == "rne" || str == "RNE")
		return static_cast<uint8_t>(FPU_RM::RNE);

	if (str == "rmm" || str == "RMM")
		return static_cast<uint8_t>(FPU_RM::RMM);

	if (str == "rtz" || str == "RTZ")
		return static_cast<uint8_t>(FPU_RM::RTZ);

	if (str == "rdn" || str == "RDN")
		return static_cast<uint8_t>(FPU_RM::RDN);

	if (str == "rup" || str == "RUP")
		return static_cast<uint8_t>(FPU_RM::RUP);

	try
	{
		uint32_t rm = toInt(str);

		if (rm > 4)
			throw std::out_of_range{ "out of range" };

		return static_cast<uint8_t>(rm) << 4;
	}
	catch (std::invalid_argument&) { if (!errorFunc) throw; }
	catch (std::out_of_range&) { if (!errorFunc) throw; }

	errorFunc("unknown rounding mode '" + str + "'.");
	return 0;
}

uint32_t getMachineCode(uint8_t opcode, bool fetchImmediate, uint8_t func, uint8_t srcA, uint8_t srcB, uint8_t dst)
{
	uint32_t machineCode = 0;

	machineCode |= (opcode & 0x1F) << 27;
	machineCode |= fetchImmediate << 26;
	machineCode |= func << 18;
	machineCode |= (srcA & 0x3F) << 12;
	machineCode |= (srcB & 0x3F) << 6;
	machineCode |= (dst & 0x3F);

	return machineCode;
}
