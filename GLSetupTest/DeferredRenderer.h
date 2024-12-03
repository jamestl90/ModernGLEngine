#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Graphics.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "ShaderManager.h"
#include "RenderTargetManager.h"
//#include "LightManager.h"

namespace JLEngine
{
    class DeferredRenderer 
    {
    public:
        DeferredRenderer(Graphics* graphics, RenderTargetManager* rtManager, ShaderManager* shaderManager, 
            int width, int height, const std::string& assetFolder);
        ~DeferredRenderer();

        void Initialize();
        void GBufferPass(Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void Resize(int width, int height);
        void DebugGBuffer(int debugMode);

    private:
        void SetupGBuffer();
        void TraverseSceneGraph(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void RenderNode(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetUniformsForGBuffer(Mesh* mesh, Material* mat);

        void InitScreenSpaceTriangle();
        void RenderScreenSpaceTriangle();

        Graphics* m_graphics;
        ShaderManager* m_shaderManager;
        RenderTargetManager* m_rtManager;

        int m_width, m_height;
        std::string m_assetFolder;

        // Framebuffers and G-buffer
        RenderTarget* m_gBufferTarget;

        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_gBufferDebugShader;

        GLuint m_triangleVAO;
        GLuint m_triangleVBO;
    };
}

#endif