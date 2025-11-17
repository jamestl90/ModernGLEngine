#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "GraphicsAPI.h"
#include "RenderTarget.h"
#include "RenderTargetPool.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "VertexArrayObject.h"
#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"
#include "GPUBuffer.h"
#include "UniformBuffer.h"
#include "TexturePool.h"
#include "SceneManager.h"
#include "Im3dManager.h"
#include "AtmosphereParameters.h"
#include "VoxelGrid.h"
#include "FlyCamera.h"

namespace JLEngine
{
    struct FrameRenderData
    {
        glm::mat4 viewMatrix;
        glm::mat4 projMatrix;
        glm::mat4 invViewMatrix;
        glm::mat4 invProjMatrix;
        glm::vec3 eyePos;
        glm::vec3 eyeDir;
        float fovRad;
        float aspect;
        float nearClip;
        float farClip;
    };

    enum DebugModes
    {
        GBuffer,
        DirectionalLightShadows,
        PbrSky,
        //HDRISkyTextures,
        None
    };

    enum class VAOType
    {
        STATIC,
        DYNAMIC,
        JL_TRANSPARENT // added prefix due to conflict with a #define in another file
    };

    class Material;
    class DirectionalLightShadowMap;
    class HDRISky;
    class DDGI;
    class PhysicallyBasedSky;
    class SkyProbe;
    class PostProcessing;

    class DeferredRenderer 
    {
    public:
        DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* resourceLoader,
            int width, int height, const std::string& assetFolder);
        ~DeferredRenderer();

        void EarlyInitialize();
        void LateInitialize();
        void Resize(int width, int height);
        
        void Render(FlyCamera& camera, double dt);

        bool GetDLShadowsEnabled() const { return m_enableDLShadows; }
        void SetDirectionalShadowDistance(bool value) { m_enableDLShadows = value; }

        void AddVAO(VAOType vaoType, VertexAttribKey key, std::shared_ptr<VertexArrayObject>& vao);
        void AddVAOs(VAOType vaoType, std::unordered_map<VertexAttribKey, std::shared_ptr<VertexArrayObject>>& vaos);

        void GenerateGPUBuffers();

        void ExtractSceneTriangles();

        std::shared_ptr<Node>& SceneRoot() { return m_sceneManager.GetRoot(); }
        SceneManager& GetSceneManager() { return m_sceneManager; }

        void SetDebugMode(DebugModes mode) { m_debugModes = mode; }
        void CycleDebugMode();

        HDRISky* GetHDRISky() { return m_hdriSky; }

        void SetDebugDrawer(IM3DManager* im3dMgr) { m_im3dManager = im3dMgr; }

        DDGI* GetDDGI();
        VoxelGridManager* GetVoxelGridManager();

    private:
        void DrawUI();        
        void DrawSky(FrameRenderData& frd);
        void DrawGeometry(const VAOResource& vaoResource, uint32_t stride);
        void CombinePass(FrameRenderData& frd);
        void LightPass(FrameRenderData& frd);
        void TransparencyPass(FrameRenderData& frd);
        void RenderBlended(FrameRenderData& frd);
        void RenderTransmissive(FrameRenderData& frd);
        void DebugPass(FrameRenderData& frd);
        void DebugGBuffer(int debugMode, float nearVal, float farVal);
        void DebugDirectionalLightShadows(float nearVal, float farVal);
        void DebugDDGI();
        void DebugDDGIRays();
        void DebugAABB();
        void RenderDebugTools(FrameRenderData& frd);
        void DebugHDRISky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DebugPbrSky(const glm::vec3& eyePos);
        void GBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetupGBuffer();
        void UpdateRigidAnimations();
        void UpdateSkinnedAnimations();
        void DirectionalShadowMapPass(FrameRenderData& frd);
        void RenderScreenSpaceTriangle();
        glm::mat4 GetDirectionalLightSpaceMatrix(
            const glm::vec3& lightDir_normalized,
            const glm::vec3& focusPoint,
            float orthoSizeFromCenter,
            float shadowNearPlane,
            float shadowFarPlane);
        void GenerateMaterialAndTextureBuffers(
            ResourceManager<JLEngine::Material>& materialManager,
            ResourceManager<JLEngine::Texture>& textureManager,
            std::vector<MaterialGPU>& materialBuffer,
            std::unordered_map<uint32_t, size_t>& materialIDMap);

        GraphicsAPI* m_graphics;
        ResourceLoader* m_resourceLoader;

        DebugModes m_debugModes;

        int m_frameCount = 0;
        int m_width, m_height;
        std::string m_assetFolder;

        float m_specularIndirectFactor = 1.0f;
        float m_diffuseIndirectFactor = 1.0f;
        float m_directFactor = 1.0f;
        float m_tonemappingExposure = 1.0f;
        glm::vec3 m_lastEyePos;

        IM3DManager* m_im3dManager;

        TexturePool m_texPool;
        RenderTargetPool m_rtPool;
        RenderTarget* m_gBufferTarget;
        RenderTarget* m_lightOutputTarget;
        RenderTarget* m_skyTarget;
        RenderTarget* m_finalOutputTarget;

        // raster shaders
        ShaderProgram* m_gBufferShader;
        ShaderProgram* m_lightingTestShader;         
        ShaderProgram* m_gBufferDebugShader;
        ShaderProgram* m_shadowDebugShader;
        ShaderProgram* m_passthroughShader;
        ShaderProgram* m_downsampleShader;
        ShaderProgram* m_blendShader;
        ShaderProgram* m_transmissionShader;
        ShaderProgram* m_skinningGBufferShader;
        //ShaderProgram* m_combineShader;
        ShaderProgram* m_debugSkyboxShader;

        // compute shaders
        ShaderProgram* m_simpleBlurCompute;
        ShaderProgram* m_jointTransformCompute;

        VertexArrayObject m_triangleVAO;

        UniformBuffer m_gShaderData;
        ShaderStorageBuffer<PerDrawData> m_ssboStaticPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboInstancedPerDraw;
        ShaderStorageBuffer<SkinnedMeshPerDrawData> m_ssboDynamicPerDraw;
        ShaderStorageBuffer<PerDrawData> m_ssboTransparentPerDraw;
        ShaderStorageBuffer<MaterialGPU> m_ssboMaterials;
        ShaderStorageBuffer<Skeleton::Joint> m_ssboJointMatrices;
        ShaderStorageBuffer<glm::mat4> m_ssboGlobalTransforms;
        ShaderStorageBuffer<LightGPU> m_lights;

        int m_staticRigidAnimationIndex = -1;
        std::unordered_map<VertexAttribKey, VAOResource> m_staticResources;
        std::pair<VertexAttribKey, VAOResource> m_skinnedMeshResources;
        std::unordered_map<VertexAttribKey, VAOResource> m_transparentResources;

        std::unordered_map<uint32_t, size_t> m_materialIDMap;
        std::vector<glm::mat4> m_jointMatrices;

        SceneManager m_sceneManager;

        // --- POST PROCESSING --- ///
        PostProcessing* m_postProcessing;

        // --- PBR SKY --- // 
        PhysicallyBasedSky* m_pbSky;
        SkyProbe* m_skyProbe;
        AtmosphereParams m_atmosphereParams = {};
        float m_uiSunAzimuthDegrees = 360.0f; 
        float m_uiSunElevationDegrees = 15.0f;
        GLuint m_brdfLUT; // brdf look up texture

        DDGI* m_ddgi;
        VoxelGridManager* m_vgm;
        DirectionalLightShadowMap* m_dlShadowMap;
        glm::vec3 m_dirLightColor = glm::vec3(1.0f);
        bool m_enableDLShadows;
        bool m_enableLights = true;

        bool m_showDDGI = false;
        bool m_showAABB = false;
        bool m_showDDGIRays = false;
        bool m_showVoxelGridBounds = false;

        HDRISky* m_hdriSky;
    };
}

#endif