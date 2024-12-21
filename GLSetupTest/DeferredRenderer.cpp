#include "DeferredRenderer.h"
#include "Material.h"
#include "Graphics.h"
#include "DirectionalLightShadowMap.h"
#include "HDRISky.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(Graphics* graphics, AssetLoader* assetLoader, int width, int height, const std::string& assetFolder)
        : m_graphics(graphics), m_assetLoader(assetLoader), m_triangleVAO(0), m_textureDebugShader(nullptr),
        m_width(width), m_height(height), m_gBufferDebugShader(nullptr), m_triangleVertexBuffer(),
        m_assetFolder(assetFolder), m_gBufferTarget(nullptr), m_gBufferShader(nullptr), 
        m_enableDLShadows(true), m_debugModes(DebugModes::None), m_hdriSky(nullptr) {}

    DeferredRenderer::~DeferredRenderer() 
    {
        m_graphics->DisposeVertexBuffer(m_triangleVertexBuffer);
        
        m_graphics->DisposeShader(m_textureDebugShader);
        m_graphics->DisposeShader(m_gBufferDebugShader);
        m_graphics->DisposeShader(m_gBufferShader);
        m_graphics->DisposeShader(m_lightingTestShader);
    }
    
    void DeferredRenderer::Initialize() 
    {
        m_directionalLight.position = glm::vec3(0, 30.0f, 30.0f);
        m_directionalLight.direction = -glm::normalize(m_directionalLight.position - glm::vec3(0.0f));

        auto shaderAssetPath = m_assetFolder + "Core/Shaders/";
        auto textureAssetPath = m_assetFolder + "HDRI/";

        auto dlShader = m_assetLoader->CreateShaderFromFile("DLShadowMap", "dlshadowmap_vert.glsl", "dlshadowmap_frag.glsl", shaderAssetPath);
        m_dlShadowMap = new DirectionalLightShadowMap(m_graphics, dlShader);
        m_dlShadowMap->Initialise();

        SetupGBuffer();
        
        m_textureDebugShader = m_assetLoader->CreateShaderFromFile("DebugScreenSpaceTriangle", "pos_uv_vert.glsl", "/Debug/depth_debug_frag.glsl", shaderAssetPath);
        m_gBufferDebugShader = m_assetLoader->CreateShaderFromFile("DebugGBuffer", "pos_uv_vert.glsl", "/Debug/gbuffer_debug_frag.glsl", shaderAssetPath);
        m_gBufferShader = m_assetLoader->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", shaderAssetPath);
        m_lightingTestShader = m_assetLoader->CreateShaderFromFile("LightingTest", "lighting_test_vert.glsl", "lighting_test_frag.glsl", shaderAssetPath);
        auto skyboxShader = m_assetLoader->CreateShaderFromFile("SkyboxShader", "enviro_cubemap_vert.glsl", "enviro_cubemap_frag.glsl", shaderAssetPath);
        
        std::array<std::string, 6> cubemapFiles = 
        {
            "venice_sunset_4k/posx.hdr",
            "venice_sunset_4k/negx.hdr",
            "venice_sunset_4k/posy.hdr",
            "venice_sunset_4k/negy.hdr",
            "venice_sunset_4k/posz.hdr",
            "venice_sunset_4k/negz.hdr"
        };
        auto hdriTexture = m_assetLoader->CreateCubemapFromFile("HDRI Cubemap Texture", cubemapFiles, textureAssetPath);

        m_hdriSky = new HDRISky(hdriTexture, skyboxShader);
        m_hdriSky->Initialise(m_graphics);

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

        VertexAttribute posAttri(JLEngine::AttributeType::POSITION, 0, 2);
        VertexAttribute uvAttri(JLEngine::AttributeType::TEX_COORD_0, 2 * sizeof(float), 2);
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

        m_gBufferTarget = m_assetLoader->CreateRenderTarget("GBufferTarget", m_width, m_height, attributes, DepthType::Texture, attributes.Size());
    }

    glm::mat4 DeferredRenderer::DirectionalShadowMapPass(RenderGroupMap& renderGroups, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        glm::mat4 lightSpaceMatrix = GetLightMatrix(m_directionalLight.position,
            m_directionalLight.direction, m_dlShadowMap->GetSize(), 0.01f, m_dlShadowMap->GetShadowDistance());

        m_dlShadowMap->ShadowMapPassSetup(lightSpaceMatrix);

        for (const auto& [key, batches] : renderGroups)
        {
            for (const auto& batch : batches)
            {
                m_graphics->BindVertexArray(batch.first->GetVertexBuffer()->GetVAO());
                m_graphics->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.first->GetIndexBuffer()->GetId());

                glm::mat4 modelMatrix = batch.second;
                m_dlShadowMap->SetModelMatrix(modelMatrix);

                m_graphics->DrawElementBuffer(GL_TRIANGLES, (uint32_t)batch.first->GetIndexBuffer()->Size(), GL_UNSIGNED_INT, nullptr);
            }
        }
        m_graphics->BindFrameBuffer(0);
        return lightSpaceMatrix;
    }

    void DeferredRenderer::GBufferPass(RenderGroupMap& renderGroups, Node* sceneGraph, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
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

        RenderGroups(renderGroups);
    }

    void DeferredRenderer::SetUniformsForGBuffer(Material* mat)
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

        if (mat->alphaMode == AlphaMode::MASK)
        {
            m_gBufferShader->SetUniformf("u_AlphaCutoff", mat->alphaCutoff);
        }
        else
        {
            m_gBufferShader->SetUniformf("u_AlphaCutoff", 1.0);
        }
    }

    void DeferredRenderer::DebugGBuffer(int debugMode) 
    {
        m_graphics->BindFrameBuffer(0); // Render to the default framebuffer
        m_graphics->ClearColour(0.2f, 0.2f, 0.2f, 0.2f);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_gBufferDebugShader->GetProgramId());

        auto frustum = m_graphics->GetViewFrustum();
        m_lightingTestShader->SetUniformf("u_Near", frustum->GetNear());
        m_lightingTestShader->SetUniformf("u_Far", frustum->GetFar());

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

    void DeferredRenderer::DebugDirectionalLightShadows()
    {
        m_graphics->SetViewport(0, 0, (int)(m_width * 0.5f), m_height);

        m_graphics->BindFrameBuffer(0); // Render to the default framebuffer
        m_graphics->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_textureDebugShader->GetProgramId());
        m_graphics->SetActiveTexture(0);
        m_graphics->BindTexture(GL_TEXTURE_2D, m_dlShadowMap->GetShadowMapID());
        m_textureDebugShader->SetUniformi("u_DepthTexture", 0);
        m_textureDebugShader->SetUniformi("u_Linearize", 0);
        m_textureDebugShader->SetUniformf("u_Near", 0.01f);
        m_textureDebugShader->SetUniformf("u_Far", m_dlShadowMap->GetShadowDistance());

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport((int)(m_width * 0.5f), 0, (int)(m_width * 0.5f), m_height);

        m_gBufferTarget->BindDepthTexture(0);
        m_gBufferDebugShader->SetUniformi("u_DepthTexture", 0);
        m_textureDebugShader->SetUniformi("u_Linearize", 1);
        auto frustum = m_graphics->GetViewFrustum();
        m_textureDebugShader->SetUniformf("u_Near", frustum->GetNear());
        m_textureDebugShader->SetUniformf("u_Far", frustum->GetFar());

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport(0, 0, m_width, m_height);
    }

    void DeferredRenderer::Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        RenderGroupMap renderGroups;
        GroupRenderables(sceneRoot, renderGroups);
        
        DrawUI();

        auto lightSpaceMatrix = DirectionalShadowMapPass(renderGroups, eyePos, viewMatrix, projMatrix);

        GBufferPass(renderGroups, sceneRoot, viewMatrix, projMatrix);
        if (m_hdriSky)
        {
            m_hdriSky->Render(viewMatrix, projMatrix);
        }
        if (m_debugModes != DebugModes::None)
        {
            if (m_debugModes == DebugModes::GBuffer)
            {
                for (int mode = 0; mode < 6; ++mode)
                {
                    DebugGBuffer(mode);
                }
            }
            else if (m_debugModes == DebugModes::DirectionalLightShadows)
            {
                DebugDirectionalLightShadows();
            }
        }
        else
        {
            LightPass(eyePos, viewMatrix, projMatrix, lightSpaceMatrix);
        }


    }

    void DeferredRenderer::CycleDebugMode()
    {
        static DebugModes modes[3] = { DebugModes::GBuffer, DebugModes::DirectionalLightShadows, DebugModes::None };
        if (m_debugModes == modes[1])
            m_debugModes = modes[2];
        else if (m_debugModes == modes[2])
            m_debugModes = modes[0];
        else if (m_debugModes == modes[0])
            m_debugModes = modes[1];
    }

    //void DeferredRenderer::SkyboxPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    //{
    //    if (!m_skybox) return;
    //
    //    auto& batch = m_skybox->mesh->GetBatches()[0];
    //    auto skyboxTexture = batch->GetMaterial()->baseColorTexture;
    //    auto& vertexBuffer = batch->GetVertexBuffer();
    //    auto& indexBuffer = batch->GetIndexBuffer();
    //
    //    //m_graphics->BindFrameBuffer(0);
    //    m_graphics->ClearColour(0.2f, 0.2f, 0.2f, 0.2f);
    //    m_graphics->SetDepthMask(GL_FALSE);
    //    m_graphics->Enable(GL_DEPTH_TEST);
    //
    //    m_graphics->BindShader(m_skyboxShader->GetProgramId());
    //    m_skyboxShader->SetUniform("u_Model", m_skybox->GetGlobalTransform());
    //    m_skyboxShader->SetUniform("u_View", glm::mat4(glm::mat3(viewMatrix)));
    //    m_skyboxShader->SetUniform("u_Projection", projMatrix);
    //    m_graphics->SetActiveTexture(0);
    //    m_graphics->BindTexture(GL_TEXTURE_2D, skyboxTexture->GetGPUID());
    //    m_skyboxShader->SetUniformi("u_SkyboxTexture", 0);
    //
    //    m_graphics->BindVertexArray(vertexBuffer->GetVAO());
    //    m_graphics->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer->GetId());
    //
    //    m_graphics->DrawElementBuffer(GL_TRIANGLES, (uint32_t)indexBuffer->Size(), GL_UNSIGNED_INT, nullptr);
    //
    //    m_graphics->SetDepthMask(GL_TRUE);
    //}

    void DeferredRenderer::DrawUI()
    {
        static float lightAngle = 0.0f;  // Rotation angle in degrees
        static float lightRadius = 30.0f; // Distance from the center
        ImGui::Begin("Shadow Controls");
        if (ImGui::SliderFloat("Light Rotation", &lightAngle, 0.0f, 360.0f, "%.1f degrees"))
        {
            float radians = glm::radians(lightAngle);
            m_directionalLight.position = glm::vec3(
                lightRadius * cos(radians),
                30.0f,
                lightRadius * sin(radians)
            );
            m_directionalLight.direction = glm::normalize(-m_directionalLight.position);
        }
        ImGui::SliderFloat("Bias", &m_dlShadowMap->GetBias(), 0.00020f, 0.0009f, "%.6f");
        ImGui::SliderFloat("Distance", &m_dlShadowMap->GetShadowDistance(), 10.0, 200.0f, "%.6f");
        ImGui::SliderFloat("Size", &m_dlShadowMap->GetSize(), 10.0, 50.0f, "%.6f");
        ImGui::SliderInt("PCF Kernel Size", &m_dlShadowMap->GetPCFKernelSize(), 0, 5);
        ImGui::End();
    }

    void DeferredRenderer::LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix)
    {
        m_graphics->BindFrameBuffer(0); // Render to the default framebuffer
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
        m_lightingTestShader->SetUniformi("gDepth", 4);
        m_graphics->SetActiveTexture(5);
        m_graphics->BindTexture(GL_TEXTURE_2D, m_dlShadowMap->GetShadowMapID());
        m_lightingTestShader->SetUniformi("gDLShadowMap", 5);

        m_lightingTestShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
        m_lightingTestShader->SetUniform("u_LightDirection", m_directionalLight.direction);
        m_lightingTestShader->SetUniform("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f)); // Warm light
        m_lightingTestShader->SetUniform("u_CameraPos", eyePos);
        m_lightingTestShader->SetUniform("u_ViewInverse", glm::inverse(viewMatrix));
        m_lightingTestShader->SetUniform("u_ProjectionInverse", glm::inverse(projMatrix));

        auto frustum = m_graphics->GetViewFrustum();
        m_lightingTestShader->SetUniformf("u_Near", frustum->GetNear());
        m_lightingTestShader->SetUniformf("u_Far", frustum->GetFar());

        m_lightingTestShader->SetUniformf("u_Bias", m_dlShadowMap->GetBias());
        m_lightingTestShader->SetUniformi("u_PCFKernelSize", m_dlShadowMap->GetPCFKernelSize());

        RenderScreenSpaceTriangle();
    }

    void DeferredRenderer::RenderScreenSpaceTriangle() 
    {
        m_graphics->BindVertexArray(m_triangleVAO);
        m_graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 3);
        m_graphics->BindVertexArray(0);
    }

    glm::mat4 DeferredRenderer::GetLightMatrix(glm::vec3& lightPos, glm::vec3& lightDir, float size, float nearPlane, float farPlane)
    {
        float orthoSize = size;

        glm::mat4 lightProjection = glm::ortho(
            -orthoSize, orthoSize,  // Left, Right
            -orthoSize, orthoSize,  // Bottom, Top
            nearPlane, farPlane       // Near, Far
        );

        glm::vec3 lightTarget = glm::normalize(lightPos + (lightDir * 2.0f));
        glm::mat4 lightView = glm::lookAt(lightPos, lightTarget, glm::vec3(0.0f, 1.0f, 0.0f));

        return lightProjection * lightView;
    }

    void DeferredRenderer::GroupRenderables(Node* node, RenderGroupMap& renderGroups)
    {
        if (!node)
            return;

        if (!node->mesh && node->children.empty()) return;

        glm::mat4 worldMatrix = node->GetGlobalTransform();

        // If the node contains a mesh, add its batches to the appropriate group
        if (node->GetTag() == NodeTag::Mesh && node->mesh)
        {
            for (auto& batch : node->mesh->GetBatches())
            {
                RenderGroupKey key(batch->GetMaterial()->GetHandle(), batch->attributesKey);
                renderGroups[key].emplace_back(batch.get(), worldMatrix);
            }
        }
        //if (node->GetTag() == NodeTag::Skybox && node->mesh)
        //{
        //    m_skybox = node;
        //}

        // Recursively process child nodes
        for (auto& child : node->children)
        {
            GroupRenderables(child.get(), renderGroups);
        }
    }

    void DeferredRenderer::RenderGroups(const RenderGroupMap& renderGroups)
    {
        for (const auto& [key, batches] : renderGroups)
        {
            // Extract material ID and attributes key from the key
            int materialId = key.first;

            // Bind material (assuming materials are identified by ID)
            auto material = m_assetLoader->GetMaterialManager()->Get(materialId);
            SetUniformsForGBuffer(material);

            // Render all batches in this group
            for (const auto& batch : batches)
            {
                // Bind vertex and index buffers
                m_graphics->BindVertexArray(batch.first->GetVertexBuffer()->GetVAO());
                m_graphics->BindBuffer(GL_ELEMENT_ARRAY_BUFFER, batch.first->GetIndexBuffer()->GetId());

                // Set model matrix
                glm::mat4 modelMatrix = batch.second;
                m_gBufferShader->SetUniform("u_Model", modelMatrix);

                // Issue draw call
                m_graphics->DrawElementBuffer(GL_TRIANGLES, (uint32_t)batch.first->GetIndexBuffer()->Size(), GL_UNSIGNED_INT, nullptr);
            }
        }
    }

    void DeferredRenderer::Resize(int width, int height) 
    {
        if (width == m_width && height == m_height)
            return; // No need to resize if dimensions are unchanged

        m_width = width;
        m_height = height;

        m_gBufferTarget->ResizeTextures(m_width, m_height);

        // Recreate the G-buffer to match the new dimensions
        //m_assetLoader->GetRenderTargetManager()->Remove(m_gBufferTarget->GetName()); // Delete the old G-buffer
        //SetupGBuffer();

        std::cout << "DeferredRenderer resized to: " << m_width << "x" << m_height << std::endl;
    }
}