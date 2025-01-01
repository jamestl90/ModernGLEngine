#include "Graphics.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Window.h"
#include "VertexArrayObject.h"
#include "ShaderProgram.h"
#include "FileHelpers.h"
#include "VertexBuffers.h"
#include "IndirectDrawBuffer.h"
#include "GraphicsAPI.h"
#include "GPUResource.h"
#include "RenderTarget.h"

#include <sstream>
#include <stdexcept>

namespace JLEngine
{
	GraphicsAPI* Graphics::m_graphicsAPI = nullptr;

	void Graphics::Initialise(Window* window)	
	{
		m_graphicsAPI = new GraphicsAPI(window);
		m_graphicsAPI->Initialise();
	}

	bool Graphics::Alive()
	{
		return m_graphicsAPI != nullptr;
	}
	
	void Graphics::DisposeTexture(Texture* texture)
	{
		GLuint id = texture->GetGPUID();
		API()->DisposeTexture(1, &id);
	}

	void Graphics::DisposeCubemap(Cubemap* cubemap)
	{
		GLuint id = cubemap->GetGPUID();
		API()->DisposeTexture(1, &id);
	}

	void Graphics::CreateTexture(Texture* texture, bool makeBindless)
	{
		if (!texture)
		{
			throw std::runtime_error("Invalid texture!");
		}

		auto& imgData = texture->GetImageData();
		auto& params = texture->GetParams();

		if (imgData.width <= 0 || imgData.height <= 0)
		{
			std::cerr << "Graphics::CreateTexture: Dimensions cant be 0 " << texture->GetName() << std::endl;
			return;
		}

		GLuint image;
		glCreateTextures(GL_TEXTURE_2D, 1, &image);
		texture->SetGPUID(image);

		GLfloat anisotropy;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &anisotropy);
		glTextureParameterf(image, GL_TEXTURE_MAX_ANISOTROPY, anisotropy);

		if (imgData.hdrData.empty() && imgData.data.empty())
		{
			throw std::runtime_error("Graphics::CreateTexture: No valid texture data provided for " + texture->GetName());
		}

		GLuint mipLevels = params.mipmapEnabled
			? static_cast<int>(std::log2(std::max(imgData.width, imgData.height))) + 1 : 1;

		glTextureStorage2D(image, mipLevels, params.internalFormat, imgData.width, imgData.height);

		if (imgData.isHDR)
		{
			glTextureSubImage2D(image, 
				0, 0, 0, imgData.width, imgData.height, params.format, 
				params.dataType, imgData.hdrData.data());
		}
		else
		{
			glTextureSubImage2D(image,
				0, 0, 0, imgData.width, imgData.height, params.format,
				params.dataType, imgData.data.data());
		}

		uint32_t minFilter = params.minFilter;
		if (params.mipmapEnabled && (minFilter != GL_NEAREST_MIPMAP_NEAREST &&
			minFilter != GL_LINEAR_MIPMAP_NEAREST &&
			minFilter != GL_NEAREST_MIPMAP_LINEAR &&
			minFilter != GL_LINEAR_MIPMAP_LINEAR))
		{
			minFilter = GL_LINEAR_MIPMAP_LINEAR; // Default to trilinear filtering
		}

		glTextureParameteri(image, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(image, GL_TEXTURE_MIN_FILTER, minFilter);
		glTextureParameteri(image, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(image, GL_TEXTURE_WRAP_T, params.wrapT);

		if (params.mipmapEnabled)
		{
			glGenerateTextureMipmap(image); // Generate mipmaps for mutable textures
		}

		if (makeBindless)
		{
			GLuint64 bindlessHandle = glGetTextureHandleARB(texture->GetGPUID());
			if (bindlessHandle == 0)
			{
				std::cerr << "Error: glGetTextureHandleARB - handle = 0" << std::endl;
				throw std::runtime_error("Invalid handle");
			}
			glMakeTextureHandleResidentARB(bindlessHandle);
			texture->SetBindlessHandle(bindlessHandle);
		}

		// debug 
		glObjectLabel(GL_TEXTURE, image, -1, texture->GetName().c_str());
	}

	void Graphics::CreateCubemap(Cubemap* cubemap)
	{
		if (!cubemap)
		{
			throw std::runtime_error("Invalid cubemap! Null pointer received.");
		}

		auto& imgData = cubemap->GetImageData();
		auto& params = cubemap->GetParams();

		if (imgData.size() != 6)
		{
			throw std::runtime_error("Invalid cubemap! Requires 6 images, got " + std::to_string(imgData.size()));
		}

		auto width = imgData[0].width;
		auto height = imgData[0].height;
		if (width <= 0 || height <= 0)
		{
			throw std::runtime_error("Graphics::CreateCubemap: Invalid dimensions (" +
				std::to_string(width) + "x" + std::to_string(height) + ") for cubemap: " +
				cubemap->GetName());
		}

		uint32_t minFilter = params.minFilter;
		if (params.mipmapEnabled && (minFilter != GL_NEAREST_MIPMAP_NEAREST &&
			minFilter != GL_LINEAR_MIPMAP_NEAREST &&
			minFilter != GL_NEAREST_MIPMAP_LINEAR &&
			minFilter != GL_LINEAR_MIPMAP_LINEAR))
		{
			minFilter = GL_LINEAR_MIPMAP_LINEAR; 
		}

		// Generate texture ID
		GLuint textureId;
		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureId);
		cubemap->SetGPUID(textureId);

		GLuint mipLevels = params.mipmapEnabled ? static_cast<int>(std::log2(std::max(width, height))) + 1 : 1;
		glTextureStorage2D(textureId, mipLevels, params.internalFormat, width, height);

		// Upload cubemap data (for each face)
		for (unsigned int i = 0; i < 6; ++i)
		{
			auto& faceData = imgData.at(i);
			if (faceData.isHDR)
			{
				glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, params.format, params.dataType,
					faceData.hdrData.data());
			}
			else
			{
				glTextureSubImage3D(textureId, 0, 0, 0, i, width, height, 1, params.format, params.dataType,
					faceData.data.data());
			}
		}

		glTextureParameteri(textureId, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTextureParameteri(textureId, GL_TEXTURE_MIN_FILTER, minFilter);
		glTextureParameteri(textureId, GL_TEXTURE_WRAP_S, params.wrapS);
		glTextureParameteri(textureId, GL_TEXTURE_WRAP_T, params.wrapT);
		glTextureParameteri(textureId, GL_TEXTURE_WRAP_R, params.wrapR);

		if (params.mipmapEnabled)
		{
			glGenerateTextureMipmap(params.textureType); // Generate mipmaps for mutable textures
		}
	}

	void Graphics::CreateVertexArray(VertexArrayObject* vao)
	{
		uint32_t vaoID;
		glCreateVertexArrays(1, &vaoID);
		vao->SetGPUID(vaoID);

		auto& vbo = vao->GetVBO();
		CreateGPUBuffer<float>(vbo.GetGPUBuffer(), vbo.GetDataMutable());

		uint32_t stride = CalculateStride(vao);

		glVertexArrayVertexBuffer(vao->GetGPUID(), 0, vbo.GetGPUBuffer().GetGPUID(), 0, stride);

		auto vertexAttribKey = vao->GetAttribKey();
		uint32_t offset = 0;
		uint32_t index = 0;

		for (uint32_t i = 0; i < static_cast<uint32_t>(AttributeType::COUNT); ++i)
		{
			if (vertexAttribKey & (1 << i))
			{
				GLenum type = GL_FLOAT;
				GLsizei size = 0;

				switch (static_cast<AttributeType>(1 << i))
				{
				case AttributeType::POSITION:
					size = vao->GetPosCount();
					break;
				case AttributeType::NORMAL:
					size = 3;
					break;
				case AttributeType::TEX_COORD_0:
					size = 2;
					break;
				case AttributeType::TEX_COORD_1:
					size = 2;
					break;
				case AttributeType::COLOUR:
					size = 4;
					break;
				case AttributeType::TANGENT:
					size = 4;
					break;
				default:
					std::cerr << "Unsupported attribute type!" << std::endl;
					continue;
				}

				glEnableVertexArrayAttrib(vaoID, index);
				glVertexArrayAttribFormat(vaoID, index, size, GL_FLOAT, GL_FALSE, offset);
				glVertexArrayAttribBinding(vaoID, index, 0);				

				offset += size * sizeof(float);
				++index; 
			}
		}
		if (vao->HasIndices())
		{
			auto& ibo = vao->GetIBO();
			CreateGPUBuffer<uint32_t>(ibo.GetGPUBuffer(), ibo.GetDataImmutable());
			glVertexArrayElementBuffer(vaoID, ibo.GetGPUBuffer().GetGPUID());
		}
	}

	void Graphics::DisposeVertexArray(VertexArrayObject* vao)
	{
		auto id = vao->GetGPUID();
		glDeleteVertexArrays(1, &id);

		auto vboid = vao->GetVBO().GetGPUBuffer().GetGPUID();
		glDeleteBuffers(1, &vboid);

		auto iboid = vao->GetIBO().GetGPUBuffer().GetGPUID();
		if (iboid != 0)
		{
			glDeleteBuffers(1, &iboid);
		}
	}

	void Graphics::CreateShader(ShaderProgram* program)
	{
		auto& shaders = program->GetShaders();
		for (uint32_t i = 0; i < shaders.size(); i++)
		{
			Shader& s = shaders.at(i);

			GLuint shaderId = glCreateShader(s.GetType());
			s.SetShaderId(shaderId);

			std::string shaderFile = s.GetSource();
			const char* cStr = shaderFile.c_str();
			glShaderSource(shaderId, 1, &cStr, NULL);
			glCompileShader(shaderId);

			Graphics::API()->ShaderCompileErrorCheck(shaderId);
		}

		GLuint programID = glCreateProgram();
		program->SetProgramId(programID);

		for (uint32_t i = 0; i < shaders.size(); i++)
		{
			glAttachShader(programID, shaders.at(i).GetShaderId());
		}

		glLinkProgram(programID);

		if (!API()->ShaderProgramLinkErrorCheck(programID))
		{
			DisposeShader(program);
		}
		else
		{
			// another loop to delete :( not sure if i can delete the shader before linking, probably not
			for (uint32_t i = 0; i < shaders.size(); i++)
			{
				glDeleteShader(shaders.at(i).GetShaderId());
			}

			GL_CHECK_ERROR();
			auto activeUniforms = Graphics::API()->GetActiveUniforms(program->GetProgramId());
			program->ClearUniforms();
			for (auto& uniform : activeUniforms)
			{
				auto& name = std::get<0>(uniform);
				auto& loc = std::get<1>(uniform);
				program->SetActiveUniform(name, loc);
			}
		}
	}

	void Graphics::DisposeShader(ShaderProgram* program)
	{
		if (program == nullptr) return;

		auto& shaders = program->GetShaders();

		for (auto it = shaders.begin(); it != shaders.end(); it++)
		{
			glDetachShader(program->GetProgramId(), (*it).GetShaderId());
			glDeleteShader((*it).GetShaderId());
		}

		glDeleteProgram(program->GetProgramId());
	}

	void Graphics::Blit(RenderTarget* src, RenderTarget* dst, uint32_t bitfield, uint32_t filter)
	{
		auto srcId = src->GetFrameBufferId();
		auto dstId = dst->GetFrameBufferId();
		auto srcWidth = src->GetWidth();
		auto srcHeight = src->GetHeight();
		auto dstWidth = dst->GetWidth();
		auto dstHeight = dst->GetHeight();

		API()->BlitNamedFramebuffer(
			srcId, dstId, 
			0, 0, srcWidth, srcHeight, 
			0, 0, dstWidth, dstHeight, 
			bitfield, filter);
	}

	void Graphics::CreateRenderTarget(RenderTarget* target)
	{
		if (target->GetWidth() == 0 || target->GetHeight() == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions!" << std::endl;
		}

		GLuint fbo;
		glCreateFramebuffers(1, &fbo);
		target->SetFrameBufferId(fbo);

		AttachTextures(target);
		AttachDepth(target);

		auto& drawBuffers = target->GetDrawBuffers();
		glNamedFramebufferDrawBuffers(fbo, static_cast<GLsizei>(drawBuffers.size()), drawBuffers.data());

		if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
			std::ostringstream errorMsg;
			errorMsg << "Framebuffer (ID: " << fbo << ") not complete: ";
			switch (status)
			{
			case GL_FRAMEBUFFER_UNDEFINED: errorMsg << "GL_FRAMEBUFFER_UNDEFINED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: errorMsg << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: errorMsg << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: errorMsg << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: errorMsg << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
			case GL_FRAMEBUFFER_UNSUPPORTED: errorMsg << "GL_FRAMEBUFFER_UNSUPPORTED"; break;
			default: errorMsg << "Unknown Error"; break;
			}

			glDeleteFramebuffers(1, &fbo);
			for (uint32_t i = 0; i < target->GetNumTextures(); i++)
			{
				GLuint tex = target->GetTexId(i);
				glDeleteTextures(1, &tex);
			}
			if (target->DepthType() == DepthType::Renderbuffer)
			{
				GLuint dboId = target->GetDepthBufferId();
				glDeleteRenderbuffers(1, &dboId);
			}
			else if (target->DepthType() == DepthType::Texture)
			{
				GLuint dboId = target->GetDepthBufferId();
				glDeleteTextures(1, &dboId);
			}

			throw std::runtime_error(errorMsg.str());
		}
	}

	void Graphics::AttachTextures(RenderTarget* target)
	{
		auto fbo = target->GetFrameBufferId();
		auto& attributes = target->GetTextureAttributes();
		for (uint32_t i = 0; i < target->GetNumTextures(); ++i)
		{
			GLuint tex;
			glCreateTextures(GL_TEXTURE_2D, 1, &tex);

			const auto& attrib = attributes[i];
			if (target->IsMultisampled())
			{
				glTextureStorage2DMultisample(tex, target->GetSamples(), attrib.internalFormat,
					target->GetWidth(), target->GetHeight(), GL_TRUE);
			}
			else
			{
				glTextureStorage2D(tex, 1, attrib.internalFormat, target->GetWidth(), target->GetHeight());
			}

			glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, attrib.minFilter);
			glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, attrib.magFilter);
			glTextureParameteri(tex, GL_TEXTURE_WRAP_S, attrib.wrapS);
			glTextureParameteri(tex, GL_TEXTURE_WRAP_T, attrib.wrapT);

			glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0 + i, tex, 0);
			target->SetTexId(i, tex);
		}
	}

	void Graphics::AttachDepth(RenderTarget* target)
	{
		auto fbo = target->GetFrameBufferId();
		GLuint depth;
		if (target->DepthType() == DepthType::Renderbuffer || target->DepthType() == DepthType::DepthStencil)
		{
			glCreateRenderbuffers(1, &depth);

			if (target->IsMultisampled())
			{
				glNamedRenderbufferStorageMultisample(
					depth,
					target->GetSamples(),
					target->DepthType() == DepthType::DepthStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT32,
					target->GetWidth(),
					target->GetHeight()
				);
			}
			else
			{
				glNamedRenderbufferStorage(
					depth,
					target->DepthType() == DepthType::DepthStencil ? GL_DEPTH24_STENCIL8 : GL_DEPTH_COMPONENT32,
					target->GetWidth(),
					target->GetHeight()
				);
			}

			target->SetDepthId(depth);

			GLenum attachment = target->DepthType() == DepthType::DepthStencil ? GL_DEPTH_STENCIL_ATTACHMENT : GL_DEPTH_ATTACHMENT;
			glNamedFramebufferRenderbuffer(fbo, attachment, GL_RENDERBUFFER, depth);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			glCreateTextures(GL_TEXTURE_2D, 1, &depth);
			if (target->IsMultisampled())
			{
				glTextureStorage2DMultisample(depth, target->GetSamples(), GL_DEPTH_COMPONENT32,
					target->GetWidth(), target->GetHeight(), GL_TRUE);
			}
			else
			{
				glTextureStorage2D(depth, 1, GL_DEPTH_COMPONENT32, target->GetWidth(), target->GetHeight());
			}
			glTextureParameteri(depth, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(depth, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameteri(depth, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTextureParameteri(depth, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			target->SetDepthId(depth);
			glNamedFramebufferTexture(fbo, GL_DEPTH_ATTACHMENT, depth, 0);
		}
	}

	void Graphics::DisposeRenderTarget(RenderTarget* target)
	{
		GLuint fboId = target->GetFrameBufferId();

		glDeleteTextures(target->GetNumTextures(), target->GetTextures().data());
		glDeleteFramebuffers(1, &fboId);

		if (target->DepthType() == DepthType::Renderbuffer)
		{
			GLuint dboId = target->GetDepthBufferId();
			glDeleteRenderbuffers(1, &dboId);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			GLuint dboId = target->GetDepthBufferId();
			glDeleteTextures(1, &dboId);
		}
	}

	void Graphics::RecreateRenderTarget(RenderTarget* target, int newWidth, int newHeight)
	{
		if (newWidth == 0 || newHeight == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions for resizing!" << std::endl;
			return;
		}

		for (uint32_t i = 0; i < target->GetNumTextures(); i++)
		{
			GLuint tex = target->GetTexId(i);
			glDeleteTextures(1, &tex);
		}

		GLuint depthBuffer = target->GetDepthBufferId();
		if (depthBuffer)
		{
			if (target->DepthType() == DepthType::Renderbuffer || target->DepthType() == DepthType::DepthStencil)
			{
				glDeleteRenderbuffers(1, &depthBuffer);
			}
			else if (target->DepthType() == DepthType::Texture)
			{
				glDeleteTextures(1, &depthBuffer);
			}
		}

		target->SetWidth(newWidth);
		target->SetHeight(newHeight);

		// Resize textures for color attachments
		AttachTextures(target);
		AttachDepth(target);

		// Check framebuffer completeness
		uint32_t fbo = target->GetFrameBufferId();
		if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER);
			std::cerr << "Framebuffer resize failed: ";
			switch (status)
			{
			case GL_FRAMEBUFFER_UNDEFINED: std::cerr << "GL_FRAMEBUFFER_UNDEFINED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
			case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED"; break;
			default: std::cerr << "Unknown Error"; break;
			}
			std::cerr << std::endl;

			return;
		}
		GL_CHECK_ERROR();
		std::cout << "RenderTarget resized to " << newWidth << "x" << newHeight << std::endl;
	}

	void Graphics::BindGPUBuffer(GPUBuffer& buffer, int bindPoint)
	{
		glBindBufferBase(buffer.GetType(), bindPoint, buffer.GetGPUID());
	}

	void Graphics::CreateGPUBuffer(GPUBuffer& buffer)
	{
		GLuint id;
		API()->CreateNamedBuffer(id);
		API()->NamedBufferStorage(id, buffer.GetSize(), buffer.GetUsageFlags(), nullptr);
		buffer.SetGPUID(id);
		buffer.ClearDirty();
	}

	void Graphics::CreateIndirectDrawBuffer(IndirectDrawBuffer* idbo)
	{
		GPUBuffer& gpuBuffer = idbo->GetGPUBuffer();

		if (gpuBuffer.GetGPUID() == 0)
		{
			GLuint id = 0;
			glCreateBuffers(1, &id);
			gpuBuffer.SetGPUID(id);
		}

		const auto& bufferData = idbo->GetDataImmutable();
		size_t bufferSize = bufferData.size() * sizeof(DrawIndirectCommand);

		if (bufferSize == 0)
		{
			// Allocate storage with no initial data
			glNamedBufferStorage(gpuBuffer.GetGPUID(), 0, nullptr, gpuBuffer.GetUsageFlags());
		}
		else
		{
			if (gpuBuffer.GetSize() != bufferSize)
			{
				// Recreate storage if the size has changed
				glNamedBufferStorage(gpuBuffer.GetGPUID(), bufferSize, bufferData.data(), gpuBuffer.GetUsageFlags());
				gpuBuffer.SetSize(bufferSize);
			}
			else
			{
				// Update existing buffer data
				glNamedBufferSubData(gpuBuffer.GetGPUID(), 0, bufferSize, bufferData.data());
			}
		}

		gpuBuffer.ClearDirty();
	}

	void Graphics::DisposeGPUBuffer(GPUBuffer* gpuBuffer)
	{
		auto id = gpuBuffer->GetGPUID();
		glDeleteBuffers(1, &id);
	}
}