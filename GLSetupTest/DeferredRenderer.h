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

namespace JLEngine
{
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

    class DeferredRenderer 
    {
    public:
        DeferredRenderer(GraphicsAPI* graphics, ResourceLoader* resourceLoader,
            int width, int height, const std::string& assetFolder);
        ~DeferredRenderer();

        void EarlyInitialize();
        void LateInitialize();
        void Resize(int width, int height);
        
        void Render(const glm::vec3& eyePos, const glm::vec3& camDir, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, double dt);

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
        void DrawSky(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DrawGeometry(const VAOResource& vaoResource, uint32_t stride);
        void CombinePass(const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void LightPass(const glm::vec3& eyePos, const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::mat4& lightSpaceMatrix);
        void TransparencyPass(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void RenderBlended(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void RenderTransmissive(const glm::vec3& eyePos, const glm::mat4& viewMat, const glm::mat4& projMatrix);
        void DebugPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DebugGBuffer(int debugMode);
        void DebugDirectionalLightShadows();
        void DebugDDGI();
        void DebugDDGIRays();
        void DebugAABB();
        void RenderDebugTools(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DebugHDRISky(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void DebugPbrSky();
        void GBufferPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
        void SetupGBuffer();
        void UpdateRigidAnimations();
        void UpdateSkinnedAnimations();
        glm::mat4 DirectionalShadowMapPass(const glm::mat4& viewMatrix, const glm::mat4& projMatrix, const glm::vec3& eyePos);
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

        float m_specularIndirectFactor = 2.0f;
        float m_diffuseIndirectFactor = 2.0f;
        float m_directFactor = 2.0f;
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
        ShaderProgram* m_combineShader;

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

        int m_staticRigidAnimationIndex = -1;
        std::unordered_map<VertexAttribKey, VAOResource> m_staticResources;
        std::pair<VertexAttribKey, VAOResource> m_skinnedMeshResources;
        std::unordered_map<VertexAttribKey, VAOResource> m_transparentResources;

        std::unordered_map<uint32_t, size_t> m_materialIDMap;
        std::vector<glm::mat4> m_jointMatrices;

        SceneManager m_sceneManager;

        // --- PBR SKY --- // 
        PhysicallyBasedSky* m_pbSky;
        AtmosphereParams m_atmosphereParams = {};
        float m_uiSunAzimuthDegrees = 180.0f; // Or initialize based on initial sunDir
        float m_uiSunElevationDegrees = 15.0f;

        DDGI* m_ddgi;
        VoxelGridManager* m_vgm;
        DirectionalLightShadowMap* m_dlShadowMap;
        glm::vec3 m_dirLightColor = glm::vec3(1.0f);
        bool m_enableDLShadows;

        bool m_showDDGI = false;
        bool m_showAABB = false;
        bool m_showDDGIRays = false;
        bool m_showVoxelGridBounds = false;

        HDRISky* m_hdriSky;
    };
}

#endif