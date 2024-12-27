#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GraphicsAPI.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "VertexArrayObject.h"
#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"
#include "Batch.h"

using RenderGroupKey = std::pair<int, JLEngine::VertexAttribKey>;
using RenderGroupValue = std::pair<JLEngine::Batch*, glm::mat4>;
using RenderGroupMap = std::map<RenderGroupKey, std::vector<RenderGroupValue>>;

namespace JLEngine
{
    enum DebugModes
    {
        GBuffer,
        DirectionalLightShadows,
        HDRISkyTextures,
        None
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
        
        void RenderIndirect(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

        const Light& GetDirectionalLight() const { return m_directionalLight; }
        bool GetDLShadowsEnabled() { return m_enableDLShadows; }
        void SetDirectionalShadowDistance(bool value) { m_enableDLShadows = value; }

        void AddStaticVAO(VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao) { m_staticVAOs[key] = vao; }

        void GenerateGPUBuffers(Node* sceneRoot);

        void SetDebugMode(DebugModes mode) { m_debugModes = mode; }
        void CycleDebugMode();

    private:
        void DrawUI();
        void BindTexture(ShaderProgram* shader, const std::string& uniformName, const std::string& flagName, Texture* texture, int textureUnit);
        //void SkyboxPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix);
        void DebugGBuffer(int debugMode);
        void DebugDirectionalLightShadows();
        void DebugHDRISky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void GBufferPass(RenderGroupMap& renderGroups, Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetupGBuffer();
        glm::mat4 DirectionalShadowMapPass(RenderGroupMap& renderGroups, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetUniformsForGBuffer(Material* mat);
        void RenderGroups(const RenderGroupMap& renderGroups);
        void GroupRenderables(Node* node, RenderGroupMap& renderGroups);
        void InitScreenSpaceTriangle();
        void RenderScreenSpaceTriangle();
        glm::mat4 GetLightMatrix(glm::vec3& lightPos, glm::vec3& lightDir, float size, float near, float far);
        void GenerateMaterialAndTextureBuffers(
            ResourceManager<JLEngine::Material>& materialManager,
            ResourceManager<JLEngine::Texture>& textureManager,
            std::vector<MaterialGPU>& materialBuffer,
            std::vector<TextureGPU>& textureBuffer,
            std::unordered_map<uint32_t, size_t>& materialIDMap);
        void GenerateDefaultTextures();

        GraphicsAPI* m_graphics;
        ResourceLoader* m_resourceLoader;

        DebugModes m_debugModes;

        int m_width, m_height;
        std::string m_assetFolder;

        RenderTarget* m_gBufferTarget;

        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_lightingTestShader;         
        ShaderProgram* m_gBufferDebugShader;
        ShaderProgram* m_shadowDebugShader;
        ShaderProgram* m_debugTextureShader;

        VertexArrayObject m_triangleVAO;

        IndirectDrawBuffer m_staticBuffer;
        IndirectDrawBuffer m_dynamicBuffer;

        ShaderStorageBuffer<PerDrawData> m_ssboStaticPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboDynamicPerDraw;
        ShaderStorageBuffer<MaterialGPU> m_ssboMaterials;
        ShaderStorageBuffer<TextureGPU> m_ssboTextures;

        std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_staticVAOs;
        std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_dynamicVAOs;

        DirectionalLightShadowMap* m_dlShadowMap;
        Light m_directionalLight;
        bool m_enableDLShadows;

        HDRISky* m_hdriSky;
    };
}

#endif