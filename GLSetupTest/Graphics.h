#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "Texture.h"
#include "ShaderStorageBuffer.h"
#include "GPUBuffer.h"
#include "GraphicsAPI.h"

#include <stdexcept>
#include <algorithm>
#include <memory>
#undef max
#include <glm/common.hpp>

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

	struct VAOResource
	{
		std::shared_ptr<VertexArrayObject> vao;
		std::shared_ptr<IndirectDrawBuffer> drawBuffer;
	};

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
		// this will allocate storage so make sure the buffer's size is set
		static void CreateGPUBuffer(GPUBuffer& buffer);
		// Upload data to an existing GPU buffer
		template <typename T>
		static void UploadToGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data, uint32_t offset = 0);
		template <typename T>
		static void UploadToGPUBuffer(GPUBuffer& buffer, const T& data, uint32_t offset = 0);
		static void BindGPUBuffer(GPUBuffer& buffer, int bindPoint);
		static void DisposeGPUBuffer(GPUBuffer* idbo);

		static void Blit(RenderTarget* src, RenderTarget* dst, uint32_t bitfield = GL_COLOR_BUFFER_BIT, uint32_t filter = GL_NEAREST);
		static void BlitToDefault(RenderTarget* src, int destWidth, int destHeight, uint32_t bitfield = GL_COLOR_BUFFER_BIT, uint32_t filter = GL_NEAREST);

		static void CreateShader(ShaderProgram* shader);
		static void DisposeShader(ShaderProgram* program);

		static void CreateRenderTarget(RenderTarget* renderTarget);
		static void DeleteRenderTarget(RenderTarget* target);
		static void RecreateRenderTarget(RenderTarget* renderTarget, int w, int h);

		static void CreateIndirectDrawBuffer(IndirectDrawBuffer* idbo);

	protected:
		static void AttachDepth(RenderTarget* target);
		static void AttachTextures(RenderTarget* target);

		static void Resize(GPUBuffer& buffer, size_t oldSize, size_t newSize);

		static GraphicsAPI* m_graphicsAPI;
	};

	template <typename T>
	void Graphics::UploadToGPUBuffer(GPUBuffer& buffer, const T& data, uint32_t offset)
	{
		size_t dataSize = sizeof(T);
		if (offset + dataSize > buffer.GetSizeInBytes())
		{
			throw std::runtime_error("Data exceeds GPU buffer size.");
		}

		API()->NamedBufferSubData(buffer.GetGPUID(), offset, dataSize, &data);
		buffer.ClearDirty();
	}

	template <typename T>
	void Graphics::CreateGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data)
	{
		bool immutable = buffer.IsImmutable();
		if (immutable && data.size() == 0) return;

 		GLuint id;
		API()->CreateNamedBuffer(id);

		size_t bufferSize = data.size() * sizeof(T);
		const void* bufferData = data.empty() ? nullptr : data.data();

		if (immutable)
			API()->NamedBufferStorage(id, bufferSize, buffer.GetUsageFlags(), bufferData);
		else
			API()->NamedBufferData(id, bufferSize, bufferData, GL_DYNAMIC_DRAW);

		buffer.SetCreated(true);
		buffer.SetGPUID(id);
		buffer.SetSizeInBytes(bufferSize);
		buffer.ClearDirty();
	}

	template <typename T>
	void Graphics::UploadToGPUBuffer(GPUBuffer& buffer, const std::vector<T>& data, uint32_t offset)
	{
		size_t dataSize = data.size() * sizeof(T);

		// Ensure the buffer has enough capacity
		if (offset + dataSize > buffer.GetSizeInBytes())
		{
			size_t newSize = glm::max(offset + dataSize, buffer.GetSizeInBytes() * 2);

			uint32_t oldGPUID = buffer.GetGPUID(); 
			Resize(buffer, buffer.GetSizeInBytes(), newSize);

			API()->DisposeBuffer(1, &oldGPUID);
		}

		// Upload data to the buffer
		API()->NamedBufferSubData(buffer.GetGPUID(), offset, dataSize, data.data());
		buffer.ClearDirty();
	}
}

#endif