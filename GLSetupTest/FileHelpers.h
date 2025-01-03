#ifndef FILE_HELPERS_H
#define FILE_HELPERS_H

#include <fstream>
#include <string>

namespace JLEngine
{
	bool ReadTextFile(const std::string& filename, std::string& output);

	bool WriteTextFile(const std::string& filename, std::string& output);
}

#endif