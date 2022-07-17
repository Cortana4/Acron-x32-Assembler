#pragma once
#include <string>
#include <fstream>
#include <filesystem>

class SourceFile
{
public:
	SourceFile(std::filesystem::path path);
	~SourceFile();

	std::string getPath();
	bool getLine(std::string& line);
	unsigned int getLineNumber();

private:
	std::filesystem::path path;
	std::ifstream file;
	unsigned int lineNumber;
};
