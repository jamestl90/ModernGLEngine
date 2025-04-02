#include "DDGI.h"

#include "Graphics.h"
#include "UniformBuffer.h"

JLEngine::DDGI::DDGI(ResourceLoader* resourceLoader, const std::string& assetPath)
	: m_resourceLoader(resourceLoader), m_updateProbesCompute(nullptr), m_assetPath(assetPath)
{
	auto finalAssetPath = m_assetPath + "Core/Shaders/Compute/";
	m_updateProbesCompute = m_resourceLoader->CreateComputeFromFile("DDGIProbeUpdate", "ddgi_probe_update.compute", finalAssetPath).get();
}

JLEngine::DDGI::~DDGI()
{
    m_resourceLoader->DeleteShader("DDGIProbeUpdate");
}

void JLEngine::DDGI::GenerateProbes(const std::vector<std::pair<JLEngine::SubMesh, Node*>>& aabbs)
{
	auto& probeData = m_probeSSBO.GetDataMutable();

	probeData.clear();
	probeData.resize(m_gridResolution.x * m_gridResolution.y * m_gridResolution.z);

	glm::vec3 gridSize = glm::vec3(m_gridResolution - 1) * m_probeSpacing;
	glm::vec3 halfGrid = gridSize * 0.5f;

	for (int z = 0; z < m_gridResolution.z; ++z)
	{
		for (int y = 0; y < m_gridResolution.y; ++y)
		{
			for (int x = 0; x < m_gridResolution.x; ++x)
			{
				int index = x + y * m_gridResolution.x + z * m_gridResolution.x * m_gridResolution.y;
				DDGIProbe& probe = probeData[index];

				glm::vec3 localOffset = glm::vec3(x, y, z) * m_probeSpacing;
				glm::vec3 worldPos = m_gridOrigin + localOffset - halfGrid;

				bool intersects = false;
				for (const auto& [submesh, node] : aabbs)
				{
					const glm::mat4 worldTransform = node->GetGlobalTransform();

					if (submesh.aabb.ContainsPoint(worldPos, worldTransform))
					{
						intersects = true;
						break;
					}
				}

				if (intersects)
				{
					probe.WorldPosition = glm::vec4(worldPos, 1.0f);
					probe.Irradiance = glm::vec4(0.0f);
					probe.HitDistance = -1.0f; // disabled when hitDistance = -1
					continue;
				}

				// Initialize normally
				probe.WorldPosition = glm::vec4(worldPos, 1.0f);
				probe.Irradiance = glm::vec4(0.0f);
				probe.HitDistance = 1.0f;
			}
		}
	}

	Graphics::CreateGPUBuffer(m_probeSSBO.GetGPUBuffer(), m_probeSSBO.GetDataImmutable());

	for (int i = 0; i < m_debugRayCount; i++)
	{
		DebugRay ray{ glm::vec3(0,1,0), 1.0f, glm::vec3(i, 2, 0), 1.0f };
		m_debugRaysSSBO.GetDataMutable().push_back(ray);
	}
	m_debugRaysSSBO.GetGPUBuffer().SetUsageFlags(GL_MAP_WRITE_BIT | GL_MAP_READ_BIT);
	Graphics::CreateGPUBuffer(m_debugRaysSSBO.GetGPUBuffer(), m_debugRaysSSBO.GetDataImmutable());
}

void JLEngine::DDGI::Update(float dt, UniformBuffer* shaderGlobaldata, const glm::mat4& inverseView, uint32_t posTex, uint32_t normalTex, uint32_t albedoTex, uint32_t depthTex)
{
    Graphics::API()->BindShader(m_updateProbesCompute->GetProgramId());

    // bind textures
    GLuint textures[] = { posTex, normalTex, albedoTex, depthTex };
    Graphics::API()->BindTextures(0, 4, textures);

    // bind probe data
    Graphics::BindGPUBuffer(m_probeSSBO.GetGPUBuffer(), 7);
	Graphics::BindGPUBuffer(m_debugRaysSSBO.GetGPUBuffer(), 4);
	Graphics::BindGPUBuffer(shaderGlobaldata->GetGPUBuffer(), 5);

    m_updateProbesCompute->SetUniform("u_GridResolution", m_gridResolution);
    m_updateProbesCompute->SetUniform("u_GridOrigin", m_gridOrigin);
    m_updateProbesCompute->SetUniform("u_ProbeSpacing", m_probeSpacing);
    m_updateProbesCompute->SetUniformi("u_RaysPerProbe", m_raysPerProbe);
    m_updateProbesCompute->SetUniformf("u_BlendFactor", m_blendFactor);
	m_updateProbesCompute->SetUniformi("u_DebugRayCount", m_debugRayCount);
	m_updateProbesCompute->SetUniformi("u_DebugProbeIndex", m_debugProbeIndex);
	m_updateProbesCompute->SetUniform("u_InverseView", inverseView);
	m_updateProbesCompute->SetUniformf("u_HitThreshold", m_hitThreshold);



    Graphics::API()->DispatchCompute(m_gridResolution.x, m_gridResolution.y, m_gridResolution.z);
    Graphics::API()->SyncShaderStorageBarrier();
}
