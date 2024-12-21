#include "GenerateBRDFLUT.h"

int main(int argc, char* argv[])
{
    std::string assetFolder = argv[1];

    return GenerateBRDFLUT(assetFolder);
}