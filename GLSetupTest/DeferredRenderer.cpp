#include "DeferredRenderer.h"
#include "Material.h"
#include "Graphics.h"
#include "DirectionalLightShadowMap.h"
#include "HDRISky.h"
#include "UniformBuffer.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <imgui.h>
#include <type_traits>

#include "ImageHelpers.h"
#include "AnimHelpers.h"
#include "ShaderGlobalData.h"
#include "DDGI.h"
#include "PhysicallyBasedSky.h"
#include "JLMath.h"

namespace JLEngine
{
    DeferredRenderer::DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* resourceLoader,
        int width, int height, const std::string& assetFolder)
        : m_graphics(graphics),
        m_resourceLoader(resourceLoader),
        m_width(width),
        m_height(height),
        m_assetFolder(assetFolder),
        m_enableDLShadows(true),
        m_debugModes(DebugModes::None),
        m_im3dManager(nullptr),
        m_ddgi(nullptr),
        m_pbSky(nullptr),

        // Initialize scene manager
        m_sceneManager(SceneManager(new Node("SceneRoot", JLEngine::NodeTag::SceneRoot), m_resourceLoader)),

        // Initialize rendering-related resources
        m_shadowDebugShader(nullptr),
        m_gBufferDebugShader(nullptr),
        m_gBufferShader(nullptr),
        m_blendShader(nullptr),
        m_downsampleShader(nullptr),
        m_lightingTestShader(nullptr),
        m_passthroughShader(nullptr),
        m_skinningGBufferShader(nullptr),
        m_combineShader(nullptr),
        m_transmissionShader(nullptr),
        m_simpleBlurCompute(nullptr),
        m_jointTransformCompute(nullptr),

        // Initialize render targets
        m_lightOutputTarget(nullptr),
        m_gBufferTarget(nullptr),
        m_finalOutputTarget(nullptr),

        m_hdriSky(nullptr),        
        m_dlShadowMap(nullptr),
        m_lastEyePos()

    {
        m_sceneManager = SceneManager(new Node("SceneRoot", JLEngine::NodeTag::SceneRoot), m_resourceLoader);
    }

    DeferredRenderer::~DeferredRenderer() 
    {
        Graphics::DisposeVertexArray(&m_triangleVAO);

        for (auto& [attrib, vaoRes] : m_staticResources)
        {
            Graphics::DisposeGPUBuffer(&vaoRes.drawBuffer->GetGPUBuffer());
        }

        for (auto& [attrib, vaoRes] : m_transparentResources)
        {
            Graphics::DisposeGPUBuffer(&vaoRes.drawBuffer->GetGPUBuffer());
        }

        Graphics::DisposeGPUBuffer(&m_skinnedMeshResources.second.drawBuffer->GetGPUBuffer());

        Graphics::DisposeGPUBuffer(&m_ssboStaticPerDraw.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboInstancedPerDraw.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboDynamicPerDraw.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboTransparentPerDraw.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboMaterials.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboJointMatrices.GetGPUBuffer());
        Graphics::DisposeGPUBuffer(&m_ssboGlobalTransforms.GetGPUBuffer());

        delete m_ddgi;
    }
    
    void DeferredRenderer::EarlyInitialize()
    {
        m_sceneManager.ForceUpdate();

        auto sizeOfCamInfo = sizeof(ShaderGlobalData);
        m_gShaderData.GetGPUBuffer().SetSizeInBytes(sizeOfCamInfo);
        Graphics::CreateGPUBuffer(m_gShaderData.GetGPUBuffer());
        GL_CHECK_ERROR();

        auto shaderAssetPath = m_assetFolder + "Core/Shaders/";
        auto textureAssetPath = m_assetFolder + "HDRI/";
        
        // --- SHADOW MAP --- 
        auto dlShader = m_resourceLoader->CreateShaderFromFile("DLShadowMap", "dlshadowmap_vert.glsl", "dlshadowmap_frag.glsl", shaderAssetPath).get();
        auto dlShaderSkinning = m_resourceLoader->CreateShaderFromFile("DLShadowMapSkinning", "dlshadowmap_skinning_vert.glsl", "dlshadowmap_frag.glsl", shaderAssetPath).get();
        m_dlShadowMap = new DirectionalLightShadowMap(m_graphics, dlShader, dlShaderSkinning);
        m_dlShadowMap->Initialise();

        SetupGBuffer();
        
        // --- RENDER TARGETS --- 
        std::vector<RTParams> lightRTParams(3);
        lightRTParams[0] = { GL_RGB8 };    // direct lighting
        lightRTParams[1] = { GL_RGB8 };    // indirect specular  
        lightRTParams[2] = { GL_RGB8 };
        m_lightOutputTarget = m_resourceLoader->CreateRenderTarget("LightOutputTarget", m_width, m_height, lightRTParams, DepthType::None, 3).get();

        RTParams rtParams;
        rtParams.internalFormat = GL_RGBA8;
        m_finalOutputTarget = m_resourceLoader->CreateRenderTarget("FinalOutputTarget", m_width, m_height, rtParams, DepthType::None, 1).get();

        GL_CHECK_ERROR();

        // --- SHADERS --- 
        m_shadowDebugShader = m_resourceLoader->CreateShaderFromFile("DebugDirShadows", "screenspacetriangle.glsl", "/Debug/depth_debug_frag.glsl", shaderAssetPath).get();
        m_gBufferDebugShader = m_resourceLoader->CreateShaderFromFile("DebugGBuffer", "screenspacetriangle.glsl", "/Debug/gbuffer_debug_frag.glsl", shaderAssetPath).get();
        m_gBufferShader = m_resourceLoader->CreateShaderFromFile("GBuffer", "gbuffer_vert.glsl", "gbuffer_frag.glsl", shaderAssetPath).get();
        m_skinningGBufferShader = m_resourceLoader->CreateShaderFromFile("SkinningGBuffer", "skinning_gbuffer_vert.glsl", "gbuffer_frag.glsl", shaderAssetPath).get();
        m_lightingTestShader = m_resourceLoader->CreateShaderFromFile("LightingTest", "screenspacetriangle.glsl", "lighting_test_frag.glsl", shaderAssetPath).get();
        m_passthroughShader = m_resourceLoader->CreateShaderFromFile("PassthroughShader", "screenspacetriangle.glsl", "pos_uv_frag.glsl", shaderAssetPath).get();
        m_downsampleShader = m_resourceLoader->CreateShaderFromFile("Downsampling", "screenspacetriangle.glsl", "pos_uv_frag.glsl", shaderAssetPath).get();
        m_blendShader = m_resourceLoader->CreateShaderFromFile("BlendShader", "alpha_blend_vert.glsl", "alpha_blend_frag.glsl", shaderAssetPath).get();
        m_combineShader = m_resourceLoader->CreateShaderFromFile("CombineStages", "screenspacetriangle.glsl", "combine_frag.glsl", shaderAssetPath).get();

        // --- COMPUTE --- 
        m_simpleBlurCompute = m_resourceLoader->CreateComputeFromFile("SimpleBlur", "gaussianblur.compute", shaderAssetPath + "Compute/").get();
        m_jointTransformCompute = m_resourceLoader->CreateComputeFromFile("AnimJointTransforms", "joint_transform.compute", shaderAssetPath + "Compute/").get();

        // --- HDR SKY ---
        //HdriSkyInitParams params;
        //params.fileName = "kloofendal_48d_partly_cloudy_puresky_4k.hdr";
        //params.irradianceMapSize = 32;
        //params.prefilteredMapSize = 128;
        //params.prefilteredSamples = 2048;
        //params.compressionThreshold = 3.0f;
        //params.maxValue = 10000.0f;
        //m_hdriSky = new HDRISky(m_resourceLoader);
        //m_hdriSky->Initialise(m_assetFolder, params);

        // --- PB SKY ---
        AtmosphereParams m_atmosphereParams{};
        m_atmosphereParams.sunDir = glm::normalize(glm::vec3(0.0f, 0.1f, -1.0f));
        m_pbSky = new PhysicallyBasedSky(m_resourceLoader, m_assetFolder);
        m_pbSky->Initialise(m_atmosphereParams);

        RTParams skyRTParams;
        skyRTParams.internalFormat = GL_RGB16F;
        m_skyTarget = m_resourceLoader->CreateRenderTarget("SkyOutputTarget", m_width, m_height, skyRTParams, DepthType::None, 1).get();
        
        if (m_ddgi == nullptr)
            m_ddgi = new DDGI(m_resourceLoader, m_assetFolder);
        m_ddgi->GenerateProbes(m_sceneManager.GetSubmeshes());

        // possible not needed now
        m_triangleVAO.SetGPUID(Graphics::API()->CreateVertexArray());

        Graphics::API()->DebugLabelObject(GL_FRAMEBUFFER, m_gBufferTarget->GetGPUID(), "gBuffer");
        Graphics::API()->DebugLabelObject(GL_FRAMEBUFFER, m_lightOutputTarget->GetGPUID(), "lightingTarget");

        GL_CHECK_ERROR();
    }

    void DeferredRenderer::LateInitialize()
    {
        if (m_vgm == nullptr)
            m_vgm = new VoxelGridManager(m_resourceLoader, m_assetFolder);
        m_vgm->Initialise();
        ExtractSceneTriangles();
        m_vgm->Render();
    }

    void DeferredRenderer::SetupGBuffer() 
    {
        // Configure G-buffer render target
        std::vector<RTParams> attributes(6);
        attributes[0] = { GL_RGBA8, GL_LINEAR, GL_LINEAR };           // Albedo (RGB) + AO (A)
        attributes[1] = { GL_RGBA16F, GL_NEAREST, GL_NEAREST };         // Normals (RGB) + Cast/Receive Shadows (A)
        attributes[2] = { GL_RGBA8, GL_LINEAR, GL_LINEAR };           // Metallic (B) + Roughness (G), Height (R), Reserved (A)
        attributes[3] = { GL_RGBA16F, GL_LINEAR, GL_LINEAR };         // Emissive (RGB) + Reserved (A)
        attributes[4] = { GL_RGB32F, GL_NEAREST, GL_NEAREST };          // World Position
        attributes[5] = { GL_R32F, GL_NEAREST, GL_NEAREST };            // manual write out depth

        m_gBufferTarget = m_resourceLoader->CreateRenderTarget(
            "GBufferTarget", 
            m_width, 
            m_height, 
            attributes, 
            DepthType::Texture,                                         // default depth
            static_cast<uint32_t>(attributes.size())).get();
    }
 
    void DeferredRenderer::UpdateRigidAnimations()
    {
        const size_t nonInstancedStaticCount = m_sceneManager.GetNonInstancedStatic().size();
        auto& rigidAnimationNodes = m_sceneManager.GetRigidAnimated();
        auto& dataMutable = m_ssboStaticPerDraw.GetDataMutable();

        for (int i = 0; i < rigidAnimationNodes.size(); i++)
        {
            auto& node = rigidAnimationNodes[i].second;
            auto& controller = node->animController;

            AnimHelpers::EvaluateRigidAnimation(*controller->CurrAnim(), *node, controller->GetTime(), controller->IsLooping(), controller->GetKeyframeIndices());

            size_t perDrawDataIndex = i + nonInstancedStaticCount;
            dataMutable.at(perDrawDataIndex).modelMatrix = node->GetGlobalTransform();
        }

        GPUBuffer& gpuBuffer = m_ssboStaticPerDraw.GetGPUBuffer();

        if (nonInstancedStaticCount >= dataMutable.size()) return;

        size_t perDrawDataSize = sizeof(PerDrawData);
        size_t offset = nonInstancedStaticCount * perDrawDataSize;
        size_t count = dataMutable.size() - nonInstancedStaticCount;
        size_t size = count * perDrawDataSize; 

        void* dataPtr = dataMutable.data() + nonInstancedStaticCount;

        Graphics::API()->NamedBufferSubData(gpuBuffer.GetGPUID(), offset, size, dataPtr);
    }

    void DeferredRenderer::UpdateSkinnedAnimations()
    {
        m_jointMatrices.clear();
        auto& skinnedMeshData = m_sceneManager.GetNonInstancedDynamic();
        for (auto& smd : skinnedMeshData)
        {
            auto& mesh = smd.second->mesh;
            auto& skeleton = smd.second->mesh->GetSkeleton();
            auto& controller = mesh->node->animController;
            float currentTime = controller->GetTime();
            auto& keyframeIndices = controller->GetKeyframeIndices();

            std::vector<glm::mat4> nodeTransforms(skeleton->joints.size(), glm::mat4(1.0f));

            AnimHelpers::EvaluateAnimation(*controller->CurrAnim(), currentTime, nodeTransforms, keyframeIndices);

            std::vector<glm::mat4> globalTransforms;
            AnimHelpers::ComputeGlobalTransforms(*skeleton, nodeTransforms, globalTransforms);

            std::vector<glm::mat4> jointMatrices;
            AnimHelpers::ComputeJointMatrices(globalTransforms, mesh->GetInverseBindMatrices(), jointMatrices);

            m_jointMatrices.insert(m_jointMatrices.end(), jointMatrices.begin(), jointMatrices.end());
        }

        auto& instancedSkinnedMeshData = m_sceneManager.GetInstancedDynamic();
        for (auto& ismd : instancedSkinnedMeshData)
        {
            auto& mesh = ismd.second.second->mesh;
            auto& skeleton = mesh->GetSkeleton();
            auto& controller = mesh->node->animController;

            std::vector<glm::mat4> nodeTransforms(skeleton->joints.size(), glm::mat4(1.0f));

            float currentTime = controller->GetTime();
            auto& keyframeIndices = controller->GetKeyframeIndices();
            AnimHelpers::EvaluateAnimation(*controller->CurrAnim(), currentTime, nodeTransforms, keyframeIndices);

            std::vector<glm::mat4> globalTransforms;
            AnimHelpers::ComputeGlobalTransforms(*skeleton, nodeTransforms, globalTransforms);

            std::vector<glm::mat4> jointMatrices;
            AnimHelpers::ComputeJointMatrices(globalTransforms, mesh->GetInverseBindMatrices(), jointMatrices);

            m_jointMatrices.insert(m_jointMatrices.end(), jointMatrices.begin(), jointMatrices.end());
        }

        if (skinnedMeshData.size() > 0 || instancedSkinnedMeshData.size() > 0)
            Graphics::UploadToGPUBuffer(m_ssboGlobalTransforms.GetGPUBuffer(), m_jointMatrices);
        GL_CHECK_ERROR();
    }

    glm::mat4 DeferredRenderer::DirectionalShadowMapPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& eyePos)
    {
        glm::vec3 currentSunDir = m_atmosphereParams.sunDir;
        glm::mat4 lightSpaceMatrix = GetDirectionalLightSpaceMatrix(currentSunDir,
                                                                    eyePos, 
                                                                    m_dlShadowMap->GetSize(), 
                                                                    0.01f, 
                                                                    m_dlShadowMap->GetShadowDistance());

        m_dlShadowMap->ShadowMapPassSetup(lightSpaceMatrix);

        auto shadowMapShader = m_dlShadowMap->GetShadowMapShader();
        Graphics::API()->BindShader(shadowMapShader->GetProgramId());
        shadowMapShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);

        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));
        
        Graphics::BindGPUBuffer(m_ssboStaticPerDraw.GetGPUBuffer(), 0);

        // --- STATIC MESHES ---
        for (const auto& [key, resource] : m_staticResources)
        {
            if (resource.vao->GetGPUID() == 0) continue;

            DrawGeometry(resource, stride);
        }

        GL_CHECK_ERROR();

        if (m_skinnedMeshResources.first == 0) return lightSpaceMatrix;
        if (m_skinnedMeshResources.second.vao->GetGPUID() != 0)
        {
            auto shadowMapSkinningShader = m_dlShadowMap->GetShadowMapSkinningShader();
            Graphics::API()->BindShader(shadowMapSkinningShader->GetProgramId());
            shadowMapSkinningShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);

            // --- DYNAMIC MESHES ---
            Graphics::BindGPUBuffer(m_ssboDynamicPerDraw.GetGPUBuffer(), 0);
            Graphics::BindGPUBuffer(m_ssboGlobalTransforms.GetGPUBuffer(), 1);

            if (m_skinnedMeshResources.second.vao->GetGPUID() != 0)
                DrawGeometry(m_skinnedMeshResources.second, stride);
        }

        return lightSpaceMatrix;
    }

    void DeferredRenderer::GBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));

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
        Graphics::BindGPUBuffer(m_gShaderData.GetGPUBuffer(), 2);

        // --- STATIC MESHES ---
        for (const auto& [key, resource] : m_staticResources)
        {
            if (resource.vao->GetGPUID() == 0) continue;

            DrawGeometry(resource, stride);
        }

        // --- SKINNING SETUP FOR DYNAMIC MESHES ---
        //int numJoints = (int)m_ssboJointMatrices.GetDataImmutable().size();
        //int workGroupSize = 32; 
        //int numWorkGroups = (numJoints + workGroupSize - 1) / workGroupSize;
        //
        //Graphics::API()->BindShader(m_jointTransformCompute->GetProgramId());
        //Graphics::BindGPUBuffer(m_ssboJointMatrices.GetGPUBuffer(), 0);
        //Graphics::BindGPUBuffer(m_globalTransforms.GetGPUBuffer(), 1);
        //Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
        //Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
        //Graphics::API()->DispatchCompute(numWorkGroups, 1, 1);
        //Graphics::API()->SyncCompute();
        //Graphics::API()->SyncSSBO();

        if (m_skinnedMeshResources.first == 0) return;
        if (m_skinnedMeshResources.second.vao->GetGPUID() != 0)
        {
            // --- DYNAMIC MESHES ---
            Graphics::API()->BindShader(m_skinningGBufferShader->GetProgramId());
            Graphics::BindGPUBuffer(m_ssboMaterials.GetGPUBuffer(), 0);
            Graphics::BindGPUBuffer(m_ssboDynamicPerDraw.GetGPUBuffer(), 1);
            Graphics::BindGPUBuffer(m_gShaderData.GetGPUBuffer(), 2);
            Graphics::BindGPUBuffer(m_ssboGlobalTransforms.GetGPUBuffer(), 3);

            if (m_skinnedMeshResources.second.vao->GetGPUID() != 0)
                DrawGeometry(m_skinnedMeshResources.second, stride);
        }
    }

    void DeferredRenderer::Render(const glm::vec3& eyePos, const glm::vec3& camDir, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, double dt)
    {
        ShaderGlobalData gShaderData{};
        gShaderData.viewMatrix = viewMatrix;
        gShaderData.projMatrix = projMatrix;
        gShaderData.cameraPos = glm::vec4(eyePos.x, eyePos.y, eyePos.z, 1.0f);
        gShaderData.camDir = glm::vec4(camDir, 1.0f);
        double time = static_cast<float>(glfwGetTime());
        gShaderData.timeInfo = glm::vec2(dt, time);
        gShaderData.windowSize = glm::vec2(m_width, m_height);
        gShaderData.frameCount = m_frameCount;

        // update camera info
        Graphics::UploadToGPUBuffer(m_gShaderData.GetGPUBuffer(), gShaderData, 0);

        // prepare the default framebuffer
        Graphics::API()->BindFrameBuffer(0);
        Graphics::API()->ClearColour(0.0f, 0.0f, 0.0f, 0.0f);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        UpdateRigidAnimations();
        UpdateSkinnedAnimations();

        auto lightSpaceMatrix = DirectionalShadowMapPass(viewMatrix, projMatrix, eyePos);
        GBufferPass(viewMatrix, projMatrix);
        DrawSky(eyePos, viewMatrix, projMatrix);

        if (m_debugModes != DebugModes::None) // specialized debug views
        {
            DebugPass(viewMatrix, projMatrix);
        }
        else
        {
            // update GI probes 
            //m_ddgi->Update(
            //    static_cast<float>(dt), 
            //    &m_gShaderData,                     
            //    m_dirLightColor,                    // directional light color
            //    m_hdriSky->GetSkyGPUID(),           // sky cubemap
            //    m_vgm->GetVoxelGrid());    

            // do lighting pass
            LightPass(eyePos, viewMatrix, projMatrix, lightSpaceMatrix);
            CombinePass(viewMatrix, projMatrix);

            DrawUI();
            // renders debug geometry/lines/points etc including imgui 
            RenderDebugTools(viewMatrix, projMatrix);

            if (m_ssboTransparentPerDraw.GetDataImmutable().empty())
            {
                ImageHelpers::CopyToScreen(m_finalOutputTarget, m_width, m_height, m_passthroughShader);
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
        m_frameCount++;

        // wrap the frame count once it gets too big
        if (m_frameCount >= std::numeric_limits<int>::max() - 1)
            m_frameCount = 0;

        Graphics::API()->BindFrameBuffer(0);
        GL_CHECK_ERROR();
    }

    void DeferredRenderer::DrawSky(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        Graphics::API()->SetViewport(0, 0, m_width, m_height);
        Graphics::API()->BindFrameBuffer(m_skyTarget->GetGPUID());
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);

        m_pbSky->RenderSky(viewMatrix, projMatrix, eyePos);
        Graphics::API()->BindFrameBuffer(0);
    }

    void DeferredRenderer::LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix)
    {
        Graphics::API()->BindFrameBuffer(m_lightOutputTarget->GetGPUID());
        // unbind any depth attachments from the lighting stage
        Graphics::API()->NamedFramebufferTexture(m_lightOutputTarget->GetGPUID(), GL_DEPTH_ATTACHMENT, 0, 0);
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);
        Graphics::API()->BindShader(m_lightingTestShader->GetProgramId());
        Graphics::API()->SetViewport(0, 0, m_width, m_height);

        Graphics::BindGPUBuffer(m_gShaderData.GetGPUBuffer(), 4);
        Graphics::BindGPUBuffer(m_ddgi->GetProbeSSBO().GetGPUBuffer(), 7);

        GLuint textures[] =
        {
            m_gBufferTarget->GetTexId(0),           // gAlbedoAO
            m_gBufferTarget->GetTexId(1),           // gNormals
            m_gBufferTarget->GetTexId(2),           // gMetallicRoughness
            m_gBufferTarget->GetTexId(3),           // gEmissive
            m_gBufferTarget->GetDepthBufferId(),    // gDepth
            m_dlShadowMap->GetShadowMapID(),        // gDLShadowMap
            m_gBufferTarget->GetTexId(4),           // world pos
            m_gBufferTarget->GetTexId(5),           // linear depth 
            m_skyTarget->GetTexId(0)                // pbSky
        };

        Graphics::API()->BindTextures(0, 9, textures);

        glm::vec3 currentSunDir = m_atmosphereParams.sunDir;
        m_lightingTestShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
        m_lightingTestShader->SetUniform("u_LightDirection", currentSunDir);
        m_lightingTestShader->SetUniform("u_LightColor", m_dirLightColor);
        m_lightingTestShader->SetUniform("u_ViewInverse", glm::inverse(viewMatrix));
        m_lightingTestShader->SetUniform("u_ProjectionInverse", glm::inverse(projMatrix));

        auto frustum = m_graphics->GetViewFrustum();
        m_lightingTestShader->SetUniformf("u_Near", frustum->GetNear());
        m_lightingTestShader->SetUniformf("u_Far", frustum->GetFar());

        m_lightingTestShader->SetUniformf("u_Bias", m_dlShadowMap->GetBias());
        m_lightingTestShader->SetUniformi("u_PCFKernelSize", m_dlShadowMap->GetPCFKernelSize());

        m_lightingTestShader->SetUniformf("u_SpecularIndirectFactor", m_specularIndirectFactor);
        m_lightingTestShader->SetUniformf("u_DiffuseIndirectFactor", m_diffuseIndirectFactor);
        m_lightingTestShader->SetUniformf("m_DirectFactor", m_directFactor);

        auto& gridRes = m_ddgi->GetGridResolution();
        auto& gridOrigin = m_ddgi->GetGridOrigin();
        auto& probeSpacing = m_ddgi->GetProbeSpacing();
        m_lightingTestShader->SetUniform("u_DDGI_GridResolution", gridRes);
        m_lightingTestShader->SetUniform("u_DDGI_GridCenter", gridOrigin);
        m_lightingTestShader->SetUniform("u_DDGI_ProbeSpacing", probeSpacing);

        glm::vec3 gridResolutionVec = glm::vec3(gridRes); 
        glm::vec3 totalGridWorldSize = gridResolutionVec * probeSpacing;
        glm::vec3 gridHalfSizeWorld = totalGridWorldSize / 2.0f;
        glm::vec3 gridMinCornerWorldPos = gridOrigin - gridHalfSizeWorld;
        m_lightingTestShader->SetUniform("u_GridMinCorner", gridMinCornerWorldPos);

        RenderScreenSpaceTriangle();
    }

    void DeferredRenderer::CombinePass(const glm::mat4& viewMat, const glm::mat4& projMat)
    {
        Graphics::API()->BindFrameBuffer(m_finalOutputTarget->GetGPUID());
        Graphics::API()->Clear(GL_COLOR_BUFFER_BIT);
        Graphics::API()->BindShader(m_combineShader->GetProgramId());
        Graphics::API()->SetViewport(0, 0, m_width, m_height);
         
        GLuint textures[] =
        {            
            m_lightOutputTarget->GetTexId(0),
            m_lightOutputTarget->GetTexId(1),
            m_lightOutputTarget->GetTexId(2),
        };

        Graphics::API()->BindTextures(0, 3, textures);

        RenderScreenSpaceTriangle();
    }

    void DeferredRenderer::TransparencyPass(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        RenderBlended(eyePos, viewMat, projMatrix);
        //RenderTransmissive(eyePos, viewMat, projMatrix);
    }

    void DeferredRenderer::RenderBlended(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        if (m_ssboTransparentPerDraw.GetDataImmutable().empty()) return;

        Graphics::API()->BindFrameBuffer(m_finalOutputTarget->GetGPUID());
        // bind the gbuffer depth to the target for the transparency pass
        Graphics::API()->NamedFramebufferTexture(m_finalOutputTarget->GetGPUID(), GL_DEPTH_ATTACHMENT, m_gBufferTarget->GetDepthBufferId(), 0);

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
            m_skyTarget->GetTexId(0)
            //m_hdriSky->GetIrradianceGPUID(),         // gIrradianceMap
            //m_hdriSky->GetPrefilteredGPUID(),        // gPrefilteredMap
            //m_hdriSky->GetBRDFLutGPUID()             // gBRDFLUT
        };

        Graphics::API()->BindTextures(0, 1, textures);

        auto stride = static_cast<uint32_t>(sizeof(JLEngine::DrawIndirectCommand));
        auto& vaoRes = m_transparentResources.begin()->second;

        Graphics::BindGPUBuffer(m_ssboMaterials.GetGPUBuffer(), 0);
        Graphics::BindGPUBuffer(m_ssboTransparentPerDraw.GetGPUBuffer(), 1);
        Graphics::BindGPUBuffer(m_gShaderData.GetGPUBuffer(), 2);

        if (m_lastEyePos != eyePos) // dont need to sort until we move
            m_sceneManager.SortTransparentBackToFront(eyePos);

        DrawGeometry(vaoRes, stride);

        // copy the output to the default framebuffer
        ImageHelpers::CopyToScreen(m_finalOutputTarget, m_width, m_height, m_passthroughShader);

        Graphics::API()->SetDepthMask(GL_TRUE);
        Graphics::API()->Disable(GL_BLEND);
    }

    void DeferredRenderer::RenderTransmissive(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix)
    {
        Graphics::API()->BindShader(m_transmissionShader->GetProgramId());
        // -- TO DO -- 
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

    // draws probe positions
    void DeferredRenderer::DebugDDGI()
    {
        for (const auto& probe : m_ddgi->GetProbeSSBO().GetDataImmutable())
        {
            if (probe._padding.x > 0.5f)
                Im3d::PushColor(Im3d::Color_Red);
            else
                Im3d::PushColor(Im3d::Color_Green);
            const glm::vec3 center = glm::vec3(probe.WorldPosition);
            Im3d::DrawSphere(Im3d::Vec3(center.x, center.y, center.z), 0.15f);
            Im3d::PopColor();
        }       
    }

    void DeferredRenderer::DebugPbrSky() // Or maybe name it UpdateAndRenderAtmosphereUI
    {
        if (!m_pbSky) return; // Ensure sky object exists

        ImGui::Begin("Atmosphere Settings");

        bool paramsChanged = false; // Track if any parameter was changed this frame

        // Temporary storage for atmosphere height
        static float atmosphereHeightKM = m_atmosphereParams.topRadiusKM - m_atmosphereParams.bottomRadiusKM;

        // --- Sun ---
        if (ImGui::CollapsingHeader("Sun Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::ColorEdit3("Solar Irradiance", &m_atmosphereParams.solarIrradiance[0], ImGuiColorEditFlags_Float)) 
            {
                paramsChanged = true;
            }

            if (ImGui::DragFloat("Sun Angular Radius", &m_atmosphereParams.sunAngularRadius, 0.0001f, 0.0f, 0.1f, "%.6f")) 
            {
                paramsChanged = true;
            }

            // Control Sun Direction via Azimuth/Elevation
            ImGui::Text("Sun Direction:");
            bool sunAngleChanged = false;
            if (ImGui::SliderFloat("Azimuth (Deg)", &m_uiSunAzimuthDegrees, 0.0f, 360.0f)) sunAngleChanged = true;
            if (ImGui::SliderFloat("Elevation (Deg)", &m_uiSunElevationDegrees, -90.0f, 90.0f)) sunAngleChanged = true;

            if (sunAngleChanged) 
            {
                paramsChanged = true;
                float azimuthRad = glm::radians(m_uiSunAzimuthDegrees);
                float elevationRad = glm::radians(m_uiSunElevationDegrees);
                m_atmosphereParams.sunDir.x = cos(elevationRad) * sin(azimuthRad);
                m_atmosphereParams.sunDir.y = sin(elevationRad);
                m_atmosphereParams.sunDir.z = cos(elevationRad) * cos(azimuthRad);
                m_atmosphereParams.sunDir = glm::normalize(m_atmosphereParams.sunDir);
            }
            ImGui::Text(" -> Vec3: (%.3f, %.3f, %.3f)", m_atmosphereParams.sunDir.x, m_atmosphereParams.sunDir.y, m_atmosphereParams.sunDir.z);

            if (ImGui::DragFloat("Exposure", &m_atmosphereParams.exposure, 0.1f, 0.1f, 100.0f, "%.1f", ImGuiSliderFlags_Logarithmic)) 
            {
                paramsChanged = true;
            }
        }

        // --- Earth / Planet ---
        if (ImGui::CollapsingHeader("Planet Settings", ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::DragFloat("Bottom Radius (km)", &m_atmosphereParams.bottomRadiusKM, 1.0f, 1.0f, 10000.0f, "%.1f")) 
            {
                paramsChanged = true;
                m_atmosphereParams.topRadiusKM = m_atmosphereParams.bottomRadiusKM + atmosphereHeightKM;
            }

            if (ImGui::DragFloat("Atmosphere Height (km)", &atmosphereHeightKM, 1.0f, 1.0f, 500.0f, "%.1f")) 
            {
                paramsChanged = true;
                m_atmosphereParams.topRadiusKM = m_atmosphereParams.bottomRadiusKM + atmosphereHeightKM;
            }
            ImGui::Text(" -> Top Radius: %.1f km", m_atmosphereParams.topRadiusKM);


            if (ImGui::ColorEdit3("Ground Albedo", &m_atmosphereParams.groundAlbedo[0], ImGuiColorEditFlags_Float)) {
                paramsChanged = true;
            }
        }

        // --- Rayleigh Scattering ---
        if (ImGui::CollapsingHeader("Rayleigh Scattering"))
        {
            if (ImGui::ColorEdit3("Scattering Coeff", &m_atmosphereParams.rayleighScatteringCoeff[0], ImGuiColorEditFlags_Float)) {
                paramsChanged = true;
            }

            if (ImGui::DragFloat("Density Height Scale", &m_atmosphereParams.rayleighDensityHeightScale, 0.1f, 0.1f, 20.0f, "%.1f km")) {
                paramsChanged = true;
            }
        }

        // --- Mie Scattering ---
        if (ImGui::CollapsingHeader("Mie Scattering"))
        {
            if (ImGui::DragFloat3("Scattering Coeff", &m_atmosphereParams.mieScatteringCoeff[0], 0.0001f, 0.0f, 0.1f, "%.6f")) {
                paramsChanged = true;
            }

            if (ImGui::DragFloat3("Extinction Coeff", &m_atmosphereParams.mieExtinctionCoeff[0], 0.0001f, 0.0f, 0.1f, "%.6f")) {
                paramsChanged = true;
            }

            if (ImGui::DragFloat("Density Height Scale", &m_atmosphereParams.mieDensityHeightScale, 0.1f, 0.1f, 5.0f, "%.1f km")) {
                paramsChanged = true;
            }

            if (ImGui::SliderFloat("Phase Func 'g'", &m_atmosphereParams.miePhaseG, -0.99f, 0.99f, "%.2f")) {
                paramsChanged = true;
            }
        }

        // --- Absorption (Ozone) ---
        if (ImGui::CollapsingHeader("Absorption (Ozone)"))
        {
            if (ImGui::ColorEdit3("Absorption Extinction", &m_atmosphereParams.absorptionExtinction[0], ImGuiColorEditFlags_Float)) {
                paramsChanged = true;
            }

            if (ImGui::DragFloat("Absorption Density Scale", &m_atmosphereParams.absorptionDensityHeightScale, 0.1f, 0.1f, 50.0f, "%.1f km")) {
                paramsChanged = true;
            }
        }

        // --- Update GPU ---
        if (paramsChanged)
        {
            m_pbSky->UpdateParams(m_atmosphereParams);
        }

        ImGui::End();
    }

    // draws probe rays
    void DeferredRenderer::DebugDDGIRays()
    {
        auto& dataSSBO = m_ddgi->GetDebugRays();

        DebugRay* rays = static_cast<DebugRay*>(Graphics::API()->MapNamedBuffer(dataSSBO.GetGPUBuffer().GetGPUID(), GL_READ_ONLY));

        for (int i = 0; i < m_ddgi->GetDebugRayCount(); ++i)
        {
            const DebugRay& ray = rays[i];

            // if they are the same dont render (used to hide invalid rays)
            if (glm::distance(ray.Origin, ray.Hit) < 0.1)
                continue;

            auto imOrigin = Math::Convert(ray.Origin);
            auto imHit = Math::Convert(ray.Hit);
            auto col = Math::Convert(ray.Color);
            Im3d::DrawLine(imOrigin, imHit, 10.0f, Im3d::Color(ray.Color.x, ray.Color.y, ray.Color.z));

            if (glm::distance(ray.Origin, ray.Hit) > 0.01f)
            {
                Im3d::DrawPoint(imHit, 15.0f, Im3d::Color_Blue);
            }
        }

        Graphics::API()->UnmapNamedBuffer(dataSSBO.GetGPUBuffer().GetGPUID());
    }

    void DeferredRenderer::DebugAABB()
    {
        const auto& submeshNodes = m_sceneManager.GetSubmeshes();

        Im3d::PushColor(Im3d::Color_Yellow);

        for (const auto& smNodePair : submeshNodes)
        {
            auto& localAABB = smNodePair.first.aabb;
            auto worldTransform = smNodePair.second->GetGlobalTransform();
            glm::vec3 corners[8] =
            {
                { localAABB.min.x, localAABB.min.y, localAABB.min.z },
                { localAABB.min.x, localAABB.min.y, localAABB.max.z },
                { localAABB.min.x, localAABB.max.y, localAABB.min.z },
                { localAABB.min.x, localAABB.max.y, localAABB.max.z },
                { localAABB.max.x, localAABB.min.y, localAABB.min.z },
                { localAABB.max.x, localAABB.min.y, localAABB.max.z },
                { localAABB.max.x, localAABB.max.y, localAABB.min.z },
                { localAABB.max.x, localAABB.max.y, localAABB.max.z }
            };

            glm::vec3 transformedMin = glm::vec3(worldTransform * glm::vec4(corners[0], 1.0f));
            glm::vec3 transformedMax = transformedMin;

            for (int i = 1; i < 8; ++i)
            {
                glm::vec3 transformed = glm::vec3(worldTransform * glm::vec4(corners[i], 1.0f));
                transformedMin = glm::min(transformedMin, transformed);
                transformedMax = glm::max(transformedMax, transformed);
            }

            glm::vec3 center = (transformedMin + transformedMax) * 0.5f;
            glm::vec3 halfExtents = (transformedMax - transformedMin) * 0.5f;

            m_im3dManager->DrawBox(center, halfExtents, Im3d::Color_Red);
        }

        Im3d::PopColor();
    }

    void DeferredRenderer::RenderDebugTools(const glm::mat4& viewMatrix, const glm::mat4& projMatrix)
    {
        DebugPbrSky();
        if (ImGui::Begin("Debug DDGI"))
        {
            ImGui::Checkbox("Show DDGI Probes", &m_showDDGI);
            ImGui::Checkbox("Show AABBs", &m_showAABB);
            ImGui::Checkbox("Show Probe Rays", &m_showDDGIRays);

            float& blendFac = m_ddgi->GetBlendFactorMutable();
            ImGui::SliderFloat("Blend Factor", &blendFac, 0.1f, 1.0f);

            float& skyBlendFac = m_ddgi->GetSkyLightColBlendFacMutable();
            ImGui::SliderFloat("Sky/LightColour Blend Factor", &skyBlendFac, 0.0f, 1.0f);

            int maxIndex = static_cast<int>(m_ddgi->GetProbeSSBO().GetDataImmutable().size()) - 1;
            int& probeIndex = m_ddgi->GetDebugProbeIndexMutable();
            ImGui::SliderInt("Debug Probe Index", &probeIndex, 0, std::max(0, maxIndex));

            float& rayLen = m_ddgi->GetRayLengthMutable();
            ImGui::SliderFloat("Max Ray Distance", &rayLen, 0.5f, 10.0f);

            //int& raysPerProbe = m_ddgi->GetRaysPerProbeMutable();
            //ImGui::SliderInt("Rays Per Probe", &raysPerProbe, 16, 512);
        }
        if (ImGui::Checkbox("Show Voxel Grid Bounds", &m_showVoxelGridBounds)) {} // Add a bool member
        if (m_showVoxelGridBounds && m_vgm) 
        {
            VoxelGrid& vg = m_vgm->GetVoxelGrid();
            glm::vec3 center = vg.worldOrigin; // Origin is often the center
            // If origin is min corner, calculate center: center = vg.worldOrigin + vg.worldSize * 0.5f;
            glm::vec3 halfExtents = vg.worldSize * 0.5f;
            m_im3dManager->DrawBox(center, halfExtents, Im3d::Color_Cyan);
        }
        ImGui::End();

        if (m_showDDGI) { DebugDDGI(); }
        if (m_showAABB) { DebugAABB(); }
        if (m_showDDGIRays) { DebugDDGIRays(); }

        // render im3d here
        if (m_im3dManager != nullptr)
        {
            m_im3dManager->EndFrameAndRender(projMatrix * viewMatrix);
        }
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
            m_skinnedMeshResources = std::make_pair(key, resource);
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
            //DebugModes::HDRISkyTextures,
            DebugModes::PbrSky,
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

    DDGI* DeferredRenderer::GetDDGI()
    {
        if (m_ddgi == nullptr)
        {
            m_ddgi = new DDGI(m_resourceLoader, m_assetFolder);
        }
        return m_ddgi;
    }

    VoxelGridManager* DeferredRenderer::GetVoxelGridManager() {
        if (m_vgm == nullptr)
        {
            m_vgm = new VoxelGridManager(m_resourceLoader, m_assetFolder);
        }
        return m_vgm;
    }

    void DeferredRenderer::DrawUI()
    {
        static float lightAngle = 0.0f;  // Rotation angle in degrees
        static float lightRadius = 30.0f; // Distance from the center
        ImGui::Begin("Shadow Controls");
        // if (ImGui::SliderFloat("Light Rotation", &lightAngle, 0.0f, 360.0f, "%.1f degrees"))
        // {
        //     float radians = glm::radians(lightAngle);
        //     m_directionalLight.position = glm::vec3(
        //         lightRadius * cos(radians),
        //         30.0f,
        //         lightRadius * sin(radians)
        //     );
        //     m_directionalLight.direction = glm::normalize(-m_directionalLight.position);
        // }
        ImGui::SliderFloat("Bias", &m_dlShadowMap->GetBias(), 0.00020f, 0.0009f, "%.6f");
        ImGui::SliderFloat("Distance", &m_dlShadowMap->GetShadowDistance(), 10.0, 200.0f, "%.6f");
        ImGui::SliderFloat("Size", &m_dlShadowMap->GetSize(), 10.0, 50.0f, "%.6f");
        ImGui::SliderInt("PCF Kernel Size", &m_dlShadowMap->GetPCFKernelSize(), 0, 5);
        ImGui::End();

        ImGui::Begin("Light Settings");
        ImGui::SliderFloat("Specular Factor", &m_specularIndirectFactor, 0.1f, 3.0f);
        ImGui::SliderFloat("Diffuse Factor", &m_diffuseIndirectFactor, 0.1f, 3.0f);
        ImGui::SliderFloat("Direct Factor", &m_directFactor, 0.1f, 3.0f);

        ImVec4 imColor = ImVec4(m_dirLightColor.x, m_dirLightColor.y, m_dirLightColor.z, 1.0f);
        ImGuiColorEditFlags flags = ImGuiColorEditFlags_NoAlpha |
                                    ImGuiColorEditFlags_Float |
                                    ImGuiColorEditFlags_PickerHueWheel;

        if (ImGui::ColorEdit3("Light Colour", &imColor.x, flags))
        {
            m_dirLightColor = glm::vec3(imColor.x, imColor.y, imColor.z);
        }
        ImGui::End();
    }
    
    void DeferredRenderer::DrawGeometry(const VAOResource& vaoResource, uint32_t stride)
    {
        auto& drawBuffer = vaoResource.drawBuffer;
        m_graphics->BindBuffer(GL_DRAW_INDIRECT_BUFFER, drawBuffer->GetGPUBuffer().GetGPUID());
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
        //else if (m_debugModes == DebugModes::HDRISkyTextures)
        //{
        //    debugString = "Bottom Left to Top Right:\nBRDF, HDR Sky, Irradiance, Prefiltered";
        //    DebugHDRISky(viewMatrix, projMatrix);
        //}
        else if (m_debugModes == DebugModes::PbrSky)
        {

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
        // --- CREATE VERTEX ARRAY OBJECTS ---
        if (!m_sceneManager.GetRoot()) return;
        for (auto& [vertexAttrib, vaoresource] : m_staticResources)
        {
            Graphics::CreateVertexArray(vaoresource.vao.get());
        }

        if (m_skinnedMeshResources.first != 0)
            Graphics::CreateVertexArray(m_skinnedMeshResources.second.vao.get());

        for (auto& [vertexAttrib, vaoresource] : m_transparentResources)
        {
            Graphics::CreateVertexArray(vaoresource.vao.get());
        }

        // --- CREATE MATERIAL AND TEXTURE BUFFERS ---
        auto matManager = m_resourceLoader->GetMaterialManager();
        auto texManager = m_resourceLoader->GetTextureManager();
        std::vector<MaterialGPU> matBuffer;

        m_materialIDMap.clear();
        GenerateMaterialAndTextureBuffers(*matManager, *texManager, matBuffer, m_materialIDMap);

        m_ssboMaterials.Set(std::move(matBuffer));
        Graphics::CreateGPUBuffer<MaterialGPU>(m_ssboMaterials.GetGPUBuffer(), m_ssboMaterials.GetDataImmutable());

        // --- CONVERT SCENE GRAPH DATA TO RENDER FRIENDLY DATA
        m_sceneManager.ForceUpdate();
        auto& nonInstancedStatic = m_sceneManager.GetNonInstancedStatic();
        auto& nonInstancedDynamic = m_sceneManager.GetNonInstancedDynamic();
        auto& transparentItems = m_sceneManager.GetTransparent();
        auto& instancedStaticItems = m_sceneManager.GetInstancedStatic();
        auto& instancedDynamicItems = m_sceneManager.GetInstancedDynamic();
        auto& rigidAnimationItems = m_sceneManager.GetRigidAnimated();

        // --- STATIC MESHES --- 
        //int perDrawDataIndex = 0;
        for (auto& item : nonInstancedStatic)
        {
            PerDrawData pdd{};
            pdd.materialID = static_cast<uint32_t>(m_materialIDMap[item.first.materialHandle]);
            pdd.modelMatrix = item.second->GetGlobalTransform();

            //item.second->perDrawDataIndex = perDrawDataIndex++;
            m_ssboStaticPerDraw.AddData(pdd);
            m_staticResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);

            m_staticRigidAnimationIndex++;
        }
        // add the rigid animations to the back of the vao
        // do not reset perDrawDataIndex as they should follow on
        for (auto& item : rigidAnimationItems)
        {
            PerDrawData pdd{};
            pdd.materialID = static_cast<uint32_t>(m_materialIDMap[item.first.materialHandle]);
            pdd.modelMatrix = item.second->GetGlobalTransform();

            m_staticRigidAnimationIndex++;
            //item.second->perDrawDataIndex = perDrawDataIndex++;
            m_ssboStaticPerDraw.AddData(pdd);
            m_staticResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);
        }

        // --- INSTANCED STATIC MESHES --- 
        // perDrawDataIndex = 0;
        uint32_t baseInstance = 0;
        for (auto& item : instancedStaticItems)
        {
            auto& submesh = item.second.first;
            auto& node = item.second.second;
            auto& transforms = submesh.instanceTransforms;

            int numTransforms = static_cast<int>(transforms->size());
            submesh.command.instanceCount = numTransforms;
            submesh.command.baseInstance = baseInstance;

            m_staticResources[submesh.attribKey].drawBuffer->AddDrawCommand(submesh.command);

            for (auto i = 0; i < transforms->size(); i++)
            {
                PerDrawData pdd{};
                pdd.materialID = static_cast<uint32_t>(m_materialIDMap[submesh.materialHandle]);
                //transforms->at(i)->perDrawDataIndex = perDrawDataIndex++;
                transforms->at(i)->UpdateHierarchy();
                pdd.modelMatrix = transforms->at(i)->GetGlobalTransform();
                m_ssboStaticPerDraw.AddData(pdd);
            }
            baseInstance += numTransforms;
        }

        // --- INSTANCED SKINNED MESHES --- 
        baseInstance = 0;
        //perDrawDataIndex = 0;
        uint32_t instancedJointCount = 0;
        for (auto& item : instancedDynamicItems)
        {
            auto& submesh = item.second.first;
            auto node = item.second.second;
            auto& skeleton = node->mesh->GetSkeleton();
            auto& transforms = submesh.instanceTransforms;

            int numTransforms = static_cast<int>(transforms->size());
            submesh.command.instanceCount = numTransforms;
            submesh.command.baseInstance = baseInstance;

            m_skinnedMeshResources.second.drawBuffer->AddDrawCommand(submesh.command);

            for (auto& joint : skeleton->joints)
            {
                m_ssboJointMatrices.AddData(joint);
            }

            for (auto i = 0; i < transforms->size(); i++)
            {
                SkinnedMeshPerDrawData pdd{};
                pdd.baseJointIndex = instancedJointCount; // all instances will share the same joint data
                pdd.materialID = static_cast<uint32_t>(m_materialIDMap[submesh.materialHandle]);
                //transforms->at(i)->perDrawDataIndex = perDrawDataIndex++;
                transforms->at(i)->UpdateHierarchy();
                pdd.modelMatrix = transforms->at(i)->GetGlobalTransform();
                m_ssboDynamicPerDraw.AddData(pdd);
            }
            baseInstance += numTransforms;
            instancedJointCount += static_cast<int>(skeleton->joints.size());
        }

        // --- SKINNED MESHES --- 
        uint32_t jointCount = 0;
        //perDrawDataIndex = 0;
        for (auto& item : nonInstancedDynamic)
        {
            SkinnedMeshPerDrawData pdd{};
            pdd.materialID = static_cast<uint32_t>(m_materialIDMap[item.first.materialHandle]);
            pdd.modelMatrix = item.second->GetGlobalTransform();
            pdd.baseJointIndex = jointCount;

            //item.second->perDrawDataIndex = perDrawDataIndex++;
            m_ssboDynamicPerDraw.AddData(pdd);
            m_skinnedMeshResources.second.drawBuffer->AddDrawCommand(item.first.command);

            auto& mesh = item.second->mesh;
            for (auto& joint : mesh->GetSkeleton()->joints)
            {
                m_ssboJointMatrices.AddData(joint);
            }

            jointCount += static_cast<int>(mesh->GetSkeleton()->joints.size());
        }

        // --- TRANSPARENT MESHES --- 
        //perDrawDataIndex = 0;
        for (auto& item : transparentItems)
        {
            PerDrawData pdd{};
            pdd.materialID = static_cast<uint32_t>(m_materialIDMap[item.first.materialHandle]);
            pdd.modelMatrix = item.second->GetGlobalTransform();
            //item.second->perDrawDataIndex = perDrawDataIndex++;
            m_ssboTransparentPerDraw.AddData(pdd);
            m_transparentResources[item.first.attribKey].drawBuffer->AddDrawCommand(item.first.command);
        }
        GL_CHECK_ERROR();
        // --- CREATE THE GPU DRAW BUFFERS ---
        for (auto& [vertexAttrib, vaoresource] : m_staticResources)
        {
            Graphics::CreateIndirectDrawBuffer(vaoresource.drawBuffer.get());
        }

        if (m_skinnedMeshResources.first != 0)
            Graphics::CreateIndirectDrawBuffer(m_skinnedMeshResources.second.drawBuffer.get());

        for (auto& [vertexAttrib, vaoresource] : m_transparentResources)
        {
            Graphics::CreateIndirectDrawBuffer(vaoresource.drawBuffer.get());
        }
        GL_CHECK_ERROR();
        m_ssboGlobalTransforms.GetGPUBuffer().SetSizeInBytes((jointCount + instancedJointCount) * sizeof(glm::mat4));
        Graphics::CreateGPUBuffer(m_ssboJointMatrices.GetGPUBuffer(), m_ssboJointMatrices.GetDataImmutable());
        GL_CHECK_ERROR();
        Graphics::CreateGPUBuffer(m_ssboGlobalTransforms.GetGPUBuffer());
        GL_CHECK_ERROR();
        Graphics::CreateGPUBuffer<PerDrawData>(m_ssboStaticPerDraw.GetGPUBuffer(), m_ssboStaticPerDraw.GetDataImmutable());
        GL_CHECK_ERROR();
        Graphics::CreateGPUBuffer<SkinnedMeshPerDrawData>(m_ssboDynamicPerDraw.GetGPUBuffer(), m_ssboDynamicPerDraw.GetDataImmutable());
        Graphics::CreateGPUBuffer<PerDrawData>(m_ssboTransparentPerDraw.GetGPUBuffer(), m_ssboTransparentPerDraw.GetDataImmutable());
        GL_CHECK_ERROR();
    }

    void DeferredRenderer::ExtractSceneTriangles()
    {
        std::cout << "PrepareTriangleSSBO: Starting triangle extraction (with Emission)..." << std::endl;
        std::vector<TriWithEmisison> trianglesForSSBO;
        // Optional: Reserve space based on previous frame's count if available
        // allVerticesForSSBO.reserve(m_vgm->GetTriangleCountEstimate() * 3);
        uint32_t totalTrianglesExtracted = 0;

        // Helper function to look up VAOResource (ensure m_*Resources maps are accessible)
        auto findVAOResource = [&](VertexAttribKey key, bool isSkinned, bool isTransparent) -> VAOResource* {
            if (isTransparent) {
                auto it = m_transparentResources.find(key);
                return (it != m_transparentResources.end()) ? &it->second : nullptr;
            }
            else if (isSkinned) {
                bool resourceHasSkinAttribs = HasVertexAttribKey(key, AttributeType::JOINT_0);
                return (m_skinnedMeshResources.first == key && resourceHasSkinAttribs) ? &m_skinnedMeshResources.second : nullptr;
            }
            else {
                auto it = m_staticResources.find(key);
                return (it != m_staticResources.end()) ? &it->second : nullptr;
            }
            return nullptr;
            };

        // Lambda to process a list of SubMesh/Node pairs
        auto processNodeList =
            [&](const auto& nodeList, ResourceManager<Material>& materialManager)
            {
                for (const auto& item : nodeList) 
                {
                    const SubMesh* pSubmesh = nullptr;
                    Node* pNode = nullptr;
                    
                    pSubmesh = &item.first;
                    pNode = item.second;

                    if (!pSubmesh || !pNode) continue; // Safety check

                    const SubMesh& submesh = *pSubmesh;
                    Node* node = pNode;

                    // *** GET MATERIAL EMISSION ***
                    glm::vec3 emissiveColor = glm::vec3(0.0f); // Default to black
                    Material* pMaterial = materialManager.Get(submesh.materialHandle).get();
                    if (pMaterial) {
                        // Use the material's emissive factor.
                        // Ignore emissive texture for simplicity in voxel grid for now.
                        emissiveColor = pMaterial->emissiveFactor;
                    }
                    else {
                        std::cerr << "Warning: Could not find material with handle "
                            << submesh.materialHandle << " for submesh on node "
                            << node->name << std::endl;
                    }
                    // *** END GET MATERIAL EMISSION ***


                    bool isSkinnedVAO = HasVertexAttribKey(submesh.attribKey, AttributeType::JOINT_0);
                    bool isTransparent = (submesh.flags & SubmeshFlags::USES_TRANSPARENCY) != 0;

                    // Skip transparent materials for voxelization (usually desired)
                    if (isTransparent) {
                        continue;
                    }

                    VAOResource* vaoResourcePtr = findVAOResource(submesh.attribKey, isSkinnedVAO, isTransparent);
                    if (!vaoResourcePtr || !vaoResourcePtr->vao) continue; // Skip if no VAO

                    VertexArrayObject& vao = *vaoResourcePtr->vao;
                    VertexBuffer& vbo = vao.GetVBO();
                    IndexBuffer& ibo = vao.GetIBO();
                    if (vao.GetStride() == 0) vao.CalcStride();
                    size_t vertexStride = vao.GetStride(); // Ensure stride is calculated
                    size_t positionOffset = 0; // Assuming position is attribute 0

                    if (vertexStride == 0) continue; // Skip if bad stride

                    const std::vector<std::byte>& vertexDataBytes = vbo.GetDataImmutable();
                    const std::vector<uint32_t>& indexData = ibo.GetDataImmutable();
                    const DrawIndirectCommand& command = submesh.command;

                    if (vertexDataBytes.empty() || (indexData.empty() && command.count > 0)) continue; // Skip if no data

                    const std::byte* vertexDataPtr = vertexDataBytes.data();
                    const size_t vertexDataSize = vertexDataBytes.size();
                    const size_t indexDataSize = indexData.size();
                    glm::mat4 worldTransform = node->GetGlobalTransform();

                    // Iterate through triangles for this submesh draw command
                    for (uint32_t i = 0; i < command.count; i += 3) {
                        uint32_t index0_ibo = command.firstIndex + i;
                        uint32_t index1_ibo = command.firstIndex + i + 1;
                        uint32_t index2_ibo = command.firstIndex + i + 2;

                        // Bounds check indices
                        if (index0_ibo >= indexDataSize || index1_ibo >= indexDataSize || index2_ibo >= indexDataSize) break;

                        uint32_t index0 = indexData[index0_ibo] + command.baseVertex;
                        uint32_t index1 = indexData[index1_ibo] + command.baseVertex;
                        uint32_t index2 = indexData[index2_ibo] + command.baseVertex;

                        size_t byteOffset0 = (size_t)index0 * vertexStride + positionOffset;
                        size_t byteOffset1 = (size_t)index1 * vertexStride + positionOffset;
                        size_t byteOffset2 = (size_t)index2 * vertexStride + positionOffset;

                        // Bounds check vertex data access
                        if (byteOffset0 + sizeof(glm::vec3) > vertexDataSize ||
                            byteOffset1 + sizeof(glm::vec3) > vertexDataSize ||
                            byteOffset2 + sizeof(glm::vec3) > vertexDataSize) break;

                        const std::byte* posPtr0 = vertexDataPtr + byteOffset0;
                        const std::byte* posPtr1 = vertexDataPtr + byteOffset1;
                        const std::byte* posPtr2 = vertexDataPtr + byteOffset2;

                        glm::vec3 v0_local = *reinterpret_cast<const glm::vec3*>(posPtr0);
                        glm::vec3 v1_local = *reinterpret_cast<const glm::vec3*>(posPtr1);
                        glm::vec3 v2_local = *reinterpret_cast<const glm::vec3*>(posPtr2);

                        glm::vec3 v0_world = glm::vec3(worldTransform * glm::vec4(v0_local, 1.0f));
                        glm::vec3 v1_world = glm::vec3(worldTransform * glm::vec4(v1_local, 1.0f));
                        glm::vec3 v2_world = glm::vec3(worldTransform * glm::vec4(v2_local, 1.0f));

                        // *** CREATE AND ADD TriWithEmisison struct ***
                        TriWithEmisison triangle;
                        triangle.v0 = v0_world;
                        triangle.v1 = v1_world;
                        triangle.v2 = v2_world;
                        triangle.emission = emissiveColor; // Assign fetched emission
                        // Padding fields (pad1, pad2, pad3, pad4) are automatically handled by struct alignment

                        trianglesForSSBO.push_back(triangle); // Add the complete struct
                        totalTrianglesExtracted++;
                    }
                } // End loop over node list items
            }; // End lambda

        auto materialManager = m_resourceLoader->GetMaterialManager();

        // Process the relevant lists from SceneManager
        std::cout << "Processing NonInstancedStatic..." << std::endl;
        processNodeList(m_sceneManager.GetNonInstancedStatic(), *materialManager);
        std::cout << "Processing RigidAnimated..." << std::endl;
        processNodeList(m_sceneManager.GetRigidAnimated(), *materialManager);
        std::cout << "Processing NonInstancedDynamic..." << std::endl;
        processNodeList(m_sceneManager.GetNonInstancedDynamic(), *materialManager);
        std::cout << "Processing Transparent..." << std::endl;
        processNodeList(m_sceneManager.GetTransparent(), *materialManager);
        //std::cout << "Processing InstancedStatic (Base Only)..." << std::endl;
        //processNodeList(m_sceneManager.GetInstancedStatic(), *materialManager);
        //std::cout << "Processing InstancedDynamic (Base Only)..." << std::endl;
        //processNodeList(m_sceneManager.GetInstancedDynamic(), *materialManager);

        std::cout << "PrepareTriangleSSBO: Extracted " << totalTrianglesExtracted << " triangles." << std::endl;

        GL_CHECK_ERROR();
        m_vgm->UpdateTriangleData(std::move(trianglesForSSBO));
        GL_CHECK_ERROR();
    }

    glm::mat4 DeferredRenderer::GetDirectionalLightSpaceMatrix(
        const glm::vec3& lightDir_normalized, 
        const glm::vec3& focusPoint,          
        float orthoSize,                      
        float shadowNearPlane,                
        float shadowFarPlane)                 
    {
        glm::mat4 lightProjection = glm::ortho(
            -orthoSize / 2.0f, orthoSize / 2.0f, // Left, Right
            -orthoSize / 2.0f, orthoSize / 2.0f, // Bottom, Top
            shadowNearPlane, shadowFarPlane       // Near, Far -> Note: Near/Far are distances *along the look direction*
        );

        glm::vec3 lightEyePos = focusPoint - (lightDir_normalized * (shadowFarPlane / 2.0f));

        glm::vec3 lightUp;
        if (glm::abs(glm::dot(lightDir_normalized, glm::vec3(0.0f, 1.0f, 0.0f))) > 0.999f)
        {
            glm::vec3 tempUp = glm::vec3(0.0f, 0.0f, -1.0f); // Or +1.0f, depends on convention
            glm::vec3 lightRight = glm::normalize(glm::cross(tempUp, lightDir_normalized));
            lightUp = glm::normalize(glm::cross(lightDir_normalized, lightRight));
        }
        else
        {
            glm::vec3 tempUp = glm::vec3(0.0f, 1.0f, 0.0f);
            glm::vec3 lightRight = glm::normalize(glm::cross(tempUp, lightDir_normalized));
            lightUp = glm::normalize(glm::cross(lightDir_normalized, lightRight));
        }

        glm::mat4 lightView = glm::lookAt(lightEyePos, focusPoint, lightUp);

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
            MaterialGPU matGPU{};
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
        m_lightOutputTarget->ResizeTextures(m_width, m_height);
        m_finalOutputTarget->ResizeTextures(m_width, m_height);
        m_skyTarget->ResizeTextures(m_width, m_height);

        // Recreate the G-buffer to match the new dimensions
        //m_assetLoader->GetRenderTargetManager()->Remove(m_gBufferTarget->GetName()); // Delete the old G-buffer
        //SetupGBuffer();

        std::cout << "DeferredRenderer resized to: " << m_width << "x" << m_height << std::endl;
    }
}