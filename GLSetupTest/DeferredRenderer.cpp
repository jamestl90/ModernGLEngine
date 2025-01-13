#include "DeferredRenderer.h"
#include "Material.h"
#include "Graphics.h"
#include "DirectionalLightShadowMap.h"
#include "HDRISky.h"
#include "UniformBuffer.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include "ImageHelpers.h"

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* resourceLoader, int width, int height, const std::string& assetFolder)
        : m_graphics(graphics), m_resourceLoader(resourceLoader), m_shadowDebugShader(nullptr),
        m_width(width), m_height(height), m_gBufferDebugShader(nullptr), 
        m_assetFolder(assetFolder), m_gBufferTarget(nullptr), m_gBufferShader(nullptr), 
        m_enableDLShadows(true), m_debugModes(DebugModes::None), m_hdriSky(nullptr) 
    {
        m_sceneManager = SceneManager(new Node("SceneRoot", JLEngine::NodeTag::SceneRoot), m_resourceLoader);
    }

    DeferredRenderer::~DeferredRenderer() 
    {
        Graphics::DisposeVertexArray(&m_triangleVAO);
        
        Graphics::DisposeShader(m_shadowDebugShader);
        Graphics::DisposeShader(m_gBufferDebugShader);
        Graphics::DisposeShader(m_gBufferShader);
        Graphics::DisposeShader(m_lightingTestShader);

        Graphics::DisposeRenderTarget(m_gBufferTarget);

        for (auto& [attrib, vaoRes] : m_staticResources)
        {
            Graphics::DisposeGPUBuffer(&vaoRes.drawBuffer->GetGPUBuffer());
            //Graphics::DisposeVertexArray(vaoRes.vao.get());
        }
        for (auto& [attrib, vaoRes] : m_dynamicResources)
        {
            Graphics::DisposeGPUBuffer(&vaoRes.drawBuffer->GetGPUBuffer());
            //Graphics::DisposeVertexArray(vaoRes.vao.get());
        }
    }
    
    void DeferredRenderer::Initialize() 
    {
        m_directionalLight.position = glm::vec3(0, 30.0f, 30.0f);
        m_directionalLight.direction = -glm::normalize(m_directionalLight.position - glm::vec3(0.0f));

        auto sizeOfCamInfo = sizeof(CameraInfo);
        m_cameraUBO.GetGPUBuffer().SetSize(sizeOfCamInfo);
        Graphics::CreateGPUBuffer(m_cameraUBO.GetGPUBuffer());
        GL_CHECK_ERROR();


        auto shaderAssetPath = m_assetFolder + "Core/Shaders/";
        auto textureAssetPath = m_assetFolder + "HDRI/";

        auto dlShader = m_resourceLoader->CreateShaderFromFile("DLShadowMap", "dlshadowmap_vert.glsl", "dlshadowmap_frag.glsl", shaderAssetPath).get();
        m_dlShadowMap = new DirectionalLightShadowMap(m_graphics, dlShader);
        m_dlShadowMap->Initialise();

        SetupGBuffer();
        RTParams rtParams;
        rtParams.internalFormat = GL_RGBA8;
        m_lightOutputTarget = m_resourceLoader->CreateRenderTarget("LightOutputTarget", m_width, m_height, rtParams, DepthType::None, 1).get();
        m_transparentTarget = m_resourceLoader->CreateRenderTarget("TransparentTarget", m_width, m_height, rtParams, DepthType::None, 1).get();
        GL_CHECK_ERROR();
        m_shadowDebugShader = m_resourceLoader->CreateShaderFromFile("DebugDirShadows", "screenspacetriangle.glsl", "/Debug/depth_debug_frag.glsl", shaderAssetPath).get();
        m_gBufferDebugShader = m_resourceLoader->CreateShaderFromFile("DebugGBuffer", "screenspacetriangle.glsl", "/Debug/gbuffer_debug_frag.glsl", shaderAssetPath).get();
        m_gBufferShader = m_resourceLoader->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", shaderAssetPath).get();
        m_lightingTestShader = m_resourceLoader->CreateShaderFromFile("LightingTest", "screenspacetriangle.glsl", "lighting_test_frag.glsl", shaderAssetPath).get();
        m_passthroughShader = m_resourceLoader->CreateShaderFromFile("PassthroughShader", "screenspacetriangle.glsl", "pos_uv_frag.glsl", shaderAssetPath).get();
        m_downsampleShader = m_resourceLoader->CreateShaderFromFile("Downsampling", "screenspacetriangle.glsl", "pos_uv_frag.glsl", shaderAssetPath).get();
        m_blendShader = m_resourceLoader->CreateShaderFromFile("BlendShader", "alpha_blend_vert.glsl", "alpha_blend_frag.glsl", shaderAssetPath).get();
        m_composeFramebufferShader = m_resourceLoader->CreateShaderFromFile("ComposeFramebuffer", "screenspacetriangle.glsl", "compose_fb_frag.glsl", shaderAssetPath).get();
        // compute shader
        m_simpleBlurCompute = m_resourceLoader->CreateComputeFromFile("SimpleBlur", "gaussianblur.glsl", shaderAssetPath + "Compute/").get();
        
        HdriSkyInitParams params;
        params.fileName = "kloofendal_48d_partly_cloudy_puresky_4k.hdr";
        params.irradianceMapSize = 32;
        params.prefilteredMapSize = 128;
        params.prefilteredSamples = 2048;
        params.compressionThreshold = 3.0f;
        params.maxValue = 10000.0f;
        m_hdriSky = new HDRISky(m_resourceLoader);
        m_hdriSky->Initialise(m_assetFolder, params);
        
        m_triangleVAO.SetGPUID(Graphics::API()->CreateVertexArray());
        GL_CHECK_ERROR();
    }

    void DeferredRenderer::InitScreenSpaceTriangle() 
    {
        //const float triangleVertices[] = 
        //{
        //    // Positions        // UVs
        //    -1.0f, -3.0f,       0.0f, 2.0f,  
        //     3.0f,  1.0f,       2.0f, 0.0f,  
        //    -1.0f,  1.0f,       0.0f, 0.0f      
        //};
        //std::vector<float> triVerts(std::begin(triangleVertices), std::end(triangleVertices));
        //
        //// Initialize the VertexBuffer
        //JLEngine::VertexBuffer triangleVBO;
        //triangleVBO.Set(std::move(triVerts));
        //
        //// Add attributes to the VertexArrayObject
        //m_triangleVAO.AddAttribute(JLEngine::AttributeType::POSITION);
        //m_triangleVAO.SetPosCount(2);
        //m_triangleVAO.AddAttribute(JLEngine::AttributeType::TEX_COORD_0);
        //
        //// Calculate the stride and associate the buffer with the VAO
        //m_triangleVAO.CalcStride();
        //m_triangleVAO.SetVertexBuffer(triangleVBO);
        //
        //Graphics::CreateVertexArray(&m_triangleVAO);

        m_triangleVAO.SetGPUID(Graphics::API()->CreateVertexArray());
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        std::vector<RTParams> attributes(4);
        attributes[0] = { GL_RGBA8 };   // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F };        // Normals (RGB) + Cast/Receive Shadows (A)
        attributes[2] = { GL_RGBA8 };      // Metallic (R) + Roughness (G)
        attributes[3] = { GL_RGBA16F };  // Emissive (RGB) + Reserved (A)

        m_gBufferTarget = m_resourceLoader->CreateRenderTarget(
            "GBufferTarget", 
            m_width, 
            m_height, 
            attributes, 
            DepthType::Texture, 
            static_cast<uint32_t>(attributes.size())).get();
    }

    glm::mat4 DeferredRenderer::DirectionalShadowMapPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        glm::mat4 lightSpaceMatrix = GetLightMatrix(m_directionalLight.position,
            m_directionalLight.direction, m_dlShadowMap->GetSize(), 0.01f, m_dlShadowMap->GetShadowDistance());

        m_dlShadowMap->ShadowMapPassSetup(lightSpaceMatrix);

        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));
        
        Graphics::BindGPUBuffer(m_ssboStaticPerDraw.GetGPUBuffer(), 0);

        for (const auto& [key, resource] : m_staticResources)
        {
            if (resource.vao->GetGPUID() == 0) continue;

            DrawGeometry(resource, stride);
        }

        GL_CHECK_ERROR();

        //for (const auto& [key, resource] : m_dynamicResources)
        //{
        //    if (resource.vao->GetGPUID() == 0) continue;
        //    if (resource.vao->GetVBO().Size() > 0)
        //    {
        //        auto size = static_cast<uint32_t>(resource.drawBuffer->GetDrawCommands().size());
        //        glBindVertexArray(resource.vao->GetGPUID());
        //        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resource.drawBuffer->GetGPUID());
        //        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, size, stride);
        //    }
        //}

        return lightSpaceMatrix;
    }

    void DeferredRenderer::GBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        Graphics::API()->BindFrameBuffer(m_gBufferTarget->GetGPUID());

        Graphics::API()->Disable(GL_BLEND);
        Graphics::API()->Enable(GL_DEPTH_TEST);
        Graphics::API()->Disable(GL_BLEND);
        Graphics::API()->SetDepthMask(GL_TRUE);
        Graphics::API()->SetDepthFunc(GL_LEQUAL);
        Graphics::API()->SetViewport(0, 0, m_width, m_height);
        Graphics::API()->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        Graphics::API()->BindShader(m_gBufferShader->GetProgramId());

        Graphics::BindGPUBuffer(m_ssboMaterials.GetGPUBuffer(), 0);
        Graphics::BindGPUBuffer(m_ssboStaticPerDraw.GetGPUBuffer(), 1);
        Graphics::BindGPUBuffer(m_cameraUBO.GetGPUBuffer(), 2);

        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));
        for (const auto& [key, resource] : m_staticResources)
        {
            if (resource.vao->GetGPUID() == 0) continue;

            DrawGeometry(resource, stride);
        }
        
        //for (const auto& [key, resource] : m_dynamicResources)
        //{
        //    if (resource.vao->GetGPUID() == 0) continue;
        //    if (resource.vao->GetVBO().Size() > 0)
        //    {
        //        auto size = static_cast<uint32_t>(resource.drawBuffer->GetDrawCommands().size());
        //        glBindVertexArray(resource.vao->GetGPUID());
        //        glBindBuffer(GL_DRAW_INDIRECT_BUFFER, resource.drawBuffer->GetGPUID());
        //        glMultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, size, stride);
        //    }
        //}
    }

    void DeferredRenderer::Render(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        CameraInfo camInfo;
        camInfo.viewMatrix = viewMatrix;
        camInfo.projMatrix = projMatrix;
        camInfo.cameraPos = glm::vec4(eyePos.x, eyePos.y, eyePos.z, 1.0f);
        double time = static_cast<float>(glfwGetTime());
        camInfo.timeInfo = glm::vec4(time * 0.02f, time * 0.05f, time * 0.2f, time);

        // update camera info
        Graphics::UploadToGPUBuffer(m_cameraUBO.GetGPUBuffer(), camInfo, 0);

        DrawUI();

        // prepare the default framebuffer
        Graphics::API()->BindFrameBuffer(0);
        Graphics::API()->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        auto lightSpaceMatrix = DirectionalShadowMapPass(viewMatrix, projMatrix);

        GBufferPass(viewMatrix, projMatrix);

        if (m_debugModes != DebugModes::None)
        {
            DebugPass(viewMatrix, projMatrix);
        }
        else
        {
            LightPass(eyePos, viewMatrix, projMatrix, lightSpaceMatrix);

            if (m_ssboTransparentPerDraw.GetDataImmutable().empty())
            {
                ImageHelpers::CopyToScreen(m_lightOutputTarget, m_width, m_height, m_passthroughShader);
            }
            else
            {
                TransparencyPass(eyePos, viewMatrix, projMatrix);
            }
            //auto rtPingPong = m_rtPool.RequestRenderTarget(sizeX, sizeY, GL_RGBA8);
            //ImageHelpers::Downsample(m_lightOutputTarget, rtPingPong, m_passthroughShader);
            //ImageHelpers::BlurInPlaceCompute(rtPingPong, m_simpleBlurCompute);
            //ImageHelpers::CopyToDefault(m_lightOutputTarget, m_width, m_height, m_passthroughShader);
            //m_rtPool.ReleaseRenderTarget(rtPingPong);
        }

        m_lastEyePos = eyePos;

        GL_CHECK_ERROR();
    }

    void DeferredRenderer::LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix)
    {
        Graphics::API()->BindFrameBuffer(m_lightOutputTarget->GetGPUID());
        // unbind any depth attachments from the lighting stage
        Graphics::API()->NamedFramebufferTexture(m_lightOutputTarget->GetGPUID(), GL_DEPTH_ATTACHMENT, 0, 0);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);
        Graphics::API()->BindShader(m_lightingTestShader->GetProgramId());
        Graphics::API()->SetViewport(0, 0, m_width, m_height);

        GLuint textures[] =
        {
            m_gBufferTarget->GetTexId(0),         // gAlbedoAO
            m_gBufferTarget->GetTexId(1),         // gNormals
            m_gBufferTarget->GetTexId(2),         // gMetallicRoughness
            m_gBufferTarget->GetTexId(3),         // gEmissive
            m_gBufferTarget->GetDepthBufferId(),     // gDepth
            m_dlShadowMap->GetShadowMapID(),         // gDLShadowMap
            m_hdriSky->GetSkyGPUID(),                // gSkyTexture
            m_hdriSky->GetIrradianceGPUID(),         // gIrradianceMap
            m_hdriSky->GetPrefilteredGPUID(),        // gPrefilteredMap
            m_hdriSky->GetBRDFLutGPUID()             // gBRDFLUT
        };

        Graphics::API()->BindTextures(0, 10, textures);

        const char* textureUniforms[] = {
            "gAlbedoAO", "gNormals", "gMetallicRoughness", "gEmissive", "gDepth",
            "gDLShadowMap", "gSkyTexture", "gIrradianceMap", "gPrefilteredMap", "gBRDFLUT"
        };

        for (int i = 0; i < 10; ++i)
        {
            m_lightingTestShader->SetUniformi(textureUniforms[i], i);
        }

        m_lightingTestShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
        m_lightingTestShader->SetUniform("u_LightDirection", m_directionalLight.direction);
        m_lightingTestShader->SetUniform("u_LightColor", glm::vec3(1.0f, 1.0f, 1.0f)); 
        m_lightingTestShader->SetUniform("u_CameraPos", eyePos);
        m_lightingTestShader->SetUniform("u_ViewInverse", glm::inverse(viewMatrix));
        m_lightingTestShader->SetUniform("u_ProjectionInverse", glm::inverse(projMatrix));

        auto frustum = m_graphics->GetViewFrustum();
        m_lightingTestShader->SetUniformf("u_Near", frustum->GetNear());
        m_lightingTestShader->SetUniformf("u_Far", frustum->GetFar());

        m_lightingTestShader->SetUniformf("u_Bias", m_dlShadowMap->GetBias());
        m_lightingTestShader->SetUniformi("u_PCFKernelSize", m_dlShadowMap->GetPCFKernelSize());

        m_lightingTestShader->SetUniformf("u_SpecularIndirectFactor", m_specularIndirectFactor);
        m_lightingTestShader->SetUniformf("u_DiffuseIndirectFactor", m_diffuseIndirectFactor);

        RenderScreenSpaceTriangle();
    }

    void DeferredRenderer::TransparencyPass(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        RenderBlended(eyePos, viewMat, projMatrix);
        //RenderTransmissive(eyePos, viewMat, projMatrix);
    }

    void DeferredRenderer::RenderBlended(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        Graphics::API()->BindFrameBuffer(m_lightOutputTarget->GetGPUID());
        // bind the gbuffer depth to the target for the transparency pass
        Graphics::API()->NamedFramebufferTexture(m_lightOutputTarget->GetGPUID(), GL_DEPTH_ATTACHMENT, m_gBufferTarget->GetDepthBufferId(), 0);

        Graphics::API()->Enable(GL_BLEND);
        Graphics::API()->SetBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        Graphics::API()->Enable(GL_DEPTH_TEST);
        Graphics::API()->SetDepthMask(GL_FALSE);
        Graphics::API()->SetDepthFunc(GL_LEQUAL);
        Graphics::API()->SetViewport(0, 0, m_width, m_height);

        Graphics::API()->BindShader(m_blendShader->GetProgramId());

        m_blendShader->SetUniformf("u_SpecularIndirectFactor", m_specularIndirectFactor);
        m_blendShader->SetUniformf("u_DiffuseIndirectFactor", m_diffuseIndirectFactor);

        GLuint textures[] =
        {
            m_hdriSky->GetIrradianceGPUID(),         // gIrradianceMap
            m_hdriSky->GetPrefilteredGPUID(),        // gPrefilteredMap
            m_hdriSky->GetBRDFLutGPUID()             // gBRDFLUT
        };

        Graphics::API()->BindTextures(0, 2, textures);

        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));
        auto& vaoRes = m_transparentResources.begin()->second;

        Graphics::BindGPUBuffer(m_ssboMaterials.GetGPUBuffer(), 0);
        Graphics::BindGPUBuffer(m_ssboTransparentPerDraw.GetGPUBuffer(), 1);
        Graphics::BindGPUBuffer(m_cameraUBO.GetGPUBuffer(), 2);

        if (m_lastEyePos != eyePos) // dont need to sort until we move
            m_sceneManager.SortTransparentBackToFront(eyePos);

        DrawGeometry(vaoRes, stride);

        // copy the output to the default framebuffer
        ImageHelpers::CopyToScreen(m_lightOutputTarget, m_width, m_height, m_passthroughShader);

        Graphics::API()->SetDepthMask(GL_TRUE);
        Graphics::API()->Disable(GL_BLEND);
    }

    void DeferredRenderer::RenderTransmissive(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        Graphics::API()->BindShader(m_transmissionShader->GetProgramId());

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

        m_graphics->BindShader(m_passthroughShader->GetProgramId());
        m_graphics->SetActiveTexture(0);
        m_graphics->BindTexture(GL_TEXTURE_2D, brdfLut);
        m_passthroughShader->SetUniformi("u_Texture", 0);

        RenderScreenSpaceTriangle();

        m_graphics->SetViewport(viewports[1]);
        m_hdriSky->Render(viewMatrix, projMatrix, 0);
        m_graphics->SetViewport(viewports[2]);
        m_hdriSky->Render(viewMatrix, projMatrix, 1);
        m_graphics->SetViewport(viewports[3]);
        m_hdriSky->Render(viewMatrix, projMatrix, 2);
    }

    void DeferredRenderer::AddVAO(VAOType vaoType, VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao)
    { 
        auto resource = VAOResource
        {
            vao,
            std::make_shared<IndirectDrawBuffer>() 
        };

        if (vaoType == VAOType::STATIC)
        {
            m_staticResources[key] = resource;
        }
        else if (vaoType == VAOType::DYNAMIC)
        {
            m_dynamicResources[key] = resource;
        }
        else if (vaoType == VAOType::JL_TRANSPARENT)
        {
            m_transparentResources[key] = resource;
        }
    }

    void DeferredRenderer::AddVAOs(VAOType vaoType, std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& vaos)
    {
        for (auto& [key, vao] : vaos)
        {
            auto resource = VAOResource
            {
                vao,
                std::make_shared<IndirectDrawBuffer>()
            };

            if (vaoType == VAOType::STATIC)
            {
                m_staticResources[key] = resource;
            }
            else if (vaoType == VAOType::DYNAMIC)
            {
                m_dynamicResources[key] = resource;
            }
            else if (vaoType == VAOType::JL_TRANSPARENT)
            {
                m_transparentResources[key] = resource;
            }
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

        ImGui::Begin("HDRI Sky Settings");
        ImGui::SliderFloat("Specular Factor", &m_specularIndirectFactor, 0.1f, 2.0f);
        ImGui::SliderFloat("Diffuse Factor", &m_diffuseIndirectFactor, 0.1f, 2.0f);
        ImGui::End();
    }

    void DeferredRenderer::DrawGeometry(const VAOResource& vaoResource, uint32_t stride)
    {
        auto& drawBuffer = vaoResource.drawBuffer;
        m_graphics->BindBuffer(GL_DRAW_INDIRECT_BUFFER, vaoResource.drawBuffer->GetGPUBuffer().GetGPUID());
        auto size = static_cast<uint32_t>(drawBuffer->GetDataImmutable().size());
        m_graphics->BindVertexArray(vaoResource.vao->GetGPUID());
        m_graphics->MultiDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_INT, nullptr, size, stride);
    }

    void DeferredRenderer::DebugPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
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

    void DeferredRenderer::RenderScreenSpaceTriangle() 
    {
        m_graphics->BindVertexArray(m_triangleVAO.GetGPUID());
        m_graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 3);
        m_graphics->BindVertexArray(0);
    }

    void DeferredRenderer::GenerateGPUBuffers()
    {
        if (!m_sceneManager.GetRoot()) return;
        GL_CHECK_ERROR();
        for (auto& [vertexAttrib, vaoresource] : m_staticResources)
        {
            Graphics::CreateVertexArray(vaoresource.vao.get());
        }
        for (auto& [vertexAttrib, vaoresource] : m_dynamicResources)
        {
            Graphics::CreateVertexArray(vaoresource.vao.get());
        }
        for (auto& [vertexAttrib, vaoresource] : m_transparentResources)
        {
            Graphics::CreateVertexArray(vaoresource.vao.get());
        }
        auto matManager = m_resourceLoader->GetMaterialManager();
        auto texManager = m_resourceLoader->GetTextureManager();
        std::vector<MaterialGPU> matBuffer;

        m_materialIDMap.clear();
        GenerateMaterialAndTextureBuffers(*matManager, *texManager, matBuffer, m_materialIDMap);

        m_ssboMaterials.Set(std::move(matBuffer));
        Graphics::CreateGPUBuffer<MaterialGPU>(m_ssboMaterials.GetGPUBuffer(), m_ssboMaterials.GetDataImmutable());

        m_sceneManager.ForceUpdate();
        auto& nonInstancedStatic = m_sceneManager.GetNonInstancedStatic();
        auto& nonInstancedDynamic = m_sceneManager.GetNonInstancedDynamic();
        auto& transparentItems = m_sceneManager.GetTransparent();
        auto& instancedItems = m_sceneManager.GetInstanced();

        for (auto& item : nonInstancedStatic)
        {
            PerDrawData pdd;
            pdd.materialID = (int)m_materialIDMap[item.first.materialHandle];
            pdd.modelMatrix = item.second->GetGlobalTransform();

            m_ssboStaticPerDraw.AddData(pdd);
            m_staticResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);
        }

        int baseInstance = 0;
        for (auto& item : instancedItems)
        {
            auto& transforms = item.second.instanceTransforms;

            int numTransforms = (int)transforms->size();
            item.second.command.instanceCount = numTransforms;
            item.second.command.baseInstance = baseInstance;

            m_staticResources[item.second.attribKey].drawBuffer->AddDrawCommand(item.second.command);

            for (auto i = 0; i < transforms->size(); i++)
            {
                PerDrawData pdd;
                pdd.materialID = (int)m_materialIDMap[item.second.materialHandle];
                transforms->at(i)->UpdateHierarchy();
                pdd.modelMatrix = transforms->at(i)->GetGlobalTransform();
                m_ssboStaticPerDraw.AddData(pdd);
            }

            baseInstance += numTransforms;
        }

        for (auto& item : nonInstancedDynamic)
        {
            PerDrawData pdd;
            pdd.materialID = (int)m_materialIDMap[item.first.materialHandle];
            pdd.modelMatrix = item.second->GetGlobalTransform();

            m_ssboDynamicPerDraw.AddData(pdd);
            m_dynamicResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);
        }

        for (auto& item : transparentItems)
        {
            PerDrawData pdd;
            pdd.materialID = (int)m_materialIDMap[item.first.materialHandle];
            pdd.modelMatrix = item.second->GetGlobalTransform();

            m_ssboTransparentPerDraw.AddData(pdd);
            m_transparentResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);
        }

        for (auto& [vertexAttrib, vaoresource] : m_staticResources)
        {
            Graphics::CreateIndirectDrawBuffer(vaoresource.drawBuffer.get());
        }

        for (auto& [vertexAttrib, vaoresource] : m_dynamicResources)
        {
            Graphics::CreateIndirectDrawBuffer(vaoresource.drawBuffer.get());
        }

        for (auto& [vertexAttrib, vaoresource] : m_transparentResources)
        {
            Graphics::CreateIndirectDrawBuffer(vaoresource.drawBuffer.get());
        }

        Graphics::CreateGPUBuffer<PerDrawData>(m_ssboStaticPerDraw.GetGPUBuffer(), m_ssboStaticPerDraw.GetDataImmutable());
        Graphics::CreateGPUBuffer<PerDrawData>(m_ssboDynamicPerDraw.GetGPUBuffer(), m_ssboDynamicPerDraw.GetDataImmutable());
        Graphics::CreateGPUBuffer<PerDrawData>(m_ssboTransparentPerDraw.GetGPUBuffer(), m_ssboTransparentPerDraw.GetDataImmutable());
        GL_CHECK_ERROR();
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

    void DeferredRenderer::GenerateMaterialAndTextureBuffers(
        ResourceManager<JLEngine::Material>& materialManager, 
        ResourceManager<JLEngine::Texture>& textureManager, 
        std::vector<MaterialGPU>& materialBuffer, 
        std::unordered_map<uint32_t, size_t>& materialIDMap)
    {
        // Map Texture GPUID to indices
        std::unordered_map<uint32_t, uint64_t> textureIDMap;
        size_t textureIndex = 0;

        for (const auto& [id, texture] : textureManager.GetResources())
        {
            textureIDMap[texture->GetGPUID()] = texture->Bindless();
        }

        // Map Material GPUID to indices and build MaterialGPU array
        size_t materialIndex = 0;
        
        for (const auto& [id, material] : materialManager.GetResources())
        {
            MaterialGPU matGPU;
            matGPU.baseColorFactor = material->baseColorFactor;
            matGPU.emissiveFactor = glm::vec4(material->emissiveFactor, 1.0f);
            matGPU.metallicFactor = material->metallicFactor;
            matGPU.roughnessFactor = material->roughnessFactor;
            matGPU.alphaCutoff = material->alphaCutoff;
            matGPU.receiveShadows = material->receiveShadows ? 1 : 0;

            // Map textures by ID
            matGPU.baseColorHandle = material->baseColorTexture ?
                textureIDMap[material->baseColorTexture->GetGPUID()] : 0;

            matGPU.metallicRoughnessHandle = material->metallicRoughnessTexture ?
                textureIDMap[material->metallicRoughnessTexture->GetGPUID()] : 0;

            matGPU.normalHandle = material->normalTexture ?
                textureIDMap[material->normalTexture->GetGPUID()] : 0;

            matGPU.occlusionHandle = material->occlusionTexture ?
                textureIDMap[material->occlusionTexture->GetGPUID()] : 0;

            matGPU.emissiveHandle = material->emissiveTexture ?
                textureIDMap[material->emissiveTexture->GetGPUID()] : 0;

            matGPU.alphaMode = static_cast<int>(material->alphaMode);

            materialBuffer.push_back(matGPU);
            materialIDMap[id] = materialIndex++;
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