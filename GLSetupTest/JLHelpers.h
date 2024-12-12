#ifndef JLHelpers
#define JLHelpers

#include <string>
#include <algorithm>

namespace JLEngine
{
	class Str
	{
	public:
		static std::string ToLower(const std::string& input);
		static bool Contains(const std::string& str, const std::string& substr);
	};
}

#endif