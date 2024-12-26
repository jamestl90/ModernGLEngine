#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <tiny_gltf.h>
#include <memory>
#include <filesystem>
#include <unordered_map>

#include "ResourceManager.h"
#include "Buffer.h"
#include "VertexBuffers.h"
#include "Material.h"
#include "TextureFactory.h"
#include "CubemapFactory.h"
#include "ShaderFactory.h"
#include "MaterialFactory.h"
#include "RenderTargetFactory.h"

namespace JLEngine
{
	class Texture;
	class ShaderProgram;
	class RenderTarget;
	class ShaderStorageBuffer;
	class Mesh;
	class Node;
	class GraphicsAPI;

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

	class ResourceLoader
	{
	public:
		ResourceLoader(GraphicsAPI* graphics);

		~ResourceLoader();

		ResourceManager<ShaderProgram>* GetShaderManager()   const { return m_shaderManager;}
		ResourceManager<Texture>* GetTextureManager()        const { return m_textureManager; }
		ResourceManager<Mesh>* GetMeshManager()           const { return m_meshManager; }
		ResourceManager<Material>* GetMaterialManager()       const { return m_materialManager; }
		ResourceManager<RenderTarget>* GetRenderTargetManager()   const { return m_renderTargetManager; }

		std::shared_ptr<Node> LoadGLB(const std::string& glbFile);		
		void SetGlobalGenerationSettings(AssetGenerationSettings& settings) { m_settings = settings; }

		// Texture Loading ///////////////////////////////////
		std::shared_ptr<Texture> CreateTexture(const std::string& name, const std::string& filePath, const TexParams& texParams = TexParams(), int outputChannels = 0);		
		std::shared_ptr<Texture> CreateTexture(const std::string& name, ImageData& imageData, const TexParams& texParams = TexParams());
		std::shared_ptr<Texture> CreateTexture(const std::string& name);
		void DeleteTexture(const std::string& name);

		std::shared_ptr<Cubemap> CreateCubemapFromFile(const std::string& name, std::array<std::string, 6> fileNames, std::string folderPath);

		// Shader Loading ///////////////////////////////////
		std::shared_ptr<ShaderProgram> CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
		std::shared_ptr<ShaderProgram> CreateShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);

		void SetHotReloading(bool hotReload) { m_hotReload = hotReload; }
		void PollForChanges(float deltaTime);

		ShaderProgram* BasicUnlitShader();
		ShaderProgram* BasicLitShader();
		ShaderProgram* SolidColorShader();
		ShaderProgram* ScreenSpaceQuadShader();

		// Material Loading ///////////////////////////////////
		std::shared_ptr<Material> CreateMaterial(const std::string& name);
		Material* GetDefaultMaterial() { return m_defaultMat; }

		// RenderTarget Loading ///////////////////////////////////
		std::shared_ptr<RenderTarget> CreateRenderTarget(const std::string& name, 
			int width, int height, std::vector<TextureAttribute>& texAttribs, 
			JLEngine::DepthType depthType, uint32_t numSources);
		
		// Mesh Loading ///////////////////////////////////
		Mesh* LoadMeshFromData(const std::string& name, VertexBuffer& vbo, IndexBuffer& ibo);
		Mesh* LoadMeshFromData(const std::string& name, VertexBuffer& vbo);
		Mesh* CreateMesh(const std::string& name);

		GraphicsAPI* GetGraphics() { return m_graphics; }
	protected:

		AssetGenerationSettings m_settings;
		GraphicsAPI* m_graphics;

		bool m_hotReload;

		/* Managers */
		ResourceManager<Texture>* m_textureManager;
		ResourceManager<ShaderProgram>* m_shaderManager;
		ResourceManager<Mesh>* m_meshManager;
		ResourceManager<Material>* m_materialManager;
		ResourceManager<RenderTarget>* m_renderTargetManager;
		ResourceManager<Cubemap>* m_cubemapManager;

		TextureFactory* m_textureFactory;
		CubemapFactory* m_cubemapFactory;
		ShaderFactory* m_shaderFactory;
		MaterialFactory* m_materialFactory;
		RenderTargetFactory* m_renderTargetfactory;

		ShaderProgram* m_basicUnlit = nullptr;
		ShaderProgram* m_basicLit = nullptr;
		ShaderProgram* m_solidColor = nullptr;
		ShaderProgram* m_screenSpaceQuad = nullptr;

		/*  Material Manager Variables */
		Material* m_defaultMat;
	};
}

#endif