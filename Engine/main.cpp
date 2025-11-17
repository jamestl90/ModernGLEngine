#include "MainApp.h"

// hack to force dedicated gpu usage
// on my laptop without this code it picks integrated graphics by default
#ifdef _WIN32
#ifdef __cplusplus
extern "C" {
#endif

	__declspec(dllexport) uint32_t NvOptimusEnablement = 1;
	__declspec(dllexport) int  AmdPowerXpressRequestHighPerformance = 1;

#ifdef __cplusplus
}
#endif
#endif

int main(int argc, char* argv[])
{
    std::string assetFolder = argv[1];

    return MainApp(assetFolder);
}