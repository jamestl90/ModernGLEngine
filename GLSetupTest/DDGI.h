#ifndef DDGI_H
#define DDGI_H

#include <glm/glm.hpp>

#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"

namespace JLEngine
{
    class UniformBuffer;

    struct alignas(16) DDGIProbe
    {
        glm::vec4 WorldPosition; // world centre of this probe
        glm::vec4 Irradiance;    // rgb
        float HitDistance;       // average hit distance 
        float Padding2[3];       // padding
    };

    struct alignas(16) DebugRay
    {
        glm::vec3 Origin;
        float _pad0;
        glm::vec3 Hit;
        float _pad1;
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

        const glm::vec3& GetGridOrigin() const { return m_gridOrigin; }
        const glm::ivec3& GetGridResolution() const { return m_gridResolution; }
        const glm::vec3& GetProbeSpacing() const { return m_probeSpacing; }
        const int GetRaysPerProbe() const { return m_raysPerProbe; }
        const float GetBlendFactor() const { return m_blendFactor; }
        const int GetDebugRayCount() const { return m_debugRayCount; }

        void GenerateProbes(const std::vector<std::pair<JLEngine::SubMesh, Node*>>& aabbs);
        void Update(float dt, UniformBuffer* shaderGlobaldata, const glm::mat4& inverseView, uint32_t posTex, uint32_t normalTex, uint32_t albedoTex);

        ShaderStorageBuffer<DDGIProbe>& GetProbeSSBO() { return m_probeSSBO; }
        ShaderStorageBuffer<DebugRay>& GetDebugRays() { return m_debugRaysSSBO; }

    private:

        ResourceLoader* m_resourceLoader;

        std::string m_assetPath;

        glm::vec3 m_gridOrigin = glm::vec3(0,4.5,0);
        glm::ivec3 m_gridResolution { 6, 3, 5 };
        glm::vec3 m_probeSpacing { 4.0f, 3.0f, 4.0f };
        
        int m_debugRayCount = 32;
        int m_raysPerProbe = 32;
        float m_blendFactor = 0.8f;

        ShaderStorageBuffer<DDGIProbe> m_probeSSBO;
        ShaderStorageBuffer<DebugRay> m_debugRaysSSBO;

        ShaderProgram* m_updateProbesCompute;
    };
}

#endif