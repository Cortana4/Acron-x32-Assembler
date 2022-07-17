#include "sourceFile.h"

SourceFile::SourceFile(std::filesystem::path path) : path{ path }, lineNumber{ 0 }
{
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	file.open(path); 
}

SourceFile::~SourceFile()
{
	file.close();
}

std::string SourceFile::getPath()
{
	return path.string();
}

bool SourceFile::getLine(std::string& line)
{
	try
	{
		std::getline(file, line);
		lineNumber++;
		return true;
	}
	catch (std::ifstream::failure&)
	{
		line.clear();
		return false;
	}
}

unsigned int SourceFile::getLineNumber()
{
	return lineNumber;
}