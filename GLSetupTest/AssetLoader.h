
#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <tiny_gltf.h>

#include <filesystem>
#include <unordered_map>

#include "ResourceManager.h"
#include "Buffer.h"
#include "VertexBuffers.h"
#include "Material.h"

namespace JLEngine
{
	class Texture;
	class ShaderProgram;
	class RenderTarget;
	class ShaderStorageBuffer;
	class Mesh;
	class Node;
	class Graphics;

	struct TextureAttribute;
	
	enum class DepthType;

	enum class NormalGen
	{
		Smooth,
		Flat
	};

	struct AssetGenerationSettings
	{
		bool GenerateNormals = true; // if missing generate normals
		bool GenerateTangents = true; // if missing generate tangents
		NormalGen NormalGenType = NormalGen::Smooth; // type of normals to generate
	};

	class AssetLoader
	{
	public:
		AssetLoader(Graphics* graphics, ResourceManager<ShaderProgram>* shaderManager, ResourceManager<Mesh>* meshManager,
			ResourceManager<Material>* materialManager, ResourceManager<Texture>* textureManager, ResourceManager<RenderTarget>* rtManager, 
			ResourceManager<ShaderStorageBuffer>* ssboManager);

		ResourceManager<ShaderProgram>* GetShaderManager()   const { return m_shaderManager;}
		ResourceManager<Texture>* GetTextureManager()        const { return m_textureManager; }
		ResourceManager<Mesh>* GetMeshManager()           const { return m_meshManager; }
		ResourceManager<Material>* GetMaterialManager()       const { return m_materialManager; }
		ResourceManager<RenderTarget>* GetRenderTargetManager()   const { return m_renderTargetManager; }
		ResourceManager<ShaderStorageBuffer>* GetShaderStorageManager()  const { return m_shaderStorageManager; }

		std::shared_ptr<Node> LoadGLB(const std::string& glbFile);
		void SetGlobalGenerationSettings(AssetGenerationSettings& settings) { m_settings = settings; }

		// Texture Loading ///////////////////////////////////
		Texture* CreateTextureFromFile(const std::string& name, const std::string& filename,
			bool clamped = false, bool mipmaps = false);
		Texture* CreateTextureFromData(const std::string& name, uint32 width, uint32 height, int channels, void* data,
			int internalFormat, int format, int dataType, bool clamped = false, bool mipmaps = false);
		Texture* CreateTextureFromData(const std::string& name, uint32 width, uint32 height, int channels, vector<unsigned char>& data,
			bool clamped = false, bool mipmaps = false);

		// Shader Loading ///////////////////////////////////
		ShaderProgram* CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
		ShaderProgram* CreateShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);

		void SetHotReloading(bool hotReload) { m_hotReload = hotReload; }
		void PollForChanges(float deltaTime);

		ShaderProgram* BasicUnlitShader();
		ShaderProgram* BasicLitShader();
		ShaderProgram* SolidColorShader();
	    
		// Shader Loading ///////////////////////////////////
		Material* CreateMaterial(const std::string& name);
		Material* GetDefaultMaterial() { return m_defaultMat; }

		// RenderTarget Loading ///////////////////////////////////
		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib,
			JLEngine::DepthType depthType, uint32 numSources);
		RenderTarget* CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs,
			JLEngine::DepthType depthType, uint32 numSources);

		// Mesh Loading ///////////////////////////////////
		Mesh* LoadMeshFromData(const std::string& name, VertexBuffer& vbo, IndexBuffer& ibo);
		Mesh* LoadMeshFromData(const std::string& name, VertexBuffer& vbo);
		Mesh* CreateEmptyMesh(const std::string& name);

		// SSBO Loading ///////////////////////////////////
		ShaderStorageBuffer* CreateSSBO(const std::string& name, size_t size);
		void UpdateSSBO(const std::string& name, const void* data, size_t size);
	protected:

		AssetGenerationSettings m_settings;
		Graphics* m_graphics;

		/* Managers */
		ResourceManager<Texture>* m_textureManager;
		ResourceManager<ShaderProgram>* m_shaderManager;
		ResourceManager<Mesh>* m_meshManager;
		ResourceManager<Material>* m_materialManager;
		ResourceManager<RenderTarget>* m_renderTargetManager;
		ResourceManager<ShaderStorageBuffer>* m_shaderStorageManager;

		/* Shader Manager Variables */
		std::unordered_map<std::string, std::filesystem::file_time_type> m_shaderTimestamps;
		bool m_hotReload;
		float m_pollTimeSeconds = 1.0;
		float m_accumTime = 0;

		ShaderProgram* m_basicUnlit;
		ShaderProgram* m_basicLit;
		ShaderProgram* m_solidColor;

		/*  Material Manager Variables */
		Material* m_defaultMat;



	};
}

#endif