#ifndef TEXTURE_H
#define TEXTURE_H

#include "GraphicsResource.h"
#include "Graphics.h"

namespace JLEngine
{
	class Graphics;

	class Texture : public GraphicsResource
	{
	public:
		Texture(uint32 handle, string& name, string& path);

		Texture(uint32 handle, string& fileName);

		~Texture();	

		void Init(Graphics* graphics);

		void UnloadFromGraphics();

		void SetClampToEdge(bool flag) { m_clampToEdge = flag; }

		bool IsClamped() { return m_clampToEdge; }

		void SetTextureRepeat(float flag) { m_repeat = flag; }

		float GetRepeat() { return m_repeat; }

		void SetFormat(int format) { m_format = format; }

		int GetFormat() { return m_format; }

		void SetInternalFormat(int format) { m_internalFormat = format; }

		int GetInternalFormat() { return m_internalFormat; }

		void SetWidth(int width) { m_width = width; }

		int GetWidth() { return m_width; }

		void SetHeight(int height) { m_height = height; }

		int GetHeight() { return m_height; }

		uint32 GetTextureId() { return m_id; }

		void SetTextureId(uint32 id) { m_id = id; }

		void* GetData() { return m_data; }

		void SetData(void* data) { m_data = data; }

		void SetDataType(int dataType) { m_dataType = dataType; }

		int GetDataType() { return m_dataType; }

	private:

		Graphics* m_graphics;

		bool m_clampToEdge;
		float m_repeat;

		int m_width;
		int m_height;

		int m_format;
		int m_internalFormat;

		void* m_data;

		int m_dataType;

		uint32 m_id;
	};
}

#endif