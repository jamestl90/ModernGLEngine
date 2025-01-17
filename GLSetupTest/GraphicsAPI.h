#ifndef GL_GRAPHICS_API_H
#define GL_GRAPHICS_API_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"

#include "ShaderProgram.h"
#include "Texture.h"
#include "ViewFrustum.h"
#include "CollisionShapes.h"
#include "Types.h"
#include "Node.h"
#include "ImageData.h"

#include <glad/glad.h>

inline void* BUFFER_OFFSET(std::size_t i) 
{
	return reinterpret_cast<void*>(i);
}

constexpr bool ENABLE_GL_DEBUG =
#ifdef NDEBUG
	false; // Release build
#else
	true;  // Debug build
#endif

inline void GLCheckError(const char* file, int line)
{
	if constexpr (ENABLE_GL_DEBUG)
	{
		GLenum error;
		while ((error = glGetError()) != GL_NO_ERROR)
		{
			std::cerr << "[OpenGL Error] (" << error << ") "
				<< " in file: " << file
				<< " at line: " << line << std::endl;
		}
	}
}

typedef GLuint64(APIENTRY* PFNGLGETTEXTUREHANDLEARBPROC)(GLuint texture);
typedef void(APIENTRY* PFNGLMAKETEXTUREHANDLERESIDENTARBPROC)(GLuint64 handle);

extern PFNGLGETTEXTUREHANDLEARBPROC glGetTextureHandleARB;
extern PFNGLMAKETEXTUREHANDLERESIDENTARBPROC glMakeTextureHandleResidentARB;

#define GL_CHECK_ERROR() GLCheckError(__FILE__, __LINE__)

using std::string;

namespace JLEngine
{
	class Texture;
	class Cubemap;
	class IWindow;
	class Mesh;
	class RenderTarget;
	class Material;

	struct PrimitiveType
	{
		enum List
		{
			Octahedron,
			Sphere,
			Cone
		};
	};

	/**
	 * \class	Graphics.
	 *
	 * \brief	Facade of low level graphics calls to OpenGL (or possibly DirectX in the future).
	 * 			No need for abstract base class I won't be changing Graphics API's at runtime.
	 * 			
	 *			Low level graphics api facade.
	 *
	 * \author	James Leupe
	 */

	class GraphicsAPI
	{
	public:

		 GraphicsAPI(Window* window);
		 ~GraphicsAPI();

		 void Initialise();

		 void toggleDrawAABB() { m_drawAABB = !m_drawAABB; }

		 const string& GetVersionInfo()   { return m_versionInfo; }
		 const string& GetVendorInfo()    { return m_vendorInfo; }
		 const string& GetRendererInfo()  { return m_rendererInfo; }
		 const string& GetExtensionInfo() { return m_extensionInfo; }
		 const string& GetShaderInfo()    { return m_shaderInfo; }

		 const int32_t& GetNumMSAABuffers() { return m_MSAABuffers; }
		 const int32_t& GetNumMSAASamples() { return m_MSAASamples; }

		 ViewFrustum* GetViewFrustum()    { return m_viewFrustum; }
		 Window* GetWindow()			 { return m_window; }

		 glm::mat4& CalculateMVP(glm::mat4& modelMat, glm::mat4& projection, glm::mat4& view);

		 void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
		 void SetViewport(glm::ivec4& params);
		 void Clear(uint32_t flags);
		 void ClearColour(float x, float y, float z, float w);
		 void SyncCompute();
		 void SyncFramebuffer();
				
		 // Shader
		 std::vector<std::tuple<std::string, int>> GetActiveUniforms(uint32_t programId);
		 void PrintActiveUniforms(uint32_t programId);
		 bool ShaderCompileErrorCheck(uint32_t id, const std::string& shaderFileName);
		 bool ShaderProgramLinkErrorCheck(uint32_t programid, const std::string& shaderFileName);

		 // Template-based uniform setter
		 template <typename T>
		 void SetProgUniform(uint32_t progId, uint32_t location, const T& value);
		 template <>
		 void SetProgUniform<uint32_t>(uint32_t progId, uint32_t location, const uint32_t& value);
		 template <>
		 void SetProgUniform<float>(uint32_t progId, uint32_t location, const float& value);
		 template <>
		 void SetProgUniform<glm::vec2>(uint32_t progId, uint32_t location, const glm::vec2& value);
		 template <>
		 void SetProgUniform<glm::vec3>(uint32_t progId, uint32_t location, const glm::vec3& value);
		 template <>
		 void SetProgUniform<glm::vec4>(uint32_t progId, uint32_t location, const glm::vec4& value);
		 template <>
		 void SetProgUniform<glm::mat2>(uint32_t progId, uint32_t location, const glm::mat2& value);
		 template <>
		 void SetProgUniform<glm::mat3>(uint32_t progId, uint32_t location, const glm::mat3& value);
		 template <>
		 void SetProgUniform<glm::mat4>(uint32_t progId, uint32_t location, const glm::mat4& value);

		 template <typename T>
		 void SetUniform(uint32_t location, const T& value);
		 template <>
		 void SetUniform<uint32_t>(uint32_t location, const uint32_t& value);
		 template <>
		 void SetUniform<float>(uint32_t location, const float& value);
		 template <>
		 void SetUniform<glm::vec2>(uint32_t location, const glm::vec2& value);
		 template <>
		 void SetUniform<glm::vec3>(uint32_t location, const glm::vec3& value);
		 template <>
		 void SetUniform<glm::vec4>(uint32_t location, const glm::vec4& value);
		 template <>
		 void SetUniform<glm::mat2>(uint32_t location, const glm::mat2& value);
		 template <>
		 void SetUniform<glm::mat3>(uint32_t location, const glm::mat3& value);
		 template <>
		 void SetUniform<glm::mat4>(uint32_t location, const glm::mat4& value);

		 void BindShader(uint32_t programId);
		 void UnbindShader();
		 void DispatchCompute(uint32_t numGroupsX, uint32_t numGroupsY, uint32_t numGroupsZ);

		// State modifier
		void SetMSAA(bool flag);
		bool UsingMSAA() const;

		void Enable(uint32_t val);
		void Disable(uint32_t val);

		void SetDepthFunc(uint32_t val);
		void SetDepthMask(uint32_t val);

		void SetBlendFunc(uint32_t first, uint32_t second);
		void SetCullFace(uint32_t face);

		// Texture 
		void ReadTexture2D(uint32_t texId, ImageData& imageData, int width, int height, int channels, bool hdr, bool useFramebuffer);
		void ReadCubemap(uint32_t texId, int width, int height, int channels, bool hdr, std::array<ImageData, 6>& imgData, bool useFramebuffer = false);

		void CreateTextures(uint32_t target, uint32_t n, uint32_t* textures);
		void DeleteTexture(uint32_t count, uint32_t* textures);
		void BindTextures(uint32_t first, uint32_t count, const uint32_t* textures);
		void BindTextureUnit(uint32_t unit, uint32_t texture);
		void BindImageTexture(uint32_t unit, uint32_t texture, uint32_t level, bool layered, uint32_t layer, GLenum access, GLenum format);

		void TextureParameter(uint32_t texture, uint32_t pname, uint32_t value);

		uint32_t GetInternalFormat(uint32_t texId, uint32_t texType, uint32_t texTarget);
		std::string InternalFormatToString(GLint internalFormat);

		// FBO
		void BlitNamedFramebuffer(uint32_t readFrameBuffer, uint32_t drawFrameBuffer,
			uint32_t srcX0, uint32_t srcY0,
			uint32_t srcX1, uint32_t srcY1,
			uint32_t dstX0, uint32_t dstY0,
			uint32_t dstX1, uint32_t dstY1,
			GLbitfield mask, uint32_t filter);
		void CreateFrameBuffer(uint32_t count, uint32_t& id);
		void BindFrameBuffer(uint32_t id);
		void BindDrawBuffer(uint32_t id);
		void DisposeFrameBuffer(uint32_t count, uint32_t* fbo);
		void NamedFramebufferTexture(uint32_t target, uint32_t attachment, uint32_t texture, uint32_t level);
		bool FramebufferComplete(uint32_t fboID);
		// RBO
		void CreateRenderBuffer(uint32_t count, uint32_t& id);
		void BindRenderBuffer(uint32_t id);
		void RenderBufferStorage(uint32_t type, uint32_t internalFormat, uint32_t width, uint32_t height);
		void BindFrameBufferToRenderbuffer(uint32_t type, uint32_t internalFormat, uint32_t target, uint32_t id);
		// VAO
		uint32_t CreateVertexArray();
		void BindVertexArray(uint32_t vaoID);
		void DeleteVertexArray(uint32_t vaoID);
		// VBO
		void CreateNamedBuffer(uint32_t& id);
		void BindBufferBase(GLenum target, uint32_t index, uint32_t bufferID);
		void BindBufferRange(uint32_t bufferID, GLenum target, uint32_t index, size_t offset, size_t size);
		void NamedBufferStorage(uint32_t id, size_t size, GLbitfield usageFlags, const void* data = nullptr);
		void NamedBufferSubData(uint32_t id, size_t offset, size_t size, const void* data);
		void CopyNamedBufferSubData(GLuint readBuffer,
			GLuint writeBuffer,
			GLintptr readOffset,
			GLintptr writeOffset,
			GLsizeiptr size);
		void* MapNamedBuffer(uint32_t id, GLbitfield access);
		void UnmapNamedBuffer(uint32_t id);
		void BindBuffer(uint32_t buffType, uint32_t boID);
		void DisposeBuffer(uint32_t count, uint32_t* id);

		// deprecated
		void BindTexture(uint32_t target, uint32_t id);
		void SetActiveTexture(uint32_t texunit);
		void BindFrameBufferToTexture(uint32_t type, uint32_t attachment, uint32_t target, uint32_t id, int32_t level = 0);

		// Render calls
		void BeginPrimitiveDraw();
		void RenderPrimitive(glm::mat4& mvp, uint32_t type, uint32_t shaderId = -1);
		void EndPrimitiveDraw();

		void DrawArrayBuffer(uint32_t drawMode, uint32_t first, uint32_t count);
		void DrawElementBuffer(uint32_t drawMode, int32_t count, uint32_t dataType, void* offset);
		void DrawBuffers(uint32_t count, uint32_t* targets);
		void MultiDrawElementsIndirect(uint32_t mode, uint32_t type, const void* indirect, uint32_t drawCount, uint32_t stride);

		void GeneratePrimitives();
		void DumpInfo() const;

	private:

		void CreatePrimitiveBuffers(float vertices[], uint32_t vertSize, uint32_t indices[], uint32_t indSize, uint32_t ids[]);
		void CreatePrimitiveBuffers(float vertices[], uint32_t vertSize, uint32_t ids[]);

		string m_shaderInfo;
		string m_versionInfo;
		string m_vendorInfo;
		string m_rendererInfo;
		string m_extensionInfo;

		int32_t m_MSAABuffers;
		int32_t m_MSAASamples;

		bool m_drawAABB;
		bool m_usingMSAA;

		uint32_t m_coneGeom[4];
		uint32_t m_octahedronGeom[4];
		uint32_t m_defaultShader;

		uint32_t m_boundShader = 0;
		uint32_t m_boundTexture = 0;
		uint32_t m_boundFramebuffer = 0;
		uint32_t m_boundDrawBuffer = 0;

		glm::vec4 m_clearColour;
		std::array<uint32_t, 4> m_viewPort;

		Window* m_window;
		ViewFrustum* m_viewFrustum;

		glm::mat4 m_mvp;
	};
}

#endif