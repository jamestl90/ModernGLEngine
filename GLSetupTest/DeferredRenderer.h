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
#include "CameraInfo.h"

namespace JLEngine
{
    enum DebugModes
    {
        GBuffer,
        DirectionalLightShadows,
        HDRISkyTextures,
        None
    };

    struct VAOResource 
    {
        std::shared_ptr<VertexArrayObject> vao;       
        std::shared_ptr<IndirectDrawBuffer> drawBuffer; 
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

        void AddVAO(bool isStatic, VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao);        
        void AddVAOs(bool isStatic, std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& vaos);

        void GenerateGPUBuffers(Node* sceneRoot);

        void SetDebugMode(DebugModes mode) { m_debugModes = mode; }
        void CycleDebugMode();

        HDRISky* GetHDRISky() { return m_hdriSky; }

    private:
        void DrawUI();        
        void DrawGeometry(const VAOResource& vaoResource, uint32_t stride);
        void LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix);
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

        RenderTargetPool m_rtPool;
        RenderTarget* m_gBufferTarget;

        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_lightingTestShader;         
        ShaderProgram* m_gBufferDebugShader;
        ShaderProgram* m_shadowDebugShader;
        ShaderProgram* m_debugTextureShader;

        VertexArrayObject m_triangleVAO;

        UniformBuffer m_cameraUBO;
        ShaderStorageBuffer<PerDrawData> m_ssboStaticPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboDynamicPerDraw;
        ShaderStorageBuffer<MaterialGPU> m_ssboMaterials;

        std::unordered_map<VertexAttribKey, VAOResource> m_staticResources;
        std::unordered_map<VertexAttribKey, VAOResource> m_dynamicResources;

        DirectionalLightShadowMap* m_dlShadowMap;
        Light m_directionalLight;
        bool m_enableDLShadows;

        HDRISky* m_hdriSky;
    };
}

#endif