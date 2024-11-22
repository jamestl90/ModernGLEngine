#include "FileHelpers.h"

namespace JLEngine
{
	bool ReadTextFile(const std::string& filename, std::string& output)
	{
		std::ifstream file(filename, std::ios::in);

		if(file.is_open())
		{
			std::string line("");

			while(getline(file, line))
			{
				output += "\n" + line;
			}
			file.close();
		}
		else
		{
			return false;
		}
		return true;
	}
}