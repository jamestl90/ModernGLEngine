#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Texture.h"
#include "ShaderStorageBuffer.h"

namespace JLEngine
{
	class GraphicsAPI;
	class Texture;
	class Cubemap;
	class Window;
	class VertexArrayObject;
	class ShaderProgram;
	class RenderTarget;
	class IndexBuffer;
	class VertexBuffer;
	class IndirectDrawBuffer;
	class GraphicsBuffer;

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
		static void DisposeShader(ShaderProgram* program);

		static void CreateRenderTarget(RenderTarget* renderTarget);
		static void DisposeRenderTarget(RenderTarget* target);
		static void ResizeRenderTarget(RenderTarget* renderTarget, int w, int h);

		static void CreateIndirectDrawBuffer(IndirectDrawBuffer* idbo);

		template <typename T>
		static void CreateShaderStorageBuffer(ShaderStorageBuffer<T>* ssbo);

		static void DisposeGraphicsBuffer(GraphicsBuffer* idbo);

	protected:
		static GraphicsAPI* m_graphicsAPI;

		static void UploadCubemapFace(GLenum face, const ImageData& img, const TexParams& params, int width, int height);
		static void CreateVertexBuffer(VertexBuffer& vbo);
		static void CreateIndexBuffer(IndexBuffer& vbo);
	};

	template <typename T>
	void Graphics::CreateShaderStorageBuffer(ShaderStorageBuffer<T>* ssbo)
	{
		if (ssbo->GetGPUID() == 0)
		{
			GLuint id = 0;
			glGenBuffers(1, &id);
			ssbo->SetGPUID(id);
		}

		auto count = static_cast<int32_t>(ssbo->GetBuffer().size());
		auto size = count * sizeof(T);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo->GetGPUID());
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, ssbo->DrawType());
		glBufferData(GL_SHADER_STORAGE_BUFFER, size, ssbo->GetBuffer().data(), ssbo->DrawType());
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo->GetGPUID());
	}
}

#endif