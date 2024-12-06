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
#include "Transform3.h"
#include "RenderTarget.h"
#include "Node.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

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

		 const int32& GetNumMSAABuffers() { return m_MSAABuffers; }
		 const int32& GetNumMSAASamples() { return m_MSAASamples; }

		 ViewFrustum* GetViewFrustum()    { return m_viewFrustum; }
		 Window* GetWindow()			 { return m_window; }

		 glm::mat4& CalculateMVP(glm::mat4& modelMat, glm::mat4& projection, glm::mat4& view);

		 void SetViewport(uint32 x, uint32 y, uint32 width, uint32 height);
		 //void UpdateViewport();
		 void SwapBuffers();
		 void Clear(uint32 flags);
		 void ClearColour(float x, float y, float z, float w);
				
		 // Shader
		 std::vector<std::tuple<std::string, int>> GetActiveUniforms(uint32 programId);
		 void PrintActiveUniforms(uint32 programId);
		 bool ShaderCompileErrorCheck(uint32 id, bool compileCheck);

		 void CreateShaderFromFile(ShaderProgram* program);
		 void CreateShaderFromText(ShaderProgram* program, std::vector<std::string> shaderTexts);
		 void DisposeShader(ShaderProgram* program);

		 void SetUniform(uint32 location, uint32 x);
		 void SetUniform(uint32 location, uint32 x, uint32 y);
		 void SetUniform(uint32 location, uint32 x, uint32 y, uint32 z);
		 void SetUniform(uint32 location, uint32 x, uint32 y, uint32 z, uint32 w);
						
		 void SetUniform(uint32 location, float x);
		 void SetUniform(uint32 location, float x, float y);
		 void SetUniform(uint32 location, float x, float y, float z);
		 void SetUniform(uint32 location, float x, float y, float z, float w);
						
		 void SetUniform(uint32 location, const glm::vec2& vec);
		 void SetUniform(uint32 location, const glm::vec3& vec);
		 void SetUniform(uint32 location, const glm::vec4& vec);
						
		 void SetUniform(uint32 location, int count, bool transpose, const glm::mat2& mat);
		 void SetUniform(uint32 location, int count, bool transpose, const glm::mat3& mat);
		 void SetUniform(uint32 location, int count, bool transpose, const glm::mat4& mat);

		 void BindShader(uint32 programId);
		 void UnbindShader();

		// State modifier
		
		void SetMSAA(bool flag);
		bool UsingMSAA() const;

		void Enable(uint32 val);
		void Disable(uint32 val);

		void SetDepthFunc(uint32 val);
		void SetDepthMask(uint32 val);

		void SetBlendFunc(uint32 first, uint32 second);

		void SetCullFace(uint32 face);

		// Texture 
		
		bool CreateRenderTarget(RenderTarget* target);
		void DisposeRenderTarget(RenderTarget* target);

		void DisposeTexture(Texture* texture);
		void CreateTexture(Texture* texture);
		void CreateTextures(uint32 count, uint32& id);
		void BindTexture(uint32 target, uint32 id);
		void BindTexture(ShaderProgram* shader, const std::string& uniformName,
			const std::string& flagName, Texture* texture, int textureUnit);
		void DisposeTexture(uint32 count, uint32* textures);
		void SetActiveTexture(uint32 texunit);

		// Buffer objects
		
		void CreateInstanceBuffer(InstanceBuffer& instancedBO, const std::vector<glm::mat4>& instanceTransforms);
		void DisposeInstanceBuffer(InstanceBuffer& instancedBO);
		void CreateVertexBuffer(VertexBuffer& vbo);
		void CreateIndexBuffer(IndexBuffer& ibo);
		void DisposeVertexBuffer(VertexBuffer& vbo);
		void DisposeVertexBuffer(GLuint vao, VertexBuffer& vbo);
		void DisposeIndexBuffer(IndexBuffer& ibo);

		void CreateMesh(Mesh* mesh);
		void DisposeMesh(Mesh* mesh);

		// FBO
		void CreateFrameBuffer(uint32 count, uint32& id);
		void BindFrameBuffer(uint32 id);
		void DisposeFrameBuffer(uint32 count, uint32* fbo);
		void BindFrameBufferToTexture(uint32 type, uint32 attachment, uint32 target, uint32 id, int32 level = 0);
		uint32 CheckFrameBuffer();
		// RBO
		void CreateRenderBuffer(uint32 count, uint32& id);
		void BindRenderBuffer(uint32 id);
		void RenderBufferStorage(uint32 type, uint32 internalFormat, uint32 width, uint32 height);
		void BindFrameBufferToRenderbuffer(uint32 type, uint32 internalFormat, uint32 target, uint32 id);
		// VAO
		uint32 CreateVertexArray();
		void CreateVertexArray(uint32 count, uint32& id);
		void BindVertexArray(uint32 vaoID);
		// VBO
		void CreateBuffer(uint32 count, uint32& id);
		void DisposeBuffer(uint32 count, uint32* id);
		void BindBuffer(uint32 buffType, uint32 vboID);
		void SetBufferData(uint32 buffType, ptrdiff_t size, void* data, uint32 usage);

		void EnableVertexAttribArray(uint32 pos);
		void DisableVertexAttribArray(uint32 pos);

		void SetBufferSubData( uint32 target, ptrdiff_t offset, int32 size, void* data);
		void SetAttributePointer(uint32 index, int32 count, uint32 datatype, bool normalise, int32 size, void* offset);

		void* MapBufferData(uint32 target, uint32 access);
		bool UnmapBufferData(uint32 target);

		// Render calls

		void DrawAABB(AABB& aabb);

		void BeginPrimitiveDraw();
		void RenderPrimitive(glm::mat4& mvp, uint32 type, uint32 shaderId = -1);
		void EndPrimitiveDraw();

		void DrawArrayBuffer(uint32 drawMode, uint32 first, uint32 count);
		void DrawElementBuffer(uint32 drawMode, int32 count, uint32 dataType, void* offset);
		void DrawBuffers(uint32 count, uint32* targets);

		void GeneratePrimitives();

		void DumpInfo() const;

	private:

		void CreatePrimitiveBuffers(float vertices[], uint32 vertSize, uint32 indices[], uint32 indSize, uint32 ids[]);
		void CreatePrimitiveBuffers(float vertices[], uint32 vertSize, uint32 ids[]);

		string m_shaderInfo;
		string m_versionInfo;
		string m_vendorInfo;
		string m_rendererInfo;
		string m_extensionInfo;

		int32 m_MSAABuffers;
		int32 m_MSAASamples;

		bool m_drawAABB;
		bool m_usingMSAA;

		uint32 m_coneGeom[4];
		uint32 m_octahedronGeom[4];
		uint32 m_defaultShader;

		uint32 m_activeShaderProgram;
		uint32 m_activeTextureUnit;      
		std::vector<GLuint> m_boundTextures;

		glm::vec4 m_clearColour;

		Window* m_window;
		ViewFrustum* m_viewFrustum;

		glm::mat4 m_mvp;
	};
}

#endif