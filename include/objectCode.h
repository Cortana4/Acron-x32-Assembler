#pragma once
#include <string>
#include <vector>

struct Reference
{
	std::string identifier;
	size_t pos;
	std::string sourceFile;
	unsigned int lineNumber;
};

struct Dereference
{
	std::string identifier;
	uint32_t address;
};

class ObjectCode
{
public:
	ObjectCode();
	~ObjectCode();

	void append(uint32_t code);
	void append(std::vector<uint32_t> code);
	size_t size();
	void resize(size_t n, const uint32_t& value);
	void clear();
	bool empty();

	void addReference(std::string identifier, std::string sourceFile, unsigned int lineNumber);
	void addDereference(std::string identifier);
	void link(int& errorCount);

	bool exportRaw(std::string path);
	bool exportMif(std::string path);
	bool exportCoe(std::string path);

private:
	std::vector<uint32_t> data;
	std::vector<Reference> references;
	std::vector<Dereference> dereferences;
};
