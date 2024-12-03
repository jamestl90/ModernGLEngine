#include "DeferredRenderer.h"
#include <glm/gtc/type_ptr.hpp>

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(Graphics* graphics, RenderTargetManager* rtManager, ShaderManager* shaderManager, int width, int height, const std::string& assetFolder)
        : m_graphics(graphics), m_rtManager(rtManager), m_shaderManager(shaderManager), m_width(width), m_height(height),
        m_assetFolder(assetFolder), m_gBufferTarget(nullptr), m_gBufferShader(nullptr) {}

    DeferredRenderer::~DeferredRenderer() 
    {

    }

    void DeferredRenderer::Initialize() 
    {
        SetupGBuffer();

        auto finalAssetPath = m_assetFolder + "Core/";
        m_gBufferDebugShader = m_shaderManager->CreateShaderFromFile("DebugGBuffer", "gbuffer_debug_vert.glsl", "gbuffer_debug_frag.glsl", finalAssetPath);
        m_gBufferShader = m_shaderManager->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", finalAssetPath);

        InitScreenSpaceTriangle();
    }

    void DeferredRenderer::InitScreenSpaceTriangle() 
    {
        const float triangleVertices[] = {
            // Positions        // UVs
            -1.0f, -3.0f,       0.0f, 2.0f,  
             3.0f,  1.0f,       2.0f, 0.0f,  
            -1.0f,  1.0f,       0.0f, 0.0f   
        };

        // Generate and bind VAO/VBO
        glGenVertexArrays(1, &m_triangleVAO);
        glGenBuffers(1, &m_triangleVBO);

        glBindVertexArray(m_triangleVAO);

        glBindBuffer(GL_ARRAY_BUFFER, m_triangleVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0); 
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

        glBindVertexArray(0); 
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        SmallArray<TextureAttribute> attributes(4);
        attributes[0] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };   // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F, GL_RGBA, GL_FLOAT };        // Normals (RGB) + Reserved (A)
        attributes[2] = { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };      // Metallic (R) + Roughness (G)
        attributes[3] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };  // Emissive (RGB) + Reserved (A)

        m_gBufferTarget = m_rtManager->CreateRenderTarget("GBufferTarget", m_width, m_height, attributes, true, attributes.Size());
    }

    void DeferredRenderer::GBufferPass(Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        m_graphics->BindFrameBuffer(m_gBufferTarget->GetFrameBufferId());
        m_graphics->SetDepthMask(GL_TRUE);
        m_graphics->Enable(GL_DEPTH_TEST);
        m_graphics->Disable(GL_BLEND);
        m_graphics->SetViewport(0, 0, m_width, m_height);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_gBufferShader->GetProgramId());

        m_gBufferShader->SetUniform("u_View", viewMatrix);
        m_gBufferShader->SetUniform("u_Projection", projMatrix);

        TraverseSceneGraph(sceneGraph, viewMatrix, projMatrix);

        m_graphics->BindFrameBuffer(0); 
    }

    void DeferredRenderer::TraverseSceneGraph(Node* node, const glm::mat4& viewMatrix, const glm::mat4& projMatrix) 
    {
        if (!node || node->children.empty() && node->GetTag() != NodeTag::Mesh) return;

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
        if (node->meshes.empty()) return; // Skip if no meshes

        Mesh* mesh = node->meshes[0];
        if (!mesh) return; // Skip if mesh is null

        Material* mat = mesh->GetMaterialAt(0);
        if (!mat) return; // Skip if material is null

        glm::mat4 modelMatrix = node->GetGlobalTransform();
        m_gBufferShader->SetUniform("u_Model", modelMatrix);

        SetUniformsForGBuffer(mesh, mat);

        if (mesh) 
        {
            m_graphics->RenderMesh(mesh);
        }
    }

    void DeferredRenderer::SetUniformsForGBuffer(Mesh* mesh, Material* mat)
    {
        m_gBufferShader->SetUniform("baseColorFactor", mat->baseColorFactor);
        m_graphics->BindTexture(m_gBufferShader, "baseColorTexture", "useBaseColorTexture", mat->baseColorTexture, 0);
        
        // Metallic-Roughness
        m_gBufferShader->SetUniformf("metallicFactor", mat->metallicFactor);
        m_gBufferShader->SetUniformf("roughnessFactor", mat->roughnessFactor);
        m_graphics->BindTexture(m_gBufferShader, "metallicRoughnessTexture", "useMetallicRoughnessTexture", mat->metallicRoughnessTexture, 1);
        
        m_graphics->BindTexture(m_gBufferShader, "normalTexture", "useNormalTexture", mat->normalTexture, 2);
        m_graphics->BindTexture(m_gBufferShader, "occlusionTexture", "useOcclusionTexture", mat->occlusionTexture, 3);
        
        // Emissive
        m_gBufferShader->SetUniform("emissiveFactor", mat->emissiveFactor);
        m_graphics->BindTexture(m_gBufferShader, "emissiveTexture", "useEmissiveTexture", mat->emissiveTexture, 4);
    }

    void DeferredRenderer::DebugGBuffer(int debugMode) 
    {
        m_graphics->BindFrameBuffer(0); // Render to the default framebuffer
        glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_gBufferDebugShader->GetProgramId());

        const int numModes = 6; // Number of debug modes (Albedo, Normals, Metallic, AO, Roughness, Emissive)
        int cols = 2;           // Number of columns for split screen
        int rows = (numModes + cols - 1) / cols;

        int viewportWidth = m_width / cols;
        int viewportHeight = m_height / rows;

        for (int mode = 0; mode < numModes; ++mode)
        {
            // Calculate viewport for each debug mode
            int x = (mode % cols) * viewportWidth;
            int y = (mode / cols) * viewportHeight;

            glViewport(x, y, viewportWidth, viewportHeight);

            // Set the debug mode
            m_gBufferDebugShader->SetUniformi("debugMode", mode);

            // Bind the appropriate texture for each mode
            switch (mode) {
            case 0: // Albedo + AO
                m_gBufferTarget->BindTexture(0, 0);
                m_gBufferDebugShader->SetUniformi("gAlbedoAO", 0);
                break;

            case 1: // Normals
                m_gBufferTarget->BindTexture(1, 1);
                m_gBufferDebugShader->SetUniformi("gNormals", 1);
                break;

            case 2: // Metallic
            case 4: // Roughness (same texture, different channels)
                m_gBufferTarget->BindTexture(2, 2);
                m_gBufferDebugShader->SetUniformi("gMetallicRoughness", 2);
                break;

            case 3: // AO (alpha channel of Albedo)
                m_gBufferTarget->BindTexture(0, 0); // Albedo + AO texture
                m_gBufferDebugShader->SetUniformi("gAlbedoAO", 0);
                break;

            case 5: // Emissive
                m_gBufferTarget->BindTexture(3, 3);
                m_gBufferDebugShader->SetUniformi("gEmissive", 3);
                break;

            default:
                break;
            }

            // Render the screen-space triangle for the current debug mode
            RenderScreenSpaceTriangle();
        }

        // Restore the full-screen viewport
        glViewport(0, 0, m_width, m_height);
    }

    void DeferredRenderer::RenderScreenSpaceTriangle() 
    {
        glBindVertexArray(m_triangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3); // Draw 3 vertices (1 triangle)
        glBindVertexArray(0);
    }

    void DeferredRenderer::Resize(int width, int height) 
    {
        m_width = width;
        m_height = height;

        m_gBufferTarget->UnloadFromGraphics();

        SetupGBuffer();
    }
}