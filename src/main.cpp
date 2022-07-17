#include <iostream>
#include <string>

#include "compiler.h"

int main(int argC, char* argV[])
{
	std::string srcPath;
	std::string dstPath;
	std::string option = "-raw"; // default option

	Compiler compiler;

	// process input arguments
	// no arguments
	if (argC == 1)
	{
		std::cout << "Fatal: no source file specified!" << std::endl;
		return -1;
	}
	// source Path only
	else if (argC == 2)
	{
		srcPath = argV[1];
		dstPath = srcPath.substr(0, srcPath.find_last_of('.'));
	}
	// source path + destination path / option
	else if (argC == 3)
	{
		srcPath = argV[1];
		// 2nd argument is option
		if (argV[2][0] == '-')
		{
			if (std::string(argV[2]) == "-raw" || std::string(argV[2]) == "-mif" || std::string(argV[2]) == "-coe")
			{
				dstPath = srcPath.substr(0, srcPath.find_last_of('.'));
				option = argV[2];
			}
			else
			{
				std::cout << "Fatal: invalid option '" << argV[2] << "'!" << std::endl;
				return -1;
			}
		}
		// 2nd argument is destination path
		else
			dstPath = argV[2];
	}
	// source path + destination path + option
	else if (argC == 4)
	{
		srcPath = argV[1];
		dstPath = argV[2];

		if (std::string(argV[3]) == "-raw" || std::string(argV[3]) == "-mif" || std::string(argV[3]) == "-coe")
			option = argV[3];
		else
		{
			std::cout << "Fatal: invalid option '" << argV[2] << "'!" << std::endl;
			return -1;
		}
	}
	// too many arguments
	else
	{
		std::cout << "Fatal: invalid number of arguments!" << std::endl;
		return -1;
	}

	if (compiler.compileSource(srcPath))
	{
		if (option == "-mif")
			return compiler.objectCode.exportMif(dstPath) ? 0 : -1;

		if (option == "-coe")
			return compiler.objectCode.exportCoe(dstPath) ? 0 : -1;

		else
			return compiler.objectCode.exportRaw(dstPath) ? 0 : -1;
	}

	return -1;
}