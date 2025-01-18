#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GraphicsAPI.h"
#include "RenderTarget.h"
#include "RenderTargetPool.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "VertexArrayObject.h"
#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"
#include "GPUBuffer.h"
#include "CameraInfo.h"
#include "UniformBuffer.h"
#include "TexturePool.h"
#include "SceneManager.h"

namespace JLEngine
{
    enum DebugModes
    {
        GBuffer,
        DirectionalLightShadows,
        HDRISkyTextures,
        None
    };

    enum class VAOType
    {
        STATIC,
        DYNAMIC,
        JL_TRANSPARENT // added prefix due to conflict with a #define in another file
    };

    class Material;
    class DirectionalLightShadowMap;
    class HDRISky;

    class DeferredRenderer 
    {
    public:
        DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* resourceLoader,
            int width, int height, const std::string& assetFolder);
        ~DeferredRenderer();

        void Initialize();
        void Resize(int width, int height);
        
        void Render(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

        const Light& GetDirectionalLight() const { return m_directionalLight; }
        bool GetDLShadowsEnabled() const { return m_enableDLShadows; }
        void SetDirectionalShadowDistance(bool value) { m_enableDLShadows = value; }

        void AddVAO(VAOType vaoType, VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao);
        void AddVAOs(VAOType vaoType, std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& vaos);

        void GenerateGPUBuffers();

        std::shared_ptr<Node>& SceneRoot() { return m_sceneManager.GetRoot(); }

        void SetDebugMode(DebugModes mode) { m_debugModes = mode; }
        void CycleDebugMode();

        HDRISky* GetHDRISky() { return m_hdriSky; }

    private:
        void DrawUI();        
        void DrawGeometry(const VAOResource& vaoResource, uint32_t stride);
        void LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix);
        void TransparencyPass(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void RenderBlended(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void RenderTransmissive(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void DebugPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DebugGBuffer(int debugMode);
        void DebugDirectionalLightShadows();
        void DebugHDRISky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void GBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetupGBuffer();
        glm::mat4 DirectionalShadowMapPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void InitScreenSpaceTriangle();
        void RenderScreenSpaceTriangle();
        glm::mat4 GetLightMatrix(glm::vec3& lightPos, glm::vec3& lightDir, float size, float near, float far);
        void GenerateMaterialAndTextureBuffers(
            ResourceManager<JLEngine::Material>& materialManager,
            ResourceManager<JLEngine::Texture>& textureManager,
            std::vector<MaterialGPU>& materialBuffer,
            std::unordered_map<uint32_t, size_t>& materialIDMap);

        GraphicsAPI* m_graphics;
        ResourceLoader* m_resourceLoader;

        DebugModes m_debugModes;

        int m_width, m_height;
        std::string m_assetFolder;

        float m_specularIndirectFactor = 1.0f;
        float m_diffuseIndirectFactor = 1.0f;
        glm::vec3 m_lastEyePos;

        TexturePool m_texPool;
        RenderTargetPool m_rtPool;
        RenderTarget* m_gBufferTarget;
        RenderTarget* m_lightOutputTarget;
        RenderTarget* m_transparentTarget;

        // raster shaders
        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_lightingTestShader;         
        ShaderProgram* m_gBufferDebugShader;
        ShaderProgram* m_shadowDebugShader;
        ShaderProgram* m_passthroughShader;
        ShaderProgram* m_downsampleShader;
        ShaderProgram* m_blendShader;
        ShaderProgram* m_transmissionShader;
        ShaderProgram* m_composeFramebufferShader;

        // compute shaders
        ShaderProgram* m_simpleBlurCompute;

        VertexArrayObject m_triangleVAO;

        UniformBuffer m_cameraUBO;
        ShaderStorageBuffer<PerDrawData> m_ssboStaticPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboInstancedPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboDynamicPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboTransparentPerDraw;
        ShaderStorageBuffer<MaterialGPU> m_ssboMaterials;
        ShaderStorageBuffer<glm::mat4> m_ssboJointMatrices;

        std::unordered_map<VertexAttribKey, VAOResource> m_staticResources;
        std::unordered_map<VertexAttribKey, VAOResource> m_skinnedMeshResources;
        std::unordered_map<VertexAttribKey, VAOResource> m_transparentResources;

        std::unordered_map<uint32_t, size_t> m_materialIDMap;

        SceneManager m_sceneManager;

        DirectionalLightShadowMap* m_dlShadowMap;
        Light m_directionalLight;
        bool m_enableDLShadows;

        HDRISky* m_hdriSky;
    };
}

#endif