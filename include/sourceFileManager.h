#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "sourceFile.h"

class SourceFileManager
{
public:
	SourceFileManager();
	~SourceFileManager();

	bool addFile(std::string path);
	void closeAll();
	std::string getPath();
	bool getLine(std::string& line);
	unsigned int getLineNumber();

private:
	std::vector<SourceFile*> sourceFileStack;
	std::filesystem::path basePath;
};
