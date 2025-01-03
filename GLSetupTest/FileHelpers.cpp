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

	bool WriteTextFile(const std::string& filename, std::string& output)
	{
		std::ofstream ofile(filename, std::ios::out);

		if (!ofile.is_open())
		{
			return false;
		}

		ofile << output;

		if (ofile.fail())
		{
			return false;
		}
		return true;
	}
}