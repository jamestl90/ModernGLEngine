#include "DeferredRenderer.h"
#include "Material.h"
#include "GraphicsAPI.h"
#include "DirectionalLightShadowMap.h"
#include "HDRISky.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

#include <imgui.h>

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* assetLoader, int width, int height, const std::string& assetFolder)
        : m_graphics(graphics), m_assetLoader(assetLoader), m_shadowDebugShader(nullptr),
        m_width(width), m_height(height), m_gBufferDebugShader(nullptr), 
        m_assetFolder(assetFolder), m_gBufferTarget(nullptr), m_gBufferShader(nullptr), 
        m_enableDLShadows(true), m_debugModes(DebugModes::None), m_hdriSky(nullptr) {}

    DeferredRenderer::~DeferredRenderer() 
    {
        Graphics::DisposeVertexArray(&m_triangleVAO);
        
        m_graphics->DisposeShader(m_shadowDebugShader);
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
        
        m_shadowDebugShader = m_assetLoader->CreateShaderFromFile("DebugDirShadows", "pos_uv_vert.glsl", "/Debug/depth_debug_frag.glsl", shaderAssetPath);
        m_gBufferDebugShader = m_assetLoader->CreateShaderFromFile("DebugGBuffer", "pos_uv_vert.glsl", "/Debug/gbuffer_debug_frag.glsl", shaderAssetPath);
        m_gBufferShader = m_assetLoader->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", shaderAssetPath);
        m_lightingTestShader = m_assetLoader->CreateShaderFromFile("LightingTest", "lighting_test_vert.glsl", "lighting_test_frag.glsl", shaderAssetPath);
        m_debugTextureShader = m_assetLoader->CreateShaderFromFile("DebugTexture", "pos_uv_vert.glsl", "pos_uv_frag.glsl", shaderAssetPath);

        //std::string hdriFolder = "venice_sunset_4k";
        //std::array<std::string, 6> hdriFiles = 
        //{
        //    hdriFolder + "/posx.hdr",
        //    hdriFolder + "/negx.hdr",
        //    hdriFolder + "/posy.hdr",
        //    hdriFolder + "/negy.hdr",
        //    hdriFolder + "/posz.hdr",
        //    hdriFolder + "/negz.hdr"
        //};
        //std::array<std::string, 6> irradianceFiles =
        //{
        //    hdriFolder + "/irradiance_posx.hdr",
        //    hdriFolder + "/irradiance_negx.hdr",
        //    hdriFolder + "/irradiance_posy.hdr",
        //    hdriFolder + "/irradiance_negy.hdr",
        //    hdriFolder + "/irradiance_posz.hdr",
        //    hdriFolder + "/irradiance_negz.hdr"
        //};

        HdriSkyInitParams params;
        params.fileName = "rogland_clear_night_4k.hdr";
        params.irradianceMapSize = 32;
        params.prefilteredMapSize = 128;
        params.prefilteredSamples = 2048;

        m_hdriSky = new HDRISky(m_assetLoader);
        m_hdriSky->Initialise(m_assetFolder, params);

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
        std::vector<float> triVerts(std::begin(triangleVertices), std::end(triangleVertices));

        // Initialize the VertexBuffer
        JLEngine::VertexBuffer triangleVBO(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
        triangleVBO.Set(triVerts);

        // Add attributes to the VertexArrayObject
        m_triangleVAO.AddAttribute(JLEngine::AttributeType::POSITION);
        m_triangleVAO.AddAttribute(JLEngine::AttributeType::TEX_COORD_0);

        // Calculate the stride and associate the buffer with the VAO
        m_triangleVAO.CalcStride();
        m_triangleVAO.SetVertexBuffer(triangleVBO);

        Graphics::CreateVertexArray(&m_triangleVAO);
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        SmallArray<TextureAttribute> attributes(4);
        attributes[0] = { GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE };   // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F, GL_RGBA, GL_FLOAT };        // Normals (RGB) + Cast/Receive Shadows (A)
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
            auto matId = key.first;
            auto material = m_assetLoader->GetMaterialManager()->Get(matId);
            if (material->castShadows == false) continue;

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
        BindTexture(m_gBufferShader, "baseColorTexture", "useBaseColorTexture", mat->baseColorTexture.get(), 0);
        
        // Metallic-Roughness
        m_gBufferShader->SetUniformf("metallicFactor", mat->metallicFactor);
        m_gBufferShader->SetUniformf("roughnessFactor", mat->roughnessFactor);
        BindTexture(m_gBufferShader, "metallicRoughnessTexture", "useMetallicRoughnessTexture", mat->metallicRoughnessTexture.get(), 1);
        
        BindTexture(m_gBufferShader, "normalTexture", "useNormalTexture", mat->normalTexture.get(), 2);
        BindTexture(m_gBufferShader, "occlusionTexture", "useOcclusionTexture", mat->occlusionTexture.get(), 3);
        
        // Emissive
        m_gBufferShader->SetUniform("emissiveFactor", mat->emissiveFactor);
        BindTexture(m_gBufferShader, "emissiveTexture", "useEmissiveTexture", mat->emissiveTexture.get(), 4);

        if (mat->alphaMode == AlphaMode::MASK)
        {
            m_gBufferShader->SetUniformf("u_AlphaCutoff", mat->alphaCutoff);
        }
        else
        {
            m_gBufferShader->SetUniformf("u_AlphaCutoff", 1.0);
        }

        m_gBufferShader->SetUniformf("u_CastShadows", mat->castShadows ? 1.0f : 0.0f);
        m_gBufferShader->SetUniformf("u_ReceiveShadows", mat->receiveShadows ? 1.0f : 0.0f);
    }

    void DeferredRenderer::BindTexture(ShaderProgram* shader, const std::string& uniformName, const std::string& flagName, Texture* texture, int textureUnit)
    {
        if (texture && texture->GetGPUID() != 0)
        {
            shader->SetUniformi(flagName, GL_TRUE);
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, texture->GetGPUID());
            shader->SetUniformi(uniformName, textureUnit);
        }
        else
        {
            shader->SetUniformi(flagName, GL_FALSE);
            glActiveTexture(GL_TEXTURE0 + textureUnit);
            glBindTexture(GL_TEXTURE_2D, 0);
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

        m_graphics->BindShader(m_shadowDebugShader->GetProgramId());
        m_graphics->SetActiveTexture(0);
        m_graphics->BindTexture(GL_TEXTURE_2D, m_dlShadowMap->GetShadowMapID());
        m_shadowDebugShader->SetUniformi("u_DepthTexture", 0);
        m_shadowDebugShader->SetUniformi("u_Linearize", 0);
        m_shadowDebugShader->SetUniformf("u_Near", 0.01f);
        m_shadowDebugShader->SetUniformf("u_Far", m_dlShadowMap->GetShadowDistance());

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport((int)(m_width * 0.5f), 0, (int)(m_width * 0.5f), m_height);

        m_gBufferTarget->BindDepthTexture(0);
        m_gBufferDebugShader->SetUniformi("u_DepthTexture", 0);
        m_shadowDebugShader->SetUniformi("u_Linearize", 1);
        auto frustum = m_graphics->GetViewFrustum();
        m_shadowDebugShader->SetUniformf("u_Near", frustum->GetNear());
        m_shadowDebugShader->SetUniformf("u_Far", frustum->GetFar());

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport(0, 0, m_width, m_height);
    }

    void DeferredRenderer::DebugHDRISky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        auto hdriSky = m_hdriSky->GetSkyGPUID();
        auto irradTex = m_hdriSky->GetIrradianceGPUID();
        auto brdfLut = m_hdriSky->GetBRDFLutGPUID();
        auto prefTex = m_hdriSky->GetPrefilteredGPUID();

        std::array<uint32_t, 4> ids =
        {
            m_hdriSky->GetSkyGPUID(),
            m_hdriSky->GetIrradianceGPUID(),
            m_hdriSky->GetBRDFLutGPUID(),
            m_hdriSky->GetPrefilteredGPUID()
        };

        std::array<glm::ivec4, 4> viewports =
        {
            glm::ivec4(0, 0, (int)(m_width * 0.5f), (int)(m_height * 0.5f)),
            glm::ivec4((int)(m_width * 0.5f), 0, (int)(m_width * 0.5f), (int)(m_height * 0.5f)),
            glm::ivec4(0, (int)(m_height * 0.5f), (int)(m_width * 0.5f), (int)(m_height * 0.5f)),
            glm::ivec4((int)(m_width * 0.5f), (int)(m_height * 0.5f), (int)(m_width * 0.5f), (int)(m_height * 0.5f))
        };

        m_graphics->BindFrameBuffer(0);

        m_graphics->SetViewport(viewports[0]);

        m_graphics->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
        m_graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_graphics->BindShader(m_debugTextureShader->GetProgramId());
        m_graphics->SetActiveTexture(0);
        m_graphics->BindTexture(GL_TEXTURE_2D, brdfLut);
        m_debugTextureShader->SetUniformi("u_Texture", 0);

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport(viewports[1]);
        m_hdriSky->Render(viewMatrix, projMatrix, 0);
        m_graphics->SetViewport(viewports[2]);
        m_hdriSky->Render(viewMatrix, projMatrix, 1);
        m_graphics->SetViewport(viewports[3]);
        m_hdriSky->Render(viewMatrix, projMatrix, 2);
    }

    void DeferredRenderer::Render(Node* sceneRoot, const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        RenderGroupMap renderGroups;
        GroupRenderables(sceneRoot, renderGroups);
        
        DrawUI();

        auto lightSpaceMatrix = DirectionalShadowMapPass(renderGroups, eyePos, viewMatrix, projMatrix);

        GBufferPass(renderGroups, sceneRoot, viewMatrix, projMatrix);

        if (m_debugModes != DebugModes::None)
        {
            std::string debugString;
            if (m_debugModes == DebugModes::GBuffer)
            {
                debugString = "Bottom Left to Top Right:\nGBuffer - Albedo, Normal, Metallic, Occlusion, Depth, Emission";
                for (int mode = 0; mode < 6; ++mode)
                {
                    DebugGBuffer(mode);
                }
            }
            else if (m_debugModes == DebugModes::DirectionalLightShadows)
            {
                debugString = "Bottom Left to Top Right:\nDirectional Shadowmap Depth, Camera Depth";
                DebugDirectionalLightShadows();
            }
            else if (m_debugModes == DebugModes::HDRISkyTextures)
            {
                debugString = "Bottom Left to Top Right:\nBRDF, HDR Sky, Irradiance, Prefiltered";
                DebugHDRISky(viewMatrix, projMatrix);
            }
            ImGui::Begin("Debug Views");
            ImGui::Text(debugString.c_str());
            ImGui::End(); 
        }
        else
        {
            LightPass(eyePos, viewMatrix, projMatrix, lightSpaceMatrix);
        }
    }

    void DeferredRenderer::CycleDebugMode()
    {
        static DebugModes modes[4] = 
        { 
            DebugModes::GBuffer, 
            DebugModes::DirectionalLightShadows, 
            DebugModes::HDRISkyTextures,
            DebugModes::None
        };

        if (m_debugModes == modes[1])
            m_debugModes = modes[2];
        else if (m_debugModes == modes[2])
            m_debugModes = modes[3];
        else if (m_debugModes == modes[3])
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

        // G-Buffer textures
        m_gBufferTarget->BindTexture(0, 0);
        m_lightingTestShader->SetUniformi("gAlbedoAO", 0);
        m_gBufferTarget->BindTexture(1, 1); 
        m_lightingTestShader->SetUniformi("gNormals", 1);
        m_gBufferTarget->BindTexture(2, 2);
        m_lightingTestShader->SetUniformi("gMetallicRoughness", 2);
        m_gBufferTarget->BindTexture(3, 3);
        m_lightingTestShader->SetUniformi("gEmissive", 3);
        // Camera Depth
        m_gBufferTarget->BindDepthTexture(4);        
        m_lightingTestShader->SetUniformi("gDepth", 4);
        // Shadowmap Depth
        m_graphics->SetActiveTexture(5);
        m_graphics->BindTexture(GL_TEXTURE_2D, m_dlShadowMap->GetShadowMapID());
        m_lightingTestShader->SetUniformi("gDLShadowMap", 5);
        // Sky Textures
        m_graphics->SetActiveTexture(6);
        m_graphics->BindTexture(GL_TEXTURE_CUBE_MAP, m_hdriSky->GetSkyGPUID());
        m_lightingTestShader->SetUniformi("gSkyTexture", 6);
        m_graphics->SetActiveTexture(7);
        m_graphics->BindTexture(GL_TEXTURE_CUBE_MAP, m_hdriSky->GetIrradianceGPUID());
        m_lightingTestShader->SetUniformi("gIrradianceMap", 7);
        m_graphics->SetActiveTexture(8);
        m_graphics->BindTexture(GL_TEXTURE_CUBE_MAP, m_hdriSky->GetPrefilteredGPUID());
        m_lightingTestShader->SetUniformi("gPrefilteredMap", 8);
        m_graphics->SetActiveTexture(9);
        m_graphics->BindTexture(GL_TEXTURE_2D, m_hdriSky->GetBRDFLutGPUID());
        m_lightingTestShader->SetUniformi("gBRDFLUT", 9);

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
        m_graphics->BindVertexArray(m_triangleVAO.GetGPUID());
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
            SetUniformsForGBuffer(material.get());

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