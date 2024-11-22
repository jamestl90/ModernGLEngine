#ifndef TEXTURE_READER_H
#define TEXTURE_READER_H

#include <string>
#include <stb/stb_image.h>

#include "Types.h"

namespace JLEngine
{
	class TextureReader
	{
	public:
		TextureReader();

		~TextureReader();

		bool ReadTexture(const std::string& texture);

		void Clear();

		byte* GetData();

		const int& GetFormat() { return m_format; }

		const int& GetInternalFormat() { return m_internalFormat; }

		const int& GetWidth() { return m_width; }

		const int& GetHeight() { return m_height; }

	private:

		bool m_success;

		int m_width;

		int m_height;

		int m_format;

		int m_internalFormat;
	};
}

#endif