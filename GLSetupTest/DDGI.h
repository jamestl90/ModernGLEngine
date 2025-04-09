#ifndef DDGI_H
#define DDGI_H

#include <glm/glm.hpp>

#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"
#include "VoxelGrid.h"

namespace JLEngine
{
    class UniformBuffer;

    struct alignas(16) DDGIProbe 
    {
        glm::vec4 WorldPosition; 
        glm::vec4 SHCoeffs[9];   
        float Depth;             
        float DepthMoment2;      
        glm::vec2 _padding;
    };

    struct alignas(16) DebugRay
    {
        glm::vec3 Origin;
        float _pad0;
        glm::vec3 Hit;
        float _pad1;
        glm::vec3 Color;
        float _pad2;
    };

    class DDGI
    {
    public:
        DDGI(ResourceLoader* resourceLoader, const std::string& assetPath);
        ~DDGI();

        void SetGridOrigin(const glm::vec3& gridOrigin) { m_gridOrigin = gridOrigin; }
        void SetGridResolution(const glm::ivec3& gridRes) { m_gridResolution = gridRes; }
        void SetProbeSpacing(const glm::vec3& probeSpacing) { m_probeSpacing = probeSpacing; }
        void SetRaysPerProbe(const int raysPerProbe) { m_raysPerProbe = raysPerProbe; }
        void SetBlendFactor(const float blendFactor) { m_blendFactor = blendFactor; }
        void SetDebugRayCount(const int count) { m_debugRayCount = count; }
        void SetDebugProbeIndex(const int idx) { m_debugProbeIndex = idx; }

        const glm::vec3& GetGridOrigin() const { return m_gridOrigin; }
        const glm::ivec3& GetGridResolution() const { return m_gridResolution; }
        const glm::vec3& GetProbeSpacing() const { return m_probeSpacing; }
        const int GetRaysPerProbe() const { return m_raysPerProbe; }
        const float GetBlendFactor() const { return m_blendFactor; }
        const int GetDebugRayCount() const { return m_debugRayCount; }
        const int GetDebugProbeIndex() const { return m_debugProbeIndex; }
        int& GetDebugProbeIndexMutable() { return m_debugProbeIndex; }
        int& GetRaysPerProbeMutable() { return m_raysPerProbe; }
        float& GetBlendFactorMutable() { return m_blendFactor; }
        float& GetSkyLightColBlendFacMutable() { return m_skyLightColBlendFac; }
        float& GetRayLengthMutable() { return m_maxDistance; }

        void GenerateProbes(const std::vector<std::pair<JLEngine::SubMesh, Node*>>& aabbs);
        void Update(float dt, 
            UniformBuffer* shaderGlobaldata, 
            const glm::mat4& inverseView, 
            glm::vec3& dirLightCol,
            uint32_t skyTex, 
            uint32_t voxtex);

        ShaderStorageBuffer<DDGIProbe>& GetProbeSSBO() { return m_probeSSBO; }
        ShaderStorageBuffer<DebugRay>& GetDebugRays() { return m_debugRaysSSBO; }

        void SetVoxelGridInfo(VoxelGrid* voxelGrid) { m_voxelGrid = voxelGrid; }

    private:

        ResourceLoader* m_resourceLoader;

        std::string m_assetPath;

        glm::vec3 m_gridOrigin = glm::vec3(0,4.5,0);
        glm::ivec3 m_gridResolution { 10, 7, 10 };
        glm::vec3 m_probeSpacing { 2.5f, 1.25f, 2.5f };
        
        int m_debugRayCount = 128;
        int m_raysPerProbe = 128;
        int m_debugProbeIndex = 0;
        float m_blendFactor = 0.8f;
        float m_maxDistance = 4.5f;    // sort of depends on probe spacing
        float m_skyLightColBlendFac = 0.5f;

        ShaderStorageBuffer<DDGIProbe> m_probeSSBO;
        ShaderStorageBuffer<DebugRay> m_debugRaysSSBO;

        ShaderProgram* m_updateProbesCompute;

        VoxelGrid* m_voxelGrid;
    };
}

#endif