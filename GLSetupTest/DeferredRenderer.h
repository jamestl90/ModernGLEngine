#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Graphics.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "VertexBuffers.h"
#include "AssetLoader.h"
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
        None
    };

    class Material;
    class DirectionalLightShadowMap;

    class DeferredRenderer 
    {
    public:
        DeferredRenderer(Graphics* graphics, AssetLoader* assetManager,
            int width, int height, const std::string& assetFolder);
        ~DeferredRenderer();

        void Initialize();
        void Resize(int width, int height);
        
        void Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

        const Light& GetDirectionalLight() const { return m_directionalLight; }
        void SetDirectionalShadowDistance(float dist) { m_dlShadowDistance = dist; }
        bool GetDLShadowsEnabled() { return m_enableDLShadows; }
        void SetDirectionalShadowDistance(bool value) { m_enableDLShadows = value; }

        void SetDebugMode(DebugModes mode) { m_debugModes = mode; }
        void CycleDebugMode();

    private:
        void SkyboxPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void TestLightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix);
        void DebugGBuffer(int debugMode);
        void DebugDirectionalLightShadows();
        void GBufferPass(RenderGroupMap& renderGroups, Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetupGBuffer();
        glm::mat4 DirectionalShadowMapPass(RenderGroupMap& renderGroups, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetUniformsForGBuffer(Material* mat);
        void RenderGroups(const RenderGroupMap& renderGroups);
        void GroupRenderables(Node* node, RenderGroupMap& renderGroups);
        void InitScreenSpaceTriangle();
        void RenderScreenSpaceTriangle();

        glm::mat4 GetLightMatrix(glm::vec3& lightPos, glm::vec3& lightDir, float size, float near, float far);

        Graphics* m_graphics;
        AssetLoader* m_assetLoader;

        DebugModes m_debugModes;

        int m_width, m_height;
        std::string m_assetFolder;

        RenderTarget* m_gBufferTarget;

        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_lightingTestShader;         
        ShaderProgram* m_gBufferDebugShader;
        ShaderProgram* m_textureDebugShader;

        VertexBuffer m_triangleVertexBuffer;
        GLuint m_triangleVAO;

        DirectionalLightShadowMap* m_dlShadowMap;
        float m_dlShadowDistance;
        Light m_directionalLight;
        bool m_enableDLShadows;

        Node* m_skybox;
        ShaderProgram* m_skyboxShader;
    };
}

#endif