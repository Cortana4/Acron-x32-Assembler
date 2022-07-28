#include "sourceFileManager.h"
#include "converter.h"

#include <iostream>

SourceFileManager::SourceFileManager()
{

}

SourceFileManager::~SourceFileManager()
{
	closeAll();
}

bool SourceFileManager::addFile(std::string path)
{
	removeQuotes(path);
	std::filesystem::path fs_path = path;

	// path of 1st file is either absolute or relative to the executable
	if (sourceFileStack.empty())
	{
		fs_path = std::filesystem::absolute(fs_path);
		basePath = fs_path.parent_path();
	}
	// path of nth file is either absolute or relative to parent directory of 1st file
	else
	{
		if (fs_path.is_relative())
			fs_path = basePath / fs_path;
		else
			fs_path = std::filesystem::absolute(fs_path);
	}

	for (std::filesystem::path& included_fs_path : included_fs_paths)
	{
		if (fs_path == included_fs_path)
			return true;
	}

	try
	{
		SourceFile* sourceFile = new SourceFile{ fs_path };
		sourceFileStack.push_back(sourceFile);
		included_fs_paths.push_back(fs_path);
		return true;
	}
	catch (std::ifstream::failure&){ return false; }
}

void SourceFileManager::closeAll()
{
	for (SourceFile*& sourceFile : sourceFileStack)
		delete sourceFile;

	sourceFileStack.clear();
	included_fs_paths.clear();
	basePath.clear();
}

std::string SourceFileManager::getPath()
{
	return sourceFileStack.back()->getPath();
}

bool SourceFileManager::getLine(std::string& line)
{
	while (!sourceFileStack.empty())
	{
		if (sourceFileStack.back()->getLine(line))
			return true;
		else
		{
			delete sourceFileStack.back();
			sourceFileStack.pop_back();
		}
	}

	return false;
}

unsigned int SourceFileManager::getLineNumber()
{
	return sourceFileStack.back()->getLineNumber();
}