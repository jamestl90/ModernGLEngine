#include "DDGI.h"

#include "Graphics.h"

void JLEngine::DDGI::Initialize()
{
    auto finalAssetPath = m_assetPath + "Core/Shaders/Compute";
    m_updateProbesCompute = m_resourceLoader->CreateComputeFromFile("DDGIProbeUpdate", "ddgi_probe_update.compute", finalAssetPath).get();

    InitProbes();
}

void JLEngine::DDGI::Update(float dt, uint32_t posTex, uint32_t normalTex, uint32_t albedoTex)
{
    Graphics::API()->BindShader(m_updateProbesCompute->GetProgramId());

    // bind textures
    GLuint textures[] =
    {
        posTex, normalTex, albedoTex
    };
    Graphics::API()->BindTextures(0, 3, textures);

    // bind probe data
    Graphics::BindGPUBuffer(m_probeSSBO.GetGPUBuffer(), 3);

    m_updateProbesCompute->SetUniform("u_GridResolution", m_gridResolution);
    m_updateProbesCompute->SetUniform("u_GridOrigin", m_gridOrigin);
    m_updateProbesCompute->SetUniform("u_ProbeSpacing", m_probeSpacing);
    m_updateProbesCompute->SetUniformf("u_RaysPerProbe", m_raysPerProbe);
    m_updateProbesCompute->SetUniformf("u_BlendFactor", m_blendFactor);

    Graphics::API()->DispatchCompute(m_gridResolution.x, m_gridResolution.y, m_gridResolution.z);
    Graphics::API()->SyncShaderStorageBarrier();
}

void JLEngine::DDGI::InitProbes()
{
    // get mutable probe data vector
    auto& probeData = m_probeSSBO.GetDataMutable();

    // clear old data if exists
    if (probeData.size() > 0)
        probeData.clear();

    // resize the data
    probeData.resize(m_gridResolution.x * m_gridResolution.y * m_gridResolution.z);

    for (int z = 0; z < m_gridResolution.z; ++z)
    {
        for (int y = 0; y < m_gridResolution.y; ++y)
        {
            for (int x = 0; x < m_gridResolution.x; ++x)
            {
                int index = x + y * m_gridResolution.x + z * m_gridResolution.x * m_gridResolution.y;
                DDGIProbe& probe = probeData[index];

                probe.WorldPosition = glm::vec4(m_gridOrigin, 1.0f) + 
                    glm::vec4(m_probeSpacing, 1.0f) * glm::vec4(x, y, z, 1.0f);
                probe.Irradiance = glm::vec4(0.0f);
                probe.HitDistance = 1.0f; // default to avoid initial blackness
            }
        }
    }

    Graphics::CreateGPUBuffer(m_probeSSBO.GetGPUBuffer(), m_probeSSBO.GetDataImmutable());
}
