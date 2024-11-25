#ifndef TEXTURE_READER_H
#define TEXTURE_READER_H

#include <string>
#include <vector>

#include "Types.h"

namespace JLEngine
{
    class TextureReader
    {
    public:
        TextureReader();
        ~TextureReader();

        bool ReadTexture(const std::string& texture, std::vector<unsigned char>& outData,
            int& outWidth, int& outHeight, int& outChannels);

    private:

    };
}

#endif