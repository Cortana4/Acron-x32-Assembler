#include "parser.h"
#include "converter.h"

void parseLine(std::string line, std::vector<std::string>& tokens)
{
	tokens.clear();
	std::string token;
	size_t pos;

	line = line.substr(0, find_first_of_outside_str(line, ";"));	// cut comment
	line.erase(0, line.find_first_not_of(" \t"));					// cut whitespaces left
	line.erase(line.find_last_not_of(" \t\n") + 1);					// cut whitespaces right

	if (line.empty())
		return;

	pos = find_first_of_outside_str(line, " \t:");
	token = line.substr(0, pos);									// parse 1st token
	line.erase(0, pos);												// cut token left
	line.erase(0, line.find_first_not_of(" \t"));					// cut whitespaces left

	if (!line.empty() && line.at(0) == ':')
	{
		token += ':';												// add delimiter ':' to label
		line.erase(0, 1);											// cut delimiter ':' left
		line.erase(0, line.find_first_not_of(" \t"));				// cut whitespaces left
	}

	tokens.push_back(token);

	while (!line.empty())
	{
		pos = find_first_of_outside_str(line, ",");
		token = line.substr(0, pos);								// parse nth token
		token.erase(token.find_last_not_of(" \t") + 1);				// cut whitespaces right

		tokens.push_back(token);

		if (pos == std::string::npos)
			line.erase(0, pos);										// cut last token
		else
		{
			line.erase(0, pos + 1);									// cut token left
			line.erase(0, line.find_first_not_of(" \t"));			// cut whitespaces left
			if (line.empty())										// found delimiter ',' but no token
				tokens.push_back("");
		}	
	}
}

bool parseAddress(std::string address, std::string& baseReg, std::string& offset)
{
	baseReg.clear();
	offset.clear();

	std::string token_1;
	std::string token_2;

	if (address.size() <= 2)
		return false;

	if (address.front() == '[' && address.back() == ']')
	{
		address.erase(0, 1);										// cut '['
		address.pop_back();											// cut ']'
	}
	else
		return false;

	address.erase(0, address.find_first_not_of(" \t"));				// cut whitespaces left
	address.erase(address.find_last_not_of(" \t") + 1);				// cut whitespaces right

	if (address.empty())
		return false;

	if (address.front() == '+' || address.front() == '-')
	{
		token_1 += address.front();									// parse sign of first token
		address.erase(0, 1);										// cut sign
		address.erase(0, address.find_first_not_of(" \t"));			// cut whitespaces left
		if (address.empty())
			return false;
	}

	token_1 += address.substr(0, address.find_first_of(" \t+-"));	// parse first token

	if (isInt(token_1))												// first token is offset
		offset = token_1;
	else
	{
		if (token_1.at(0) == '+')									// first token is register, ignore positive sign
			token_1.erase(0, 1);

		baseReg = token_1;
	}

	address.erase(0, address.find_first_of(" \t+-"));				// cut first token
	address.erase(0, address.find_first_not_of(" \t"));				// cut whitespaces left

	if (address.empty())
		return true;

	if (address.front() == '+' || address.front() == '-')
	{
		token_2 += address.front();									// parse sign of second token
		address.erase(0, 1);										// cut sign
		address.erase(0, address.find_first_not_of(" \t"));			// cut whitespaces left
		if (address.empty())
			return false;
	}
	else
		return false;

	token_2 += address.substr(0, address.find_first_of(" \t"));		// parse second token

	if (isInt(token_2))
	{
		if (offset.empty())											// second token is offset, only one offset is allowed
			offset = token_2;
		else
			return false;
	}
	else
	{
		if (baseReg.empty())
		{
			if (token_2.at(0) == '+')								// second token is register, ignore positive sign
				token_2.erase(0, 1);

			baseReg = token_2;
		}
		else
			return false;
	}

	address.erase(0, address.find_first_of(" \t"));					// cut second token
	address.erase(0, address.find_first_not_of(" \t"));				// cut whitespaces left
	return address.empty();											// if address still not empty, return false
}

bool contains(std::string str, const char c)
{
	for (const char& currentChar : str)
	{
		if (currentChar == c)
			return true;
	}

	return false;
}

bool endsWith(std::string str, std::string end)
{
	if (str.length() >= end.length())
	{
		if (str.compare(str.length() - end.length(), end.length(), end) == 0)
			return true;
		else
			return false;
	}
	else
		return false;
}

size_t find_first_of_outside_str(std::string str, std::string charsToFind)
{
	size_t pos = 0;
	bool found = false;
	unsigned char state = 0x00;
	const char* currentChar = str.c_str();
	char strDelimiter;

	while (!found && *currentChar != '\0')
	{
		switch (state)
		{
		case 0x00:
			if (contains(charsToFind, *currentChar))
				found = true;
			else if (*currentChar == '\"' || *currentChar == '\'')
			{
				strDelimiter = *currentChar;
				state = 0x01;
				currentChar++;
				pos++;
			}
			else
			{
				currentChar++;
				pos++;
			}
			break;

		case 0x01:	// inside string
			if (*currentChar == strDelimiter)
				state = 0x00;
			else if (*currentChar == '\\')
				state = 0x02;

			currentChar++;
			pos++;
			break;

		case 0x02:	//escape char
			state = 0x01;
			currentChar++;
			pos++;
			break;
		}
	}

	if (found)
		return pos;
	else
		return std::string::npos;
}
