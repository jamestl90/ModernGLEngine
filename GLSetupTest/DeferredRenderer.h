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
    class DeferredRenderer {
    public:
        DeferredRenderer(Graphics* graphics, RenderTargetManager* rtManager, ShaderManager* shaderManager, int width, int height);
        ~DeferredRenderer();

        void Initialize();
        void GBufferPass(Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void Resize(int width, int height);

    private:
        void SetupGBuffer();
        void TraverseSceneGraph(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void RenderNode(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

        Graphics* m_graphics;
        ShaderManager* m_shaderManager;
        RenderTargetManager* m_rtManager;

        int m_width, m_height;

        // Framebuffers and G-buffer
        RenderTarget* m_gBuffer;

        // Default geometry shader
        ShaderProgram* m_gBufferShader;
    };
}

#endif