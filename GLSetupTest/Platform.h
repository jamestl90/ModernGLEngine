#ifndef PLATFORM_SPECIFIC_H
#define PLATFORM_SPECIFIC_H

#undef APIENTRY
#include <windows.h>
#include <string>

namespace JLEngine
{
	class Platform
	{
	public:
		static bool AlertBox(std::string message, std::string caption)
		{
			MessageBoxA(NULL, message.c_str(), caption.c_str(), MB_OK);
			return true;
		}

		static void HideConsole()
		{
			FreeConsole();
		}
	};
}

#endif