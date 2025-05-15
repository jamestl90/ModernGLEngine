#include "Cubemap.h"
#include "FileHelpers.h"
#include "GraphicsAPI.h"
#include "Mesh.h"
#include "RenderTarget.h"
#include "TextureReader.h"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <set>

PFNGLGETTEXTUREHANDLEARBPROC glGetTextureHandleARB = nullptr;
PFNGLMAKETEXTUREHANDLERESIDENTARBPROC glMakeTextureHandleResidentARB = nullptr;

namespace JLEngine
{
	GraphicsAPI::GraphicsAPI(Window* window) 
		: m_shaderInfo(""), m_versionInfo(""), 
		m_vendorInfo(""), m_extensionInfo(""), m_rendererInfo(""), m_window(window), m_usingMSAA(false)
	{
		float fov = 45.0f;
		float nearDist = 0.1f;
		float farDist = 70.0f;

		m_viewFrustum = new ViewFrustum(fov, (float)window->GetWidth() / (float)window->GetHeight(), nearDist, farDist);
		m_viewPort = { 0, 0, 0, 0 };
		m_clearColour = glm::vec4(1.0f);

		Initialise();

		// get GLSL version
		const GLubyte* str = glGetString(GL_SHADING_LANGUAGE_VERSION);
		m_shaderInfo = "GLSL Version: " + string((char*)str);

		// get GL version
		str = glGetString(GL_VERSION);
		m_versionInfo = "GL Version: " + string((char*)str);

		// get vendor (AMD etc)
		str = glGetString(GL_VENDOR);
		m_vendorInfo = "Vendor: " + string((char*)str);

		// get renderer (graphics card?)
		str = glGetString(GL_RENDERER);
		m_rendererInfo = "Renderer: " + string((char*)str);

		glGetIntegerv(GL_SAMPLE_BUFFERS, &m_MSAABuffers);
		glGetIntegerv(GL_SAMPLES, &m_MSAASamples);

		// get list of extensions
		int numOfExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numOfExtensions);
		for (int i = 0; i < numOfExtensions; i++)
		{
			str = glGetStringi(GL_EXTENSIONS, i);
			m_extensionInfo += string((char*)str) + "\n";
		}

		GLint maxUnits;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);

		// enable bindless textures
		glGetTextureHandleARB = (PFNGLGETTEXTUREHANDLEARBPROC)glfwGetProcAddress("glGetTextureHandleARB");
		glMakeTextureHandleResidentARB = (PFNGLMAKETEXTUREHANDLERESIDENTARBPROC)glfwGetProcAddress("glMakeTextureHandleResidentARB");

		if (!glGetTextureHandleARB || !glMakeTextureHandleResidentARB) 
		{
			std::cerr << "Failed to load ARB_bindless_texture functions!" << std::endl;
		}
	}

	GraphicsAPI::~GraphicsAPI()
	{
		delete m_viewFrustum;
		glDeleteShader(m_defaultShader);

		glDeleteVertexArrays(1, &m_octahedronGeom[0]);
		glDeleteVertexArrays(1, &m_coneGeom[0]);
	}

	void GraphicsAPI::Initialise()
	{
		glViewport(0, 0, m_window->GetWidth(), m_window->GetHeight());
		glClearColor(m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w); 
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

		std::string defaultShaderV = 
			"#version 400\n" 
			"layout (location = 0) in vec3 in_Position;\n" 
			"uniform mat4 u_MVP;\n"
			//"out vec3 Colour;\n" 
			"void main()\n" 
			"{\n" 
			"gl_Position = u_MVP * vec4(in_Position, 1.0);\n" 
			//"Colour = in_Colour;\n"
			"}";
		std::string defaultShaderF = 
			"#version 400\n" 
			//"in vec3 Colour;\n" 
			"out vec4 FragColour;\n"
			"void main()\n"
			"{\n" 
			"FragColour = vec4(1.0, 0.0, 0.0, 1.0);\n" 
			"}";

		GLuint vertId = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragId = glCreateShader(GL_FRAGMENT_SHADER);

		const char* cStrVert = defaultShaderV.c_str();
		const char* cStrFrag = defaultShaderF.c_str();
		glShaderSource(vertId, 1, &cStrVert, NULL);
		glCompileShader(vertId);
		glShaderSource(fragId, 1, &cStrFrag, NULL);
		glCompileShader(fragId);
		ShaderCompileErrorCheck(vertId, "");
		ShaderCompileErrorCheck(fragId, "");

		m_defaultShader = glCreateProgram();

		glAttachShader(m_defaultShader, vertId);
		glAttachShader(m_defaultShader, fragId);
		glLinkProgram(m_defaultShader);
		ShaderProgramLinkErrorCheck(m_defaultShader, "");

		GeneratePrimitives();
	}

	void GraphicsAPI::ClearColour( float x, float y, float z, float w )
	{
		glClearColor(x, y, z, w);
		m_clearColour = glm::vec4(x, y, z, w);
	}

	void GraphicsAPI::SyncCompute()
	{
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
	}

	void GraphicsAPI::SyncShaderStorageBarrier()
	{
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
	}

	void GraphicsAPI::SyncFramebuffer()
	{
		glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT);
	}

	inline void GraphicsAPI::PrintVRAMUsage()
	{
		// I think this prints total vram used by GPU, not this specific app
		if (GL_NVX_gpu_memory_info) {
			GLint total_mem_kb = 0;
			glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_mem_kb);

			GLint available_mem_kb = 0;
			glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_mem_kb);

			GLint used_mem_kb = total_mem_kb - available_mem_kb;
			std::cout << "Used VRAM: " << used_mem_kb << " KB" << std::endl;
		}
		// amd: GL_ATI_meminfo
	}

	glm::mat4& GraphicsAPI::CalculateMVP(glm::mat4& modelMat, glm::mat4& projection, glm::mat4& view)
	{
		m_mvp = projection * view * modelMat;
		return m_mvp;
	}

	void GraphicsAPI::SetMSAA( bool flag )
	{
		m_usingMSAA = flag;
		if (flag)
		{
			glEnable(GL_MULTISAMPLE); 
		}
		else
		{
			glDisable(GL_MULTISAMPLE);
		}
	}

	bool GraphicsAPI::UsingMSAA() const
	{
		return m_usingMSAA;
	}

	bool GraphicsAPI::ShaderCompileErrorCheck(uint32_t id, const std::string& shaderFileName)
	{
		GLint compileStatus;
		glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);

		if (compileStatus == GL_FALSE)
		{
			int maxLength;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);
			GLchar* infoLog = new char[maxLength];
			glGetShaderInfoLog(id, maxLength, &maxLength, infoLog);
			std::cout << shaderFileName << " - Error:" << infoLog << std::endl;
			delete[] infoLog;
			return false;
		}
		return true;
	}

	bool GraphicsAPI::ShaderProgramLinkErrorCheck(uint32_t programId, const std::string& shaderFileName)
	{
		GLint linkStatus;
		glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

		if (linkStatus == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);

			if (maxLength > 0)
			{
				GLchar* infoLog = new GLchar[maxLength];
				glGetProgramInfoLog(programId, maxLength, &maxLength, infoLog);
				std::cout << shaderFileName << " - Error:" << infoLog << std::endl;
				delete[] infoLog;
			}
			else
			{
				std::cout << "ShaderInfoLog: (empty)" << std::endl;
			}
			return false;
		}

		return true;
	}

	template <>
	void GraphicsAPI::SetProgUniform<uint32_t>(uint32_t progId, uint32_t location, const uint32_t& value) {
		glProgramUniform1i(progId, location, value);
	}

	template <>
	void GraphicsAPI::SetProgUniform<float>(uint32_t progId, uint32_t location, const float& value) {
		glProgramUniform1f(progId, location, value);
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::vec2>(uint32_t progId, uint32_t location, const glm::vec2& value) {
		glProgramUniform2fv(progId, location, 1, glm::value_ptr(value));
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::vec3>(uint32_t progId, uint32_t location, const glm::vec3& value) {
		glProgramUniform3fv(progId, location, 1, glm::value_ptr(value));
	}

	template<>
	void GraphicsAPI::SetProgUniform(uint32_t progId, uint32_t location, const glm::ivec3& value)
	{
		glProgramUniform3iv(progId, location, 1, glm::value_ptr(value));
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::vec4>(uint32_t progId, uint32_t location, const glm::vec4& value) {
		glProgramUniform4fv(progId, location, 1, glm::value_ptr(value));
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::mat2>(uint32_t progId, uint32_t location, const glm::mat2& value) {
		glProgramUniformMatrix2fv(progId, location, 1, GL_FALSE, glm::value_ptr(value));
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::mat3>(uint32_t progId, uint32_t location, const glm::mat3& value) {
		glProgramUniformMatrix3fv(progId, location, 1, GL_FALSE, glm::value_ptr(value));
	}

	template <>
	void GraphicsAPI::SetProgUniform<glm::mat4>(uint32_t progId, uint32_t location, const glm::mat4& value) {
		glProgramUniformMatrix4fv(progId, location, 1, GL_FALSE, glm::value_ptr(value));
	}

	std::vector<std::tuple<std::string, int>> GraphicsAPI::GetActiveUniforms(uint32_t programId)
	{
		std::vector<std::tuple<std::string, int>> activeUniforms;
		GLint nUniforms, maxLen;
		glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
		glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &nUniforms);
		GLchar* name = new GLchar[maxLen];
		GLint size, location;
		GLsizei written;
		GLenum type;
		for (int i = 0; i < nUniforms; i++)
		{
			glGetActiveUniform(programId, i, maxLen, &written, &size, &type, name);
			location = glGetUniformLocation(programId, name);
			activeUniforms.push_back({ name, location });
		}
		return activeUniforms;
	}

	void GraphicsAPI::PrintActiveUniforms( uint32_t programId )
	{
		GLint nUniforms, maxLen;
		glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
		glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &nUniforms);
		GLchar* name = new GLchar[maxLen];
		GLint size, location;
		GLsizei written;
		GLenum type;
		std::cout << "Active uniforms" << std::endl;
		for (int i = 0; i < nUniforms; i++)
		{
			glGetActiveUniform(programId, i, maxLen, &written, &size, &type, name);
			location = glGetUniformLocation(programId, name);
			std::cout << location << " " << name << std::endl;
		}
	}

	uint32_t GraphicsAPI::CreateVertexArray()
	{
		GLuint id;
		glCreateVertexArrays(1, &id);
		return id;
	}

	void GraphicsAPI::BindFrameBuffer( uint32_t id )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void GraphicsAPI::BindDrawBuffer(uint32_t id)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, id);
	}

	void GraphicsAPI::BlitNamedFramebuffer(uint32_t readFrameBuffer, uint32_t drawFrameBuffer, uint32_t srcX0, uint32_t srcY0, uint32_t srcX1, uint32_t srcY1, uint32_t dstX0, uint32_t dstY0, uint32_t dstX1, uint32_t dstY1, GLbitfield mask, uint32_t filter)
	{
		glBlitNamedFramebuffer(readFrameBuffer, drawFrameBuffer, srcX0, srcY0,
			srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
	}

	void GraphicsAPI::CreateFrameBuffer(uint32_t count, uint32_t* id )
	{
		glCreateFramebuffers(count, id);
	}

	void GraphicsAPI::CreateRenderBuffer(uint32_t count, uint32_t* id)
	{
		glCreateRenderbuffers(count, id);
	}

	void GraphicsAPI::BindVertexArray( uint32_t vaoID )
	{
		glBindVertexArray(vaoID);
	}

	void GraphicsAPI::DeleteVertexArray(uint32_t vaoID)
	{
		glDeleteVertexArrays(1, &vaoID);
	}

	void GraphicsAPI::CreateNamedBuffer(uint32_t& id)
	{
		glCreateBuffers(1, &id);
	}

	void GraphicsAPI::BindBufferBase(GLenum target, uint32_t index, uint32_t bufferID)
	{
		glBindBufferBase(target, index, bufferID);
	}

	void GraphicsAPI::BindBufferRange(uint32_t bufferID, GLenum target, uint32_t index, size_t offset, size_t size)
	{
		glBindBufferRange(target, index, bufferID, offset, size);
	}

	void GraphicsAPI::NamedBufferStorage(uint32_t id, size_t size, GLbitfield usageFlags, const void* data)
	{
		glNamedBufferStorage(id, size, data, usageFlags);
	}

	void GraphicsAPI::NamedBufferData(uint32_t id, size_t size, const void* data, uint32_t drawFlag)
	{
		glNamedBufferData(id, size, data, drawFlag);
	}

	void GraphicsAPI::NamedBufferSubData(uint32_t id, size_t offset, size_t size, const void* data)
	{
		glNamedBufferSubData(id, offset, size, data);
	}

	void GraphicsAPI::CopyNamedBufferSubData(GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size)
	{
		glCopyNamedBufferSubData(readBuffer, writeBuffer, readOffset, writeOffset, size);
	}

	void* GraphicsAPI::MapNamedBuffer(uint32_t id, GLbitfield access)
	{
		auto mapped = glMapNamedBuffer(id, access);
		if (!mapped)
		{
			throw std::runtime_error("Failed to map named buffer");
		}
		return mapped;
	}

	void* GraphicsAPI::MapNamedBufferRange(uint32_t id, GLbitfield access, uint32_t offset, size_t length)
	{
		auto mapped = glMapNamedBufferRange(id, offset, length, access);
		if (!mapped)
		{
			throw std::runtime_error("Failed to map named buffer");
		}
		return mapped;
	}

	void GraphicsAPI::UnmapNamedBuffer(uint32_t id)
	{
		auto result = glUnmapNamedBuffer(id);
		if (!result)
		{
			throw std::runtime_error("Buffer data corruption detected during unmapping");
		}
	}

	void GraphicsAPI::BindBuffer( uint32_t buffType, uint32_t vboID )
	{
		glBindBuffer(buffType, vboID);
	}

	void GraphicsAPI::DrawArrayBuffer( uint32_t drawMode, uint32_t first, uint32_t count )
	{
		glDrawArrays(drawMode, first, count);
	}

	void GraphicsAPI::DrawElementBuffer( uint32_t drawMode, int32_t count, uint32_t dataType, void* offset )
	{
		glDrawElements(drawMode, count, dataType, offset);
	}

	void GraphicsAPI::BindShader(uint32_t programId)
	{
		if (m_boundShader != programId)
		{
			glUseProgram(programId);
			m_boundShader = programId;
		}
	}

	void GraphicsAPI::UnbindShader()
	{
		glUseProgram(0);
	}

	void GraphicsAPI::DispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ)
	{
		glDispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
	}

	void GraphicsAPI::BindTexture( uint32_t target, uint32_t id )
	{
		glBindTexture(target, id);
	}

	void GraphicsAPI::SetActiveTexture( uint32_t texunit )
	{
		glActiveTexture(GL_TEXTURE0 + texunit);
	}

	uint32_t GraphicsAPI::GetInternalFormat(uint32_t texId, uint32_t texType, uint32_t texTarget)
	{
		glBindTexture(texType, texId);

		GLint internalFormat = 0;
		glGetTexLevelParameteriv(texTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::cerr << "OpenGL Error: " << error << " in GetInternalFormat" << std::endl;
		}

		glBindTexture(texType, 0);
		return internalFormat;
	}

	std::string GraphicsAPI::InternalFormatToString(GLint internalFormat)
	{
		switch (internalFormat)
		{
			case GL_RGB8: return "GL_RGB8";
			case GL_RGBA8: return "GL_RGBA8";
			case GL_RGB16F: return "GL_RGB16F";
			case GL_RGBA16F: return "GL_RGBA16F";
			case GL_RGB32F: return "GL_RGB32F";
			case GL_RGBA32F: return "GL_RGBA32F";
			case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
			default: return "Unknown Format";
		}
	}

	/* UPDATE TO USE DSA IN FUTURE */
	void GraphicsAPI::ReadTexture2D(uint32_t texId, ImageData& imageData, int width, int height, int channels, bool hdr, bool useFramebuffer)
	{
		GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
		GLenum type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;

		imageData.isHDR = hdr;
		imageData.width = width;
		imageData.height = height;
		imageData.channels = channels;

		if (useFramebuffer)
		{
			// Use a framebuffer to read the texture
			GLuint fbo;
			glGenFramebuffers(1, &fbo);			
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				glDeleteFramebuffers(1, &fbo);
				throw std::runtime_error("Framebuffer is incomplete for Texture2D.");
			}

			// Allocate storage for pixel data
			size_t dataSize = imageData.width * imageData.height * imageData.channels;
			if (imageData.isHDR)
			{
				imageData.hdrData.resize(dataSize);
				glReadPixels(0, 0, imageData.width, imageData.height, format, type, imageData.hdrData.data());
			}
			else
			{
				imageData.data.resize(dataSize);
				glReadPixels(0, 0, imageData.width, imageData.height, format, type, imageData.data.data());
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &fbo);
		}
		else
		{
			// Use glGetTexImage to read the texture
			glBindTexture(GL_TEXTURE_2D, texId);

			//GLint internalFormat;
			//glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

			size_t dataSize = imageData.width * imageData.height * imageData.channels;
			if (hdr)
			{
				imageData.hdrData.resize(dataSize);
				glGetTexImage(GL_TEXTURE_2D, 0, format, type, imageData.hdrData.data());
			}
			else
			{
				imageData.data.resize(dataSize);
				glGetTexImage(GL_TEXTURE_2D, 0, format, type, imageData.data.data());
			}
		}

		GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			throw std::runtime_error("Failed to read Texture2D. GL Error: " + std::to_string(errorCode));
		}

		std::cout << "Successfully read Texture2D with " << (imageData.isHDR ? "HDR" : "LDR") << " data." << std::endl;
	}

	/* UPDATE TO USE DSA IN FUTURE */
	void GraphicsAPI::ReadCubemap(uint32_t texId, int width, int height, int channels, bool hdr, std::array<ImageData, 6>& imgData, bool useFramebuffer /* = false */)
	{
		// --- Argument Validation ---
		if (!glIsTexture(texId)) {
			throw std::runtime_error("ReadCubemap: Invalid texture ID provided: " + std::to_string(texId));
		}
		if (width <= 0 || height <= 0) {
			// Query texture dimensions if not provided or invalid?
			// glGetTextureLevelParameteriv(texId, 0, GL_TEXTURE_WIDTH, &width);
			// glGetTextureLevelParameteriv(texId, 0, GL_TEXTURE_HEIGHT, &height);
			// if (width <= 0 || height <= 0) // Check again after query
			throw std::runtime_error("ReadCubemap: Invalid dimensions provided: " + std::to_string(width) + "x" + std::to_string(height));
		}
		if (channels != 3 && channels != 4) {
			throw std::runtime_error("ReadCubemap: Unsupported number of channels: " + std::to_string(channels));
		}

		// --- Setup ---
		GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
		GLenum type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
		size_t pixelSize = channels * (hdr ? sizeof(float) : sizeof(unsigned char));
		size_t faceSizeBytes = (size_t)width * height * pixelSize;

		if (faceSizeBytes == 0) {
			throw std::runtime_error("ReadCubemap: Calculated face size is zero.");
		}

		// Save original viewport to restore later
		GLint originalViewport[4];
		glGetIntegerv(GL_VIEWPORT, originalViewport);

		// Save original pixel pack alignment
		GLint originalPackAlignment = 4; // Default
		glGetIntegerv(GL_PACK_ALIGNMENT, &originalPackAlignment);
		// Set alignment to 1 for tightly packed reading (common practice)
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		GL_CHECK_ERROR();

		// --- Reading Logic ---
		try {
			if (useFramebuffer) // Use Framebuffer + glReadPixels
			{
				GLuint fbo = 0;
				glCreateFramebuffers(1, &fbo);
				if (fbo == 0) throw std::runtime_error("ReadCubemap: Failed to create FBO.");
				glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo); // Bind FBO for reading

				for (int i = 0; i < 6; ++i) {
					// Prepare ImageData struct
					imgData.at(i).width = width;
					imgData.at(i).height = height;
					imgData.at(i).channels = channels;
					imgData.at(i).isHDR = hdr;

					// Attach the correct face and mip level (0) to the FBO
					// glNamedFramebufferTextureLayer is better, but requires FBO ID from glCreate*
					glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texId, 0);
					GL_CHECK_ERROR();

					// Check FBO completeness *after* attaching
					GLenum status = glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
					if (status != GL_FRAMEBUFFER_COMPLETE) {
						glDeleteFramebuffers(1, &fbo); // Clean up FBO
						glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
						throw std::runtime_error("ReadCubemap: Framebuffer incomplete for face " + std::to_string(i) + ". Status: " + std::to_string(status));
					}

					// Set the source buffer for reading
					glReadBuffer(GL_COLOR_ATTACHMENT0);
					GL_CHECK_ERROR();

					// Read pixels into the appropriate vector
					if (hdr) {
						std::vector<float> faceData(width * height * channels); // Allocate here
						glReadPixels(0, 0, width, height, format, type, faceData.data());
						imgData.at(i).hdrData = std::move(faceData);
					}
					else {
						std::vector<unsigned char> faceData(width * height * channels); // Allocate here
						glReadPixels(0, 0, width, height, format, type, faceData.data());
						imgData.at(i).data = std::move(faceData);
					}
					GL_CHECK_ERROR(); // Check after glReadPixels
				}
				glBindFramebuffer(GL_READ_FRAMEBUFFER, 0); // Unbind FBO
				glDeleteFramebuffers(1, &fbo); // Delete FBO
			}
			else // Use Direct Texture Access (glGetTextureSubImage)
			{
				// Determine required buffer size for one face
				GLsizei bufferSize = static_cast<GLsizei>(faceSizeBytes);

				for (int i = 0; i < 6; ++i) {
					// Prepare ImageData struct
					imgData.at(i).width = width;
					imgData.at(i).height = height;
					imgData.at(i).channels = channels;
					imgData.at(i).isHDR = hdr;

					// Read directly from the texture object using DSA
					if (hdr) {
						std::vector<float> faceData(width * height * channels); // Allocate here
						glGetTextureSubImage(texId, // Texture ID
							0,     // Mip Level
							0, 0, i, // x, y offset, z offset (z is face index for cube map array/layers) -> THIS IS WRONG FOR CUBEMAP
							width, height, 1, // width, height, depth (depth=1 for single face)
							format, type,
							bufferSize,
							faceData.data());
						// ** CORRECTION for Cubemap Faces with glGetTextureSubImage **
						// glGetTextureSubImage treats cubemaps like 3D textures or 2D Arrays.
						// We need to get the whole layer (face). glGetTextureImage is simpler here.
						// std::vector<float> faceData(width * height * channels); // Allocate here
						// glGetTextureImage(texId, // Texture ID
						//                   0,     // Mip Level
						//                   format, type,
						//                   bufferSize,
						//                   faceData.data()); // Reads the *entire* texture - need per face!

						// Let's stick to the non-DSA glGetTexImage for simplicity per face if DSA subimage is complex
						glBindTexture(GL_TEXTURE_CUBE_MAP, texId); // Need to bind for non-DSA version
						glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, type, faceData.data());
						glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // Unbind
						// *** END Correction ***

						imgData.at(i).hdrData = std::move(faceData);
					}
					else {
						// Similar logic for LDR data if needed, using glGetTexImage or corrected glGetTextureSubImage
						std::vector<unsigned char> faceData(width * height * channels); // Allocate here
						glBindTexture(GL_TEXTURE_CUBE_MAP, texId); // Bind for non-DSA version
						glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, type, faceData.data());
						glBindTexture(GL_TEXTURE_CUBE_MAP, 0); // Unbind
						imgData.at(i).data = std::move(faceData);
					}
					GL_CHECK_ERROR(); // Check after reading face
				}
			}
		}
		catch (const std::exception& e) {
			// Restore state even if reading fails
			glPixelStorei(GL_PACK_ALIGNMENT, originalPackAlignment);
			glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
			std::cerr << "ERROR during ReadCubemap data reading: " << e.what() << std::endl;
			throw; // Re-throw exception
		}

		// --- Restore State ---
		glPixelStorei(GL_PACK_ALIGNMENT, originalPackAlignment); // Restore original alignment
		glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]); // Restore viewport
		GL_CHECK_ERROR();

		std::cout << "Successfully read Cubemap with " << (imgData.at(0).isHDR ? "HDR" : "LDR") << " data (Method: " << (useFramebuffer ? "FBO/ReadPixels" : "Direct/GetTexImage") << ")." << std::endl;
	}

	void GraphicsAPI::CreateTextures(uint32_t target, uint32_t n, uint32_t* textures)
	{
		glCreateTextures(target, n, textures);
	}

	void GraphicsAPI::BindTextures(uint32_t first, uint32_t count, const uint32_t* textures)
	{
		glBindTextures(first, count, textures);
	}

	void GraphicsAPI::BindTextureUnit(uint32_t unit, uint32_t texture)
	{
		glBindTextureUnit(unit, texture);
	}

	void GraphicsAPI::BindImageTexture(uint32_t unit, uint32_t texture, uint32_t level, bool layered, uint32_t layer, GLenum access, GLenum format)
	{
		glBindImageTexture(unit, texture, level, layered, layer, access, format);
	}

	void GraphicsAPI::GenerateTextureMipmap(uint32_t texture)
	{
		glGenerateTextureMipmap(texture);
	}

	void GraphicsAPI::TextureStorage2D(uint32_t tex, int levels, uint32_t format, uint32_t width, uint32_t height)
	{
		glTextureStorage2D(tex, levels, format, width, height);
	}

	void GraphicsAPI::TextureStorage3D(uint32_t tex, int levels, uint32_t internalformat, uint32_t width, uint32_t height, uint32_t depth)
	{
		glTextureStorage3D(tex, levels, internalformat, width, height, depth);
	}

	void GraphicsAPI::TextureParameter(uint32_t texture, uint32_t pname, uint32_t value)
	{
		glTextureParameteri(texture, pname, value);
	}

	void GraphicsAPI::DeleteTexture(uint32_t count, uint32_t* textures)
	{
		glDeleteTextures(count, textures);
	}

	void GraphicsAPI::Clear(uint32_t flags)
	{
		glClear(flags);
	}

	void GraphicsAPI::CreateRenderBuffer(uint32_t count, uint32_t& id)
	{
		glCreateRenderbuffers(count, &id);
	}

	void GraphicsAPI::BindRenderBuffer(uint32_t id )
	{
		glBindRenderbuffer(GL_RENDERBUFFER, id);
	}

	void GraphicsAPI::BindFrameBufferToTexture(uint32_t fbo, uint32_t attachment, uint32_t texture, int32_t level)
	{
		glNamedFramebufferTexture(fbo, attachment, texture, level);
	}

	bool GraphicsAPI::FramebufferComplete(uint32_t fboID)
	{
		return glCheckNamedFramebufferStatus(fboID, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
	}

	void GraphicsAPI::BindFrameBufferToRenderbuffer(uint32_t fbo, uint32_t attachment, uint32_t target, uint32_t id)
	{
		glNamedFramebufferRenderbuffer(fbo, attachment, target, id);
	}

	void GraphicsAPI::RenderBufferStorage( uint32_t type, uint32_t internalFormat, uint32_t width, uint32_t height )
	{
		glRenderbufferStorage(type, internalFormat, width, height);
	}
	
	void GraphicsAPI::DrawBuffers( uint32_t count, uint32_t* targets)
	{
		glDrawBuffers(count, targets);
	}

	void GraphicsAPI::MultiDrawElementsIndirect(uint32_t mode, uint32_t type, const void* indirect, uint32_t drawCount, uint32_t stride)
	{
		glMultiDrawElementsIndirect(mode, type, indirect, drawCount, stride);
	}

	void GraphicsAPI::DisposeBuffer( uint32_t count, uint32_t* id )
	{
		glDeleteBuffers(count, id);
	}

	void GraphicsAPI::SetViewport( uint32_t x, uint32_t y, uint32_t width, uint32_t height )
	{
		// can uncomment this once i remove all raw glViewport calls
		//if (m_viewPort[0] != x || m_viewPort[1] != y || m_viewPort[2] != width || m_viewPort[3] != height)
		//{
			glViewport(x, y, width, height);
			m_viewPort[0] = x;
			m_viewPort[1] = y;
			m_viewPort[2] = width;
			m_viewPort[3] = height;
		//}
	}

	void GraphicsAPI::SetViewport(glm::ivec4& params)
	{
		glViewport(params.x, params.y, params.z, params.w);
	}

	void GraphicsAPI::DisposeFrameBuffer( uint32_t count, uint32_t* fbo )
	{
		glDeleteFramebuffers(count, fbo);
	}

	void GraphicsAPI::NamedFramebufferTexture(uint32_t target, uint32_t attachment, uint32_t texture, uint32_t level)
	{
		glNamedFramebufferTexture(target, attachment, texture, level);
	}

	void GraphicsAPI::NamedFramebufferTextureLayer(uint32_t fbo, GLenum attachment, GLuint texture, GLint level, GLint layer)
	{
		glNamedFramebufferTextureLayer(fbo, attachment, texture, level, layer);
	}

	void GraphicsAPI::GeneratePrimitives()
	{
		float coneVerts[13 * 3] = 
		{  
			0.f, 0.f, 0.f,
			0.f, 1.f, -1.f,   -0.5f, 0.866f, -1.f,   -0.866f, 0.5f, -1.f,
			-1.f, 0.f, -1.f,   -0.866f, -0.5f, -1.f,   -0.5f, -0.866f, -1.f,
			0.f, -1.f, -1.f,   0.5f, -0.866f, -1.f,   0.866f, -0.5f, -1.f,
			1.f, 0.f, -1.f,   0.866f, 0.5f, -1.f,   0.5f, 0.866f, -1.f,
		};
		uint32_t coneInds[22 * 3] = 
		{
			0, 1, 2,   0, 2, 3,   0, 3, 4,   0, 4, 5,   0, 5, 6,   0, 6, 7,
			0, 7, 8,   0, 8, 9,   0, 9, 10,   0, 10, 11,   0, 11, 12,   0, 12, 1,
			10, 6, 2,   10, 8, 6,   10, 9, 8,   8, 7, 6,   6, 4, 2,   6, 5, 4,   4, 3, 2,
			2, 12, 10,   2, 1, 12,   12, 11, 10
		};
	
		float octahedronVerts[18] = 
		{
			 0.0f,  1.0f,  0.0f,  
			 1.0f,  0.0f,  0.0f,  
			 0.0f,  0.0f,  1.0f,  
			 0.0f,  0.0f, -1.0f,  
			-1.0f,  0.0f,  0.0f,  
			 0.0f, -1.0f,  0.0f   
		};

		uint32_t octahedronInds[24] = 
		{
			0, 1, 2, 
			0, 2, 3,
			0, 3, 4, 
			0, 4, 1, 
			5, 2, 1, 
			5, 3, 2, 
			5, 4, 3, 
			5, 1, 4
		};

		CreatePrimitiveBuffers(octahedronVerts, sizeof(octahedronVerts), octahedronInds, sizeof(octahedronInds), m_octahedronGeom);
		CreatePrimitiveBuffers(coneVerts, sizeof(coneVerts), coneInds, sizeof(coneInds), m_coneGeom);
	}

	/* Needs deprecating/updating*/
	void GraphicsAPI::CreatePrimitiveBuffers( float vertices[], uint32_t vertSize, uint32_t indices[], uint32_t indSize, uint32_t ids[] )
	{
		glGenVertexArrays(1, &ids[0]);
		glBindVertexArray(ids[0]);
		glGenBuffers(1, &ids[1]);
		glBindBuffer(GL_ARRAY_BUFFER, ids[1]);
		glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 12, BUFFER_OFFSET(0));
		glGenBuffers(1, &ids[2]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize, indices, GL_STATIC_DRAW);
		ids[3] = indSize / sizeof(uint32_t);
		glBindVertexArray(0);
	}

	/* Needs deprecating/updating*/
	void GraphicsAPI::CreatePrimitiveBuffers(float vertices[], uint32_t vertSize, uint32_t ids[])
	{
		glGenVertexArrays(1, &ids[0]);
		glBindVertexArray(ids[0]);
		glGenBuffers(1, &ids[1]);
		glBindBuffer(GL_ARRAY_BUFFER, ids[1]);
		glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
	}

	void GraphicsAPI::SetBlendFunc( uint32_t first, uint32_t second )
	{
		glBlendFunc(first, second);
	}

	void GraphicsAPI::Enable( uint32_t val )
	{
		glEnable(val);
	}

	void GraphicsAPI::Disable( uint32_t val )
	{
		glDisable(val);
	}

	void GraphicsAPI::SetDepthFunc( uint32_t val )
	{
		glDepthFunc(val);
	}

	void GraphicsAPI::SetDepthMask( uint32_t val )
	{
		glDepthMask(val);
	}

	void GraphicsAPI::BeginPrimitiveDraw()
	{
		glUseProgram(m_defaultShader);
	}

	/* Probably replace/remove this */
	void GraphicsAPI::RenderPrimitive(glm::mat4& mvp, uint32_t type, uint32_t shaderId )
	{
		if (shaderId == -1)
		{	
			auto loc = glGetUniformLocation(m_defaultShader, "u_MVP");
			SetProgUniform<glm::mat4>(shaderId, loc, mvp);
		}
		else
		{
			auto loc = glGetUniformLocation(shaderId, "u_MVP");
			SetProgUniform<glm::mat4>(shaderId, loc, mvp);
		}

		switch (type)
		{
		case PrimitiveType::Cone: 
				glBindVertexArray(m_coneGeom[0]);
				glDrawElements(GL_TRIANGLES, m_coneGeom[3], GL_UNSIGNED_INT, BUFFER_OFFSET(0));
				glBindVertexArray(0);
			break;
		case PrimitiveType::Octahedron: 
				glBindVertexArray(m_octahedronGeom[0]);
				glDrawElements(GL_TRIANGLES, m_octahedronGeom[3], GL_UNSIGNED_INT, BUFFER_OFFSET(0));
				glBindVertexArray(0);
			break;
		case PrimitiveType::Sphere: break;
		default: break;
		}
	}

	void GraphicsAPI::EndPrimitiveDraw()
	{
		glUseProgram(0);
	}

	void GraphicsAPI::SetCullFace( uint32_t face )
	{
		glCullFace(face);
	}

	void GraphicsAPI::DumpInfo()
	{
		std::cout << "****************************************************" << std::endl;
		std::cout << m_shaderInfo << std::endl;
		std::cout << m_versionInfo << std::endl;
		std::cout << m_vendorInfo << std::endl;
		std::cout << m_rendererInfo << std::endl;
		std::cout << "GLFW Version: " << m_window->GetVersionMajor() 
			<< "." << m_window->GetVersionMinor() 
			<< "." << m_window->GetRevision() << std::endl;
		std::cout << "MSAA Buffers: " << m_MSAABuffers << std::endl;
		std::cout << "MSAA Samples: " << m_MSAASamples << std::endl;
		std::cout << "****************************************************" << std::endl;
	}
}
