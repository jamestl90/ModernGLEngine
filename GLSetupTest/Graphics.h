#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Texture.h"
#include "VertexBuffers.h"

namespace JLEngine
{
	class GraphicsAPI;
	class Texture;
	class Cubemap;
	class Window;
	class VertexArrayObject;
	class ShaderProgram;

	class Graphics
	{
	public:
		static void Initialise(Window* window);
		static GraphicsAPI* API() { return m_graphicsAPI; }
		static bool Alive();

		static void DisposeTexture(Texture* texture);
		static void DisposeCubemap(Cubemap* texture);
		static void CreateTexture(Texture* texture);
		static void CreateCubemap(Cubemap* cubemap);		

		static void CreateVertexArray(VertexArrayObject* vao);
		static void DisposeVertexArray(VertexArrayObject* vao);

		static void CreateShader(ShaderProgram* shader);

	protected:
		static GraphicsAPI* m_graphicsAPI;

		static void UploadCubemapFace(GLenum face, const ImageData& img, const TexParams& params, int width, int height);
		static void CreateVertexBuffer(VertexBuffer& vbo);
		static void CreateIndexBuffer(IndexBuffer& vbo);
	};
}

#endif