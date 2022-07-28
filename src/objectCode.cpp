#include "objectCode.h"
#include "converter.h"
#include "parser.h"
#include "constants.h"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <cmath>

ObjectCode::ObjectCode()
{
	data.reserve(memorySize);
}

ObjectCode::~ObjectCode()
{
	clear();
}

void ObjectCode::append(uint32_t code)
{
	data.push_back(code);
}

void ObjectCode::append(std::vector<uint32_t> code)
{
	data.insert(data.end(), code.begin(), code.end());
}

size_t ObjectCode::size()
{
	return data.size();
}

void ObjectCode::resize(size_t n, const uint32_t& value)
{
	data.resize(n, value);
}

void ObjectCode::clear()
{
	data.clear();
	references.clear();
	dereferences.clear();
}

bool ObjectCode::empty()
{
	return data.empty();
}

void ObjectCode::addReference(std::string identifier, std::string sourceFile, unsigned int lineNumber)
{
	// placeholder which will be resolved by linking
	append(0x00000000);
	Reference reference = { identifier, data.size() - 1, sourceFile, lineNumber };
	references.push_back(reference);
}

void ObjectCode::addDereference(std::string identifier)
{
	Dereference dereference = { identifier, data.size() + basePtr };
	dereferences.push_back(dereference);
}

void ObjectCode::link(int& errorCount)
{
	bool found;

	for (Reference& reference : references)
	{
		found = false;

		for (Dereference& dereference : dereferences)
		{
			if (dereference.identifier == reference.identifier)
			{
				data.at(reference.pos) = dereference.address;
				found = true;
				break;
			}
		}

		if (!found)
		{
			std::cout << reference.sourceFile << ": line: " << reference.lineNumber << ": error: cannot resolve '" << reference.identifier << "'." << std::endl;
			errorCount++;
		}
	}
}

bool ObjectCode::exportRaw(std::string path)
{
	removeQuotes(path);
	if (!endsWith(path, ".hex"))
		path += ".hex";

	std::filesystem::path fs_path = path;
	fs_path = std::filesystem::absolute(fs_path);

	std::ofstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		file.open(fs_path, std::ios::binary | std::ios::trunc);
		for (uint32_t value : data)
			file.write(reinterpret_cast<const char*>(&value), sizeof(uint32_t));
	}
	catch (std::ofstream::failure&)
	{
		std::cout << "Fatal: error creating file " << fs_path << "!" << std::endl;
		file.close();
		return false;
	}

	file.close();
	return true;
}

bool ObjectCode::exportMif(std::string path)
{
	removeQuotes(path);
	if (!endsWith(path, ".mif"))
		path += ".mif";

	std::filesystem::path fs_path = path;
	fs_path = std::filesystem::absolute(fs_path);

	std::ofstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	unsigned int fill = static_cast<unsigned int>(std::ceil(std::log2(memorySize) * 0.25));

	try
	{
		file.open(fs_path, std::ios::trunc);
		file << "DEPTH = " << memorySize << ";" << std::endl;
		file << "WIDTH = 32;" << std::endl;
		file << "ADDRESS_RADIX = HEX;" << std::endl;
		file << "DATA_RADIX = HEX;" << std::endl;
		file << "CONTENT" << std::endl;
		file << "BEGIN" << std::endl;
		file << std::endl;

		for (size_t i = 0; i < data.size(); i++)
		{
			
			file << std::setbase(16) << std::setw(fill) << std::setfill('0') << i << " : ";
			file << std::setbase(16) << std::setw(8) << std::setfill('0') << data.at(i) << ";" << std::endl;
		}

		file << std::endl;
		file << "END;" << std::endl;
	}
	catch (std::ofstream::failure&)
	{
		std::cout << "Fatal: error creating file " << fs_path << "!" << std::endl;
		file.close();
		return false;
	}

	file.close();
	return true;
}

bool ObjectCode::exportCoe(std::string path)
{
	removeQuotes(path);
	if (!endsWith(path, ".coe"))
		path += ".coe";

	std::filesystem::path fs_path = path;
	fs_path = std::filesystem::absolute(fs_path);

	std::ofstream file;
	file.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	try
	{
		file.open(fs_path, std::ios::trunc);
		file << "memory_initialization_radix=16;" << std::endl;
		file << "memory_initialization_vector=" << std::endl;

		for (size_t i = 0; i < data.size(); i++)
		{
			file << std::setbase(16) << std::setw(8) << std::setfill('0') << data.at(i);

			if (i == data.size() - 1)
				file << ';' << std::endl;
			else
				file << ',' << std::endl;
		}
	}
	catch (std::ofstream::failure&)
	{
		std::cout << "Fatal: error creating file " << fs_path << "!" << std::endl;
		file.close();
		return false;
	}

	file.close();
	return true;
}
