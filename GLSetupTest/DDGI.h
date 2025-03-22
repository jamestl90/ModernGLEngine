#ifndef DDGI_H
#define DDGI_H

#include <glm/glm.hpp>

#include "ShaderStorageBuffer.h"
#include "ResourceLoader.h"

namespace JLEngine
{
    struct alignas(16) DDGIProbe
    {
        glm::vec4 WorldPosition; // world centre of this probe
        glm::vec4 Irradiance;    // rgb
        float HitDistance;       // average hit distance 
        float Padding2[3];       // padding
    };

    class DDGI
    {
    public:
        DDGI(ResourceLoader* resourceLoader, const std::string& assetPath)
            : m_resourceLoader(resourceLoader), m_updateProbesCompute(nullptr), m_assetPath(assetPath) {};
        ~DDGI() = default;

        void SetGridOrigin(const glm::vec3& gridOrigin) { m_gridOrigin = gridOrigin; }
        void SetGridResolution(const glm::ivec3& gridRes) { m_gridResolution = gridRes; }
        void SetProbeSpacing(const glm::vec3& probeSpacing) { m_probeSpacing = probeSpacing; }
        void SetRaysPerProbe(const float raysPerProbe) { m_raysPerProbe = raysPerProbe; }
        void SetBlendFactor(const float blendFactor) { m_blendFactor = blendFactor; }

        const glm::vec3& GetGridOrigin() const { return m_gridOrigin; }
        const glm::ivec3& GetGridResolution() const { return m_gridResolution; }
        const glm::vec3& GetProbeSpacing() const { return m_probeSpacing; }
        const float GetRaysPerProbe() const { return m_raysPerProbe; }
        const float GetBlendFactor() const { return m_blendFactor; }

        void Initialize();     
        void Update(float dt, uint32_t posTex, uint32_t normalTex, uint32_t albedoTex);

    private:
        void InitProbes();

        ResourceLoader* m_resourceLoader;

        std::string m_assetPath;

        glm::vec3 m_gridOrigin{ 0.0f };
        glm::ivec3 m_gridResolution{ 8, 4, 8 };
        glm::vec3 m_probeSpacing{ 4.0f };
        
        float m_raysPerProbe = 32;
        float m_blendFactor = 0.8f;

        ShaderStorageBuffer<DDGIProbe> m_probeSSBO;

        ShaderProgram* m_updateProbesCompute;
    };
}

#endif