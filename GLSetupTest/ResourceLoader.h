#ifndef ASSET_LOADER_H
#define ASSET_LOADER_H

#include <tiny_gltf.h>
#include <memory>
#include <filesystem>
#include <unordered_map>
#include <typeindex>
#include <any>

#include "ResourceManager.h"
#include "VertexBuffers.h"
#include "Material.h"
#include "TextureFactory.h"
#include "CubemapFactory.h"
#include "ShaderFactory.h"
#include "MaterialFactory.h"
#include "RenderTargetFactory.h"
#include "ShaderStorageBuffer.h"
#include "MeshFactory.h"
#include "GLBLoader.h"
#include "AnimationFactory.h"

namespace JLEngine
{
	class Texture;
	class ShaderProgram;
	class RenderTarget;
	class Mesh;
	class Node;
	class GraphicsAPI;
	class VertexArrayObject;

	struct RTParams;
	
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
		ResourceManager<Animation>* GetAnimationManager() const { return m_animManager; }

		std::shared_ptr<Node> LoadGLB(const std::string& glbFile);		
		void SetGlobalGenerationSettings(AssetGenerationSettings& settings) { m_settings = settings; }

		// Texture Loading ///////////////////////////////////
		std::shared_ptr<Texture> CreateTexture(const std::string& name, const std::string& filePath, const TexParams& texParams, int outputChannels = 0);		
		std::shared_ptr<Texture> CreateTexture(const std::string& name, const std::string& filePath);
		std::shared_ptr<Texture> CreateTexture(const std::string& name, ImageData& imageData, const TexParams& texParams = TexParams());
		std::shared_ptr<Texture> CreateTextureEmpty(const std::string& name);
		void DeleteTexture(const std::string& name);

		std::shared_ptr<Cubemap> CreateCubemapFromFile(const std::string& name, std::array<std::string, 6> fileNames, std::string folderPath);

		// Shader Loading ///////////////////////////////////
		std::shared_ptr<ShaderProgram> CreateComputeFromFile(const std::string& name, const std::string& computeFile, const std::string& folderPath);
		std::shared_ptr<ShaderProgram> CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
		std::shared_ptr<ShaderProgram> CreateShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);
		void DeleteShader(const std::string& name);

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
			int width, int height, std::vector<RTParams>& texAttribs, 
			JLEngine::DepthType depthType, uint32_t numSources);
		std::shared_ptr<RenderTarget> CreateRenderTarget(const std::string& name,
			int width, int height,RTParams& texAttribs,
			JLEngine::DepthType depthType, uint32_t numSources);

		// Mesh Loading ///////////////////////////////////
		std::shared_ptr<Mesh> CreateMesh(const std::string& name);

		// Animation Loading //////////////////////////////
		std::shared_ptr<Animation> CreateAnimation(const std::string& name);

		GLBLoader* GetGLBLoader() { return m_glbLoader; }
		GraphicsAPI* GetGraphics() { return m_graphics; }

		template <typename T>
		static std::shared_ptr<T> Get(const std::string& name)
		{
			auto it = m_managers.find(typeid(T));
			if (it != m_managers.end())
			{
				// this feels real dirty
				auto mgr = std::any_cast<ResourceManager<T>*>(it->second);
				return mgr->Get(name);
			}
			return nullptr;
		}

		template <typename T>
		static std::shared_ptr<T> Get(uint32_t handle)
		{
			auto it = m_managers.find(typeid(T));
			if (it != m_managers.end())
			{
				// this feels real dirty
				auto mgr = std::any_cast<ResourceManager<T>*>(it->second);
				return mgr->Get(handle);
			}
			return nullptr;
		}

		static std::shared_ptr<Material> GetMat(Node* node, int index = 0)
		{
			auto it = m_managers.find(typeid(Material));
			if (it != m_managers.end())
			{
				// this feels real dirty
				auto mgr = std::any_cast<ResourceManager<Material>*>(it->second);
				return mgr->Get(node->mesh->GetSubmeshes().at(index).materialHandle);
			}
			return nullptr;
		}

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
		ResourceManager<Animation>* m_animManager;

		static std::unordered_map<std::type_index, std::any> m_managers;

		TextureFactory* m_textureFactory;
		CubemapFactory* m_cubemapFactory;
		ShaderFactory* m_shaderFactory;
		MaterialFactory* m_materialFactory;
		RenderTargetFactory* m_renderTargetfactory;
		MeshFactory* m_meshFactory;
		AnimationFactory* m_animFactory;

		ShaderProgram* m_basicUnlit = nullptr;
		ShaderProgram* m_basicLit = nullptr;
		ShaderProgram* m_solidColor = nullptr;
		ShaderProgram* m_screenSpaceQuad = nullptr;

		GLBLoader* m_glbLoader;

		/*  Material Manager Variables */
		Material* m_defaultMat;
	};
}

#endif