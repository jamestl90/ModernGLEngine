#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Texture.h"
#include "ShaderStorageBuffer.h"
#include "GPUBuffer.h"
#include "GraphicsAPI.h"

#include <stdexcept>

namespace JLEngine
{
	class Texture;
	class Cubemap;
	class Window;
	class VertexArrayObject;
	class ShaderProgram;
	class RenderTarget;
	class IndexBuffer;
	class VertexBuffer;
	class IndirectDrawBuffer;

	class Graphics
	{
	public:
		static void Initialise(Window* window);
		static GraphicsAPI* API() { return m_graphicsAPI; }
		static bool Alive();

		static void DisposeTexture(Texture* texture);
		static void DisposeCubemap(Cubemap* texture);
		static void CreateTexture(Texture* texture, bool makeBindless = true);
		static void CreateCubemap(Cubemap* cubemap);		

		static void CreateVertexArray(VertexArrayObject* vao);
		static void DisposeVertexArray(VertexArrayObject* vao);

		// Create a GPU buffer with initial data
		template <typename T>
		static void CreateGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data);
		// Create an empty GPU buffer
		static void CreateGPUBuffer(GPUBuffer& buffer);
		// Upload data to an existing GPU buffer
		template <typename T>
		static void UploadToGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data, uint32_t offset = 0);

		static void CreateShader(ShaderProgram* shader);
		static void DisposeShader(ShaderProgram* program);

		static void CreateRenderTarget(RenderTarget* renderTarget);
		static void DisposeRenderTarget(RenderTarget* target);
		static void ResizeRenderTarget(RenderTarget* renderTarget, int w, int h);

		static void CreateIndirectDrawBuffer(IndirectDrawBuffer* idbo);

		static void DisposeGraphicsBuffer(GPUBuffer* idbo);

	protected:
		static GraphicsAPI* m_graphicsAPI;

		static void UploadCubemapFace(GLenum face, const ImageData& img, const TexParams& params, int width, int height);
	};

	template <typename T>
	void Graphics::CreateGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data)
	{
 		GLuint id;
		API()->CreateNamedBuffer(id);
		if (data.size() == 0) return;

		API()->NamedBufferStorage(id, data.size() * sizeof(T), buffer.GetUsageFlags(), data.size() == 0 ? nullptr : data.data());
		buffer.SetGPUID(id);
		buffer.SetSize(data.size());
		buffer.ClearDirty();
	}

	template <typename T>
	void Graphics::UploadToGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data, uint32_t offset)
	{
		size_t dataSize = data.size() * sizeof(T);
		if (offset + dataSize > buffer.GetSize())
		{
			throw std::runtime_error("Data exceeds GPU buffer size.");
		}

		API()->NamedBufferSubData(buffer.GetGPUID(), data.data(), buffer.GetSize(), offset);
		buffer.ClearDirty();
	}
}

#endif