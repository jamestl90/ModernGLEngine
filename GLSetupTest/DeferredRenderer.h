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
        
        void Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

        const Light& GetDirectionalLight() const { return m_directionalLight; }
        bool GetDLShadowsEnabled() { return m_enableDLShadows; }
        void SetDirectionalShadowDistance(bool value) { m_enableDLShadows = value; }

        void AddStaticVAO(VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao)
        {
            m_staticVAOs[key] = vao;
        }

        void GenerateGPUBuffers(Node* sceneRoot)
        {
            if (!sceneRoot) return;

            for (auto [vertexAttrib, vao] : m_staticVAOs)
            {
                Graphics::CreateVertexArray(vao.get());
            }

            for (auto [vertexAttrib, vao] : m_dynamicVAOs)
            {
                Graphics::CreateVertexArray(vao.get());
            }

            std::function<void(Node*)> traverseScene = [&](Node* node)
                {
                    if (!node) return;

                    if (node->GetTag() == NodeTag::Mesh)
                    {
                        const auto& submeshes = node->mesh->GetSubmeshes();
                        for (const auto& submesh : submeshes)
                        {
                            if (node->mesh->IsStatic())
                                m_staticBuffer.AddDrawCommand(submesh.command);
                            else
                                m_dynamicBuffer.AddDrawCommand(submesh.command);
                        }
                    }

                    for (const auto& child : node->children)
                    {
                        traverseScene(child.get());
                    }
                };
            traverseScene(sceneRoot);

            m_staticBuffer.SyncToGPU();
            m_dynamicBuffer.SyncToGPU();
        }

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

        std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_staticVAOs;
        std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>> m_dynamicVAOs;

        DirectionalLightShadowMap* m_dlShadowMap;
        Light m_directionalLight;
        bool m_enableDLShadows;

        HDRISky* m_hdriSky;
    };
}

#endif