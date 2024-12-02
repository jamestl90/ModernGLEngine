#include "DeferredRenderer.h"

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(Graphics* graphics, RenderTargetManager* rtManager, ShaderManager* shaderManager, int width, int height)
        : m_graphics(graphics), m_rtManager(rtManager), m_shaderManager(shaderManager), m_width(width), m_height(height),
        m_gBuffer(nullptr), m_gBufferShader(nullptr) {}

    DeferredRenderer::~DeferredRenderer() 
    {
        delete m_gBuffer;
    }

    void DeferredRenderer::Initialize() 
    {
        SetupGBuffer();

        m_gBufferShader = m_shaderManager->CreateShaderFromFile("GeometryPass", "gbuffer_vert.glsl", "gbuffer_frag.glsl", "../Assets/");
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        SmallArray<TextureAttribute> attributes(4);
        attributes[0] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };   // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F, GL_RGBA, GL_FLOAT };        // Normals (RGB) + Reserved (A)
        attributes[2] = { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };      // Metallic (R) + Roughness (G)
        attributes[3] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };  // Emissive (RGB) + Reserved (A)

        m_gBuffer = m_rtManager->CreateRenderTarget("GBufferTarget", m_width, m_height, attributes, true, attributes.Size());
    }

    void DeferredRenderer::GBufferPass(Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        m_gBuffer->Bind(); // Bind the G-buffer
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_gBufferShader->GetProgramId());

        TraverseSceneGraph(sceneGraph, viewMatrix, projMatrix);

        m_gBuffer->Unbind(); // Unbind the G-buffer
    }

    void DeferredRenderer::TraverseSceneGraph(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) 
    {
        // Recursively traverse the scene graph
        for (auto child : node->children) 
        {
            TraverseSceneGraph(child.get(), viewMatrix, projMatrix);
        }

        // If the node is renderable, draw it
        if (node->GetTag() == NodeTag::Mesh) 
        {
            RenderNode(node, viewMatrix, projMatrix);
        }
    }

    void DeferredRenderer::RenderNode(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) 
    {
        ShaderProgram* shader = m_gBufferShader;

        m_graphics->BindShader(shader->GetProgramId());

        glm::mat4 modelMatrix = node->GetGlobalTransform();
        glm::mat4 modelViewMatrix = viewMatrix * modelMatrix;
        glm::mat4 mvpMatrix = projMatrix * modelViewMatrix;
        glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelViewMatrix)));

        // Pass matrices to the shader
        shader->SetUniform("u_ModelMatrix", modelMatrix);
        shader->SetUniform("u_ModelViewMatrix", modelViewMatrix);
        shader->SetUniform("u_MVPMatrix", mvpMatrix);
        shader->SetUniform("u_NormalMatrix", normalMatrix);

        // Render the mesh
        Mesh* mesh = node->meshes[0];
        if (mesh) 
        {
            m_graphics->RenderMesh(mesh);
        }
    }

    void DeferredRenderer::Resize(int width, int height) 
    {
        m_width = width;
        m_height = height;

        m_gBuffer->UnloadFromGraphics();

        SetupGBuffer();
    }
}