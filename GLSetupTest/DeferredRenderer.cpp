#include "DeferredRenderer.h"
#include <glm/gtc/type_ptr.hpp>

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(Graphics* graphics, RenderTargetManager* rtManager, ShaderManager* shaderManager, ShaderStorageManager* shaderStorageManager, int width, int height, const std::string& assetFolder)
        : m_graphics(graphics), m_rtManager(rtManager), m_shaderManager(shaderManager), m_triangleVAO(0), m_shaderStorageManager(shaderStorageManager),
        m_width(width), m_height(height), m_gBufferDebugShader(nullptr), m_triangleVertexBuffer(),
        m_assetFolder(assetFolder), m_gBufferTarget(nullptr), m_gBufferShader(nullptr) {}

    DeferredRenderer::~DeferredRenderer() 
    {
        m_graphics->DisposeVertexBuffer(m_triangleVAO, m_triangleVertexBuffer);
    }

    void DeferredRenderer::Initialize() 
    {
        SetupGBuffer();

        auto finalAssetPath = m_assetFolder + "Core/";
        m_gBufferDebugShader = m_shaderManager->CreateShaderFromFile("DebugGBuffer", "gbuffer_debug_vert.glsl", "gbuffer_debug_frag.glsl", finalAssetPath);
        m_gBufferShader = m_shaderManager->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", finalAssetPath);
        m_lightingTestShader = m_shaderManager->CreateShaderFromFile("LightingTest", "lighting_test_vert.glsl", "lighting_test_frag.glsl", finalAssetPath);

        InitScreenSpaceTriangle();
    }

    void DeferredRenderer::InitScreenSpaceTriangle() 
    {
        const float triangleVertices[] = 
        {
            // Positions        // UVs
            -1.0f, -3.0f,       0.0f, 2.0f,  
             3.0f,  1.0f,       2.0f, 0.0f,  
            -1.0f,  1.0f,       0.0f, 0.0f   
        };
        std::vector<float> triVerts;
        triVerts.insert(triVerts.end(), std::begin(triangleVertices), std::end(triangleVertices));

        m_triangleVertexBuffer.SetDataType(GL_FLOAT);
        m_triangleVertexBuffer.SetType(GL_ARRAY_BUFFER);
        m_triangleVertexBuffer.SetDrawType(GL_STATIC_DRAW);

        VertexAttribute posAttri(JLEngine::POSITION, 0, 2);
        VertexAttribute uvAttri(JLEngine::TEX_COORD_2D, 2 * sizeof(float), 2);
        m_triangleVertexBuffer.AddAttribute(posAttri);
        m_triangleVertexBuffer.AddAttribute(uvAttri);
        m_triangleVertexBuffer.Set(triVerts);
        m_triangleVertexBuffer.CalcStride();

        m_triangleVAO = m_graphics->CreateVertexArray();
        m_graphics->CreateVertexBuffer(m_triangleVertexBuffer);
        m_graphics->BindVertexArray(0);
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        SmallArray<TextureAttribute> attributes(4);
        attributes[0] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };   // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F, GL_RGBA, GL_FLOAT };        // Normals (RGB) + Reserved (A)
        attributes[2] = { GL_RG8, GL_RG, GL_UNSIGNED_BYTE };      // Metallic (R) + Roughness (G)
        attributes[3] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };  // Emissive (RGB) + Reserved (A)

        m_gBufferTarget = m_rtManager->CreateRenderTarget("GBufferTarget", m_width, m_height, attributes, DepthType::Texture, attributes.Size());
    }

    void DeferredRenderer::GBufferPass(Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        m_graphics->BindFrameBuffer(m_gBufferTarget->GetFrameBufferId());
        m_graphics->SetDepthMask(GL_TRUE);
        m_graphics->Enable(GL_DEPTH_TEST);
        m_graphics->Disable(GL_BLEND);
        m_graphics->SetViewport(0, 0, m_width, m_height);
        m_graphics->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
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
        for (auto& child : node->children) 
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
        m_graphics->ClearColour(0.2f, 0.2f, 0.2f, 0.2f);
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

            m_graphics->SetViewport(x, y, viewportWidth, viewportHeight);

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

            case 2: // Metallic Roughness (same texture, different channels)
                m_gBufferTarget->BindTexture(2, 2);
                m_gBufferDebugShader->SetUniformi("gMetallicRoughness", 2);
                break;

            case 3: // AO (alpha channel of Albedo)
                m_gBufferTarget->BindTexture(0, 0); // Albedo + AO texture
                m_gBufferDebugShader->SetUniformi("gAlbedoAO", 0);
                break;

            case 4: 
                m_gBufferTarget->BindDepthTexture(4); 
                m_gBufferDebugShader->SetUniformi("gDepth", 4);
                break;

            case 5: // Emissive
                m_gBufferTarget->BindTexture(3, 3);
                m_gBufferDebugShader->SetUniformi("gEmissive", 3);
                break;

            default:
                break;
            }

            RenderScreenSpaceTriangle();
        }

        m_graphics->SetViewport(0, 0, m_width, m_height);
    }

    void DeferredRenderer::Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, bool debugGBuffer)
    {
        GBufferPass(sceneRoot, viewMatrix, projMatrix);

        if (debugGBuffer)
        {
            for (int mode = 0; mode < 6; ++mode)
            {
                DebugGBuffer(mode);
            }
        }
        else
        {
            TestLightPass(eyePos, viewMatrix, projMatrix);
        }
    }

    void DeferredRenderer::TestLightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        glm::vec3 lightDir(0.0f, 0.0f, 1.0f);

        m_graphics->BindFrameBuffer(0); // Render to the default framebuffer
        m_graphics->ClearColour(0.2f, 0.2f, 0.2f, 0.2f);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_lightingTestShader->GetProgramId());
        m_graphics->SetViewport(0, 0, m_width, m_height);

        m_gBufferTarget->BindTexture(0, 0);
        m_lightingTestShader->SetUniformi("gAlbedoAO", 0);
        m_gBufferTarget->BindTexture(1, 1);
        m_lightingTestShader->SetUniformi("gNormals", 1);
        m_gBufferTarget->BindTexture(2, 2);
        m_lightingTestShader->SetUniformi("gMetallicRoughness", 2);
        m_gBufferTarget->BindTexture(3, 3);
        m_lightingTestShader->SetUniformi("gEmissive", 3);
        m_gBufferTarget->BindDepthTexture(4);
        m_gBufferDebugShader->SetUniformi("gDepth", 4);

        m_lightingTestShader->SetUniform("lightDirection", glm::vec3(0.5f, -1.0f, -0.5f));
        m_lightingTestShader->SetUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Warm light
        m_lightingTestShader->SetUniform("cameraPos", eyePos);
        m_lightingTestShader->SetUniform("u_ViewInverse", glm::inverse(viewMatrix));
        m_lightingTestShader->SetUniform("u_ProjectionInverse", glm::inverse(projMatrix));

        auto frustum = m_graphics->GetViewFrustum();
        m_lightingTestShader->SetUniformf("u_Near", frustum->GetNear());
        m_lightingTestShader->SetUniformf("u_Far", frustum->GetNear());

        RenderScreenSpaceTriangle();
    }

    void DeferredRenderer::RenderScreenSpaceTriangle() 
    {
        m_graphics->BindVertexArray(m_triangleVAO);
        m_graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 3);
        m_graphics->BindVertexArray(0);
    }

    void DeferredRenderer::Resize(int width, int height) 
    {
        if (width == m_width && height == m_height)
            return; // No need to resize if dimensions are unchanged

        m_width = width;
        m_height = height;

        // Recreate the G-buffer to match the new dimensions
        m_rtManager->Remove(m_gBufferTarget->GetName()); // Delete the old G-buffer
        SetupGBuffer();

        std::cout << "DeferredRenderer resized to: " << m_width << "x" << m_height << std::endl;
    }
}