#include "JLHelpers.h"

namespace JLEngine
{
	std::string Str::ToLower(const std::string& input)
	{
		auto lowerStr = input;
		std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
			[](unsigned char c) { return std::tolower(c); });
		return lowerStr;
	}
}