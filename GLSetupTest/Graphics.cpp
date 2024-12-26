#include "Graphics.h"
#include "GraphicsAPI.h"
#include "Texture.h"
#include "Cubemap.h"
#include "Window.h"
#include "VertexArrayObject.h"
#include "ShaderProgram.h"
#include "FileHelpers.h"

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

	void Graphics::CreateTexture(Texture* texture)
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
		glGenTextures(1, &image);
		glBindTexture(params.textureType, image);

		uint32_t minFilter = params.minFilter;
		if (params.mipmapEnabled && (minFilter != GL_NEAREST_MIPMAP_NEAREST &&
			minFilter != GL_LINEAR_MIPMAP_NEAREST &&
			minFilter != GL_NEAREST_MIPMAP_LINEAR &&
			minFilter != GL_LINEAR_MIPMAP_LINEAR))
		{
			minFilter = GL_LINEAR_MIPMAP_LINEAR; // Default to trilinear filtering
		}

		glTexParameteri(params.textureType, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTexParameteri(params.textureType, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(params.textureType, GL_TEXTURE_WRAP_S, params.wrapS);
		glTexParameteri(params.textureType, GL_TEXTURE_WRAP_T, params.wrapT);

		if (imgData.hdrData.empty() && imgData.data.empty())
		{
			throw std::runtime_error("Graphics::CreateTexture: No valid texture data provided for " + texture->GetName());
		}

		if (params.immutable)
		{
			int mipLevels = params.mipmapEnabled ? static_cast<int>(std::log2(std::max(imgData.width, imgData.height)) + 1) : 1;
			glTexStorage2D(params.textureType, mipLevels,
				params.internalFormat, imgData.width, imgData.height);

			if (imgData.isHDR && !imgData.hdrData.empty())
			{
				glTexSubImage2D(params.textureType, 0, 0, 0, imgData.width, imgData.height,
					params.format, params.dataType, imgData.hdrData.data());
			}
			else if (!imgData.data.empty())
			{
				glTexSubImage2D(params.textureType, 0, 0, 0, imgData.width, imgData.height,
					params.format, params.dataType, imgData.data.data());
			}
		}
		else
		{
			if (imgData.isHDR && !imgData.hdrData.empty())
			{
				glTexImage2D(params.textureType, 0, params.internalFormat, imgData.width, imgData.height, 0,
					params.format, params.dataType, imgData.hdrData.data());
			}
			else if (!imgData.data.empty())
			{
				glTexImage2D(params.textureType, 0, params.internalFormat, imgData.width, imgData.height, 0,
					params.format, params.dataType, imgData.data.data());
			}
		}

		if (params.mipmapEnabled)
		{
			glGenerateMipmap(params.textureType); // Generate mipmaps for mutable textures
		}

		texture->SetGPUID(image);

		glBindTexture(params.textureType, 0);
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
			std::cerr << "Graphics::CreateCubemap: Invalid dimensions (" << width << "x" << height << ") for cubemap: "
				<< cubemap->GetName() << std::endl;
			return;
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
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureId);

		// Upload cubemap data (for each face)
		for (unsigned int i = 0; i < 6; ++i)
		{
			UploadCubemapFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, imgData.at(i), params, width, height);
		}

		glTexParameteri(params.textureType, GL_TEXTURE_MAG_FILTER, params.magFilter);
		glTexParameteri(params.textureType, GL_TEXTURE_MIN_FILTER, minFilter);
		glTexParameteri(params.textureType, GL_TEXTURE_WRAP_S, params.wrapS);
		glTexParameteri(params.textureType, GL_TEXTURE_WRAP_T, params.wrapT);
		glTexParameteri(params.textureType, GL_TEXTURE_WRAP_R, params.wrapR);

		cubemap->SetGPUID(textureId);

		if (params.mipmapEnabled)
		{
			glGenerateMipmap(params.textureType); // Generate mipmaps for mutable textures
		}

		glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
	}

	void Graphics::CreateVertexArray(VertexArrayObject* vao)
	{
		uint32_t vaoID;
		glCreateVertexArrays(1, &vaoID);
		glBindVertexArray(vaoID);
		vao->SetGPUID(vaoID);

		if (vao->GetVBO().GetId() != 0)
		{
			CreateVertexBuffer(vao->GetVBO());

			auto vertexAttribKey = vao->GetAttribKey();
			uint32_t offset = 0;
			uint32_t index = 0;
			uint32_t stride = CalculateStride(vertexAttribKey);

			for (uint32_t i = 0; i < static_cast<uint32_t>(AttributeType::COUNT); ++i)
			{
				if (vertexAttribKey & (1 << i))
				{
					GLenum type = GL_FLOAT;
					GLsizei size = 0;

					switch (static_cast<AttributeType>(1 << i))
					{
					case AttributeType::POSITION:
						size = 3;
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

					glEnableVertexAttribArray(index);
					glVertexAttribPointer(
						index,
						size,
						type,
						GL_FALSE,
						stride,
						BUFFER_OFFSET(offset)
					);

					offset += size * sizeof(float);
					++index; 
				}
			}
		}
		if (vao->GetIBO().GetId() != 0)
		{
			CreateIndexBuffer(vao->GetIBO());
		}
	}

	void Graphics::DisposeVertexArray(VertexArrayObject* vao)
	{
		auto id = vao->GetGPUID();
		glDeleteVertexArrays(1, &id);

		auto vboid = vao->GetVBO().GetId();
		glDeleteBuffers(1, &vboid);

		auto iboid = vao->GetIBO().GetId();
		if (iboid != 0)
		{
			glDeleteBuffers(1, &iboid);
		}
	}

	void Graphics::CreateShader(ShaderProgram* program)
	{
		auto shaders = program->GetShaders();
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
		}

		auto activeUniforms = Graphics::API()->GetActiveUniforms(program->GetProgramId());
		for (auto& uniform : activeUniforms)
		{
			auto& name = std::get<0>(uniform);
			auto& loc = std::get<1>(uniform);
			program->SetActiveUniform(name, loc);
		}
	}

	void Graphics::DisposeShader(ShaderProgram* program)
	{
		if (program == nullptr) return;

		auto shaders = program->GetShaders();

		for (auto it = shaders.begin(); it != shaders.end(); it++)
		{
			glDetachShader(program->GetProgramId(), (*it).GetShaderId());
			glDeleteShader((*it).GetShaderId());
		}

		glDeleteProgram(program->GetProgramId());
	}

	void Graphics::CreateRenderTarget(RenderTarget* target)
	{
		if (target->GetWidth() == 0 || target->GetHeight() == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions!" << std::endl;
		}

		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		target->SetFrameBufferId(fbo);

		auto& attributes = target->GetTextureAttributes();

		for (uint32_t i = 0; i < target->GetNumSources(); i++)
		{
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			auto& attrib = attributes[i];

			glTexImage2D(GL_TEXTURE_2D, 0, attrib.internalFormat, target->GetWidth(), target->GetHeight(), 0, attrib.format, attrib.dataType, nullptr);
			//glTexStorage2D(GL_TEXTURE_2D, 1, attrib.internalFormat, target->GetWidth(), target->GetHeight());
			// glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, attrib.internalFormat, target->GetWidth(), target->GetHeight(), GL_TRUE);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex, 0);
			target->SetSourceId(i, tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attrib.minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, attrib.magFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, attrib.wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, attrib.wrapT);
		}

		GLuint depth;
		if (target->DepthType() == DepthType::Renderbuffer)
		{
			glGenRenderbuffers(1, &depth);
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, target->GetWidth(), target->GetHeight());
			// glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, target->GetWidth(), target->GetHeight()); // 4x MSAA
			target->SetDepthId(depth);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, target->GetWidth(), target->GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			target->SetDepthId(depth);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		}

		glDrawBuffers(target->GetNumSources(), target->GetDrawBuffers().data());

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cerr << "Framebuffer not complete: ";
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

			glDeleteFramebuffers(1, &fbo);
			for (uint32_t i = 0; i < target->GetNumSources(); i++)
			{
				GLuint tex = target->GetSourceId(i);
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

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}

	void Graphics::DisposeRenderTarget(RenderTarget* target)
	{
		GLuint fboId = target->GetFrameBufferId();

		glDeleteTextures(target->GetNumSources(), target->GetSources().data());
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

	void Graphics::ResizeRenderTarget(RenderTarget* target, int newWidth, int newHeight)
	{
		if (newWidth == 0 || newHeight == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions for resizing!" << std::endl;
			return;
		}

		target->SetWidth(newWidth);
		target->SetHeight(newHeight);

		glBindFramebuffer(GL_FRAMEBUFFER, target->GetFrameBufferId());

		// Resize textures for color attachments
		auto& attributes = target->GetTextureAttributes();
		for (uint32_t i = 0; i < target->GetNumSources(); i++)
		{
			GLuint tex = target->GetSourceId(i);
			glBindTexture(GL_TEXTURE_2D, tex);

			auto& attrib = attributes[i];
			glTexImage2D(GL_TEXTURE_2D, 0, attrib.internalFormat, newWidth, newHeight, 0, attrib.format, attrib.dataType, nullptr);
		}

		// Resize depth buffer
		GLuint depth = target->GetDepthBufferId();
		if (target->DepthType() == DepthType::Renderbuffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		}

		// Check framebuffer completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
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
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		std::cout << "RenderTarget resized to " << newWidth << "x" << newHeight << std::endl;
	}

	void Graphics::UploadCubemapFace(GLenum face, const ImageData& img, const TexParams& params, int width, int height)
	{
		if (img.isHDR)
		{
			glTexImage2D(face, 0, params.internalFormat, width, height, 0, params.format, params.dataType, img.hdrData.data());
		}
		else
		{
			glTexImage2D(face, 0, params.internalFormat, width, height, 0, params.format, params.dataType, img.data.data());
		}
	}

	void Graphics::CreateVertexBuffer(VertexBuffer& vbo)
	{
		if (vbo.Uploaded()) return;

		uint32_t id;
		glGenBuffers(1, &id);
		glBindBuffer(vbo.Type(), id);
		glBufferData(vbo.Type(), vbo.SizeInBytes(), vbo.Array(), vbo.DrawType());
		vbo.SetId(id);
	}

	void Graphics::CreateIndexBuffer(IndexBuffer& ibo)
	{
		if (ibo.Uploaded()) return;

		uint32_t id;
		glGenBuffers(1, &id);
		glBindBuffer(ibo.Type(), id);
		glBufferData(ibo.Type(), ibo.Size() * sizeof(uint32_t), ibo.Array(), ibo.DrawType());
		ibo.SetId(id);
	}
}