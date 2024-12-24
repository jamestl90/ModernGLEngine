#ifndef GL_GRAPHICS_H
#define GL_GRAPHICS_H

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Window.h"

#include "ShaderProgram.h"
#include "Texture.h"
#include "VertexBuffers.h"
#include "ViewFrustum.h"
#include "CollisionShapes.h"
#include "Types.h"
#include "RenderTarget.h"
#include "Node.h"
#include "ImageData.h"

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

#define GL_CHECK_ERROR() GLCheckError(__FILE__, __LINE__)

using std::string;

namespace JLEngine
{
	class Texture;
	class IWindow;
	class Mesh;
	class RenderTarget;
	class TextureReader;
	class Material;
	class InstanceBuffer;
	class Batch;

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

	class Graphics
	{
	public:

		 Graphics(Window* window);
		 ~Graphics();

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
		 void SwapBuffers();
		 void Clear(uint32_t flags);
		 void ClearColour(float x, float y, float z, float w);
		 void ClearColour();
				
		 // Shader
		 std::vector<std::tuple<std::string, int>> GetActiveUniforms(uint32_t programId);
		 void PrintActiveUniforms(uint32_t programId);
		 bool ShaderCompileErrorCheck(uint32_t id);
		 bool ShaderProgramLinkErrorCheck(uint32_t programid);

		 void CreateShaderFromFile(ShaderProgram* program);
		 void CreateShaderFromText(ShaderProgram* program, std::vector<std::string> shaderTexts);
		 void DisposeShader(ShaderProgram* program);

		 void SetUniform(uint32_t location, uint32_t x);
		 void SetUniform(uint32_t location, uint32_t x, uint32_t y);
		 void SetUniform(uint32_t location, uint32_t x, uint32_t y, uint32_t z);
		 void SetUniform(uint32_t location, uint32_t x, uint32_t y, uint32_t z, uint32_t w);
						
		 void SetUniform(uint32_t location, float x);
		 void SetUniform(uint32_t location, float x, float y);
		 void SetUniform(uint32_t location, float x, float y, float z);
		 void SetUniform(uint32_t location, float x, float y, float z, float w);
						
		 void SetUniform(uint32_t location, const glm::vec2& vec);
		 void SetUniform(uint32_t location, const glm::vec3& vec);
		 void SetUniform(uint32_t location, const glm::vec4& vec);
						
		 void SetUniform(uint32_t location, int count, bool transpose, const glm::mat2& mat);
		 void SetUniform(uint32_t location, int count, bool transpose, const glm::mat3& mat);
		 void SetUniform(uint32_t location, int count, bool transpose, const glm::mat4& mat);

		 void BindShader(uint32_t programId);
		 void UnbindShader();

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
		bool ResizeRenderTarget(RenderTarget* target, int newWidth, int newHeight);
		bool CreateRenderTarget(RenderTarget* target);
		void DisposeRenderTarget(RenderTarget* target);

		void DisposeTexture(Texture* texture);
		void CreateTexture(Texture* texture);
		uint32_t CreateTexture(ImageData& imgData, bool genMipmaps = false);
		void ReadTexture2D(uint32_t texId, ImageData& imageData, int width, int height, int channels, bool hdr, bool useFramebuffer);
		void ReadCubemap(uint32_t texId, int width, int height, int channels, bool hdr, std::array<ImageData, 6>& imgData, bool useFramebuffer = false);
		int CreateCubemap(std::array<ImageData, 6>& cubeFaceData);
		void CreateTextures(uint32_t count, uint32_t& id);
		void BindTexture(uint32_t target, uint32_t id);
		void DisposeTexture(uint32_t count, uint32_t* textures);
		void SetActiveTexture(uint32_t texunit);

		uint32_t GetInternalFormat(uint32_t texId, uint32_t texType, uint32_t texTarget);
		std::string InternalFormatToString(GLint internalFormat);

		// Buffer objects
		void CreateInstanceBuffer(InstanceBuffer& instancedBO, const std::vector<glm::mat4>& instanceTransforms);
		void DisposeInstanceBuffer(InstanceBuffer& instancedBO);
		void CreateBatch(Batch& batch);
		void DisposeBatch(Batch& batch);
		void CreateVertexBuffer(VertexBuffer& vbo);
		void CreateIndexBuffer(IndexBuffer& ibo);
		void DisposeVertexBuffer(VertexBuffer& vbo);
		void DisposeIndexBuffer(IndexBuffer& ibo);

		// FBO
		void CreateFrameBuffer(uint32_t count, uint32_t& id);
		void BindFrameBuffer(uint32_t id);
		void DisposeFrameBuffer(uint32_t count, uint32_t* fbo);
		void BindFrameBufferToTexture(uint32_t type, uint32_t attachment, uint32_t target, uint32_t id, int32_t level = 0);
		uint32_t CheckFrameBuffer();
		// RBO
		void CreateRenderBuffer(uint32_t count, uint32_t& id);
		void BindRenderBuffer(uint32_t id);
		void RenderBufferStorage(uint32_t type, uint32_t internalFormat, uint32_t width, uint32_t height);
		void BindFrameBufferToRenderbuffer(uint32_t type, uint32_t internalFormat, uint32_t target, uint32_t id);
		// VAO
		uint32_t CreateVertexArray();
		//void CreateVertexArray(uint32_t count, uint32_t& id);
		void BindVertexArray(uint32_t vaoID);
		// VBO
		void CreateBuffer(uint32_t count, uint32_t& id);
		void DisposeBuffer(uint32_t count, uint32_t* id);
		void BindBuffer(uint32_t buffType, uint32_t vboID);
		void SetBufferData(uint32_t buffType, ptrdiff_t size, void* data, uint32_t usage);

		void EnableVertexAttribArray(uint32_t pos);
		void DisableVertexAttribArray(uint32_t pos);

		void SetBufferSubData( uint32_t target, ptrdiff_t offset, int32_t size, void* data);
		void SetAttributePointer(uint32_t index, int32_t count, uint32_t datatype, bool normalise, int32_t size, void* offset);

		void* MapBufferData(uint32_t target, uint32_t access);
		bool UnmapBufferData(uint32_t target);

		// Render calls
		void BeginPrimitiveDraw();
		void RenderPrimitive(glm::mat4& mvp, uint32_t type, uint32_t shaderId = -1);
		void EndPrimitiveDraw();

		void DrawArrayBuffer(uint32_t drawMode, uint32_t first, uint32_t count);
		void DrawElementBuffer(uint32_t drawMode, int32_t count, uint32_t dataType, void* offset);
		void DrawBuffers(uint32_t count, uint32_t* targets);

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

		uint32_t m_activeShaderProgram;
		uint32_t m_activeTextureUnit;      
		std::vector<GLuint> m_boundTextures;

		glm::vec4 m_clearColour;

		Window* m_window;
		ViewFrustum* m_viewFrustum;

		glm::mat4 m_mvp;
	};
}

#endif