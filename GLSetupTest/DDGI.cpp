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

	float maxDistance = 70.0f; 

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
					probe.WorldPosition = glm::vec4(worldPos, 1.0f); // or maybe w=0 as a flag?
					for (int i = 0; i < 9; ++i) probe.SHCoeffs[i] = glm::vec4(0.0f);
					probe.Depth = 0.0f; // indicate no valid distance data
					probe.DepthMoment2 = 0.0f;
					probe._padding.x = 1.0f;
					continue;
				}

				// Initialize normally
				probe.WorldPosition = glm::vec4(worldPos, 1.0f);
				for (int i = 0; i < 9; ++i) probe.SHCoeffs[i] = glm::vec4(0.0f); // Start black
				probe.Depth = maxDistance; // initialize with max distance
				probe.DepthMoment2 = maxDistance * maxDistance;
				probe._padding = glm::vec2(0.0f);
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

void JLEngine::DDGI::Update(float dt, 
	UniformBuffer* shaderGlobaldata, 
	glm::vec3& dirLightCol,
	uint32_t skyTex, 
	VoxelGrid& grid)
{
    Graphics::API()->BindShader(m_updateProbesCompute->GetProgramId());

    // bind textures
    GLuint textures[] = 
	{ 
		skyTex, 
		grid.occupancyTexId, 
		grid.emissionTexIds[0], 
		grid.emissionTexIds[1], 
		grid.emissionTexIds[2] 
	};
    Graphics::API()->BindTextures(0, 5, textures);

    // bind probe data
    Graphics::BindGPUBuffer(m_probeSSBO.GetGPUBuffer(), 7);
	Graphics::BindGPUBuffer(m_debugRaysSSBO.GetGPUBuffer(), 4);
	Graphics::BindGPUBuffer(shaderGlobaldata->GetGPUBuffer(), 5);

    m_updateProbesCompute->SetUniform("u_ProbeGridResolution", m_gridResolution);
	m_updateProbesCompute->SetUniform("u_ProbeGridCenter", m_gridOrigin);
	m_updateProbesCompute->SetUniform("u_ProbeSpacing", m_probeSpacing);
    m_updateProbesCompute->SetUniformi("u_RaysPerProbe", m_raysPerProbe);
	m_updateProbesCompute->SetUniformf("u_MaxDistance", m_maxDistance);
    m_updateProbesCompute->SetUniformf("u_BlendFactor", m_blendFactor);
	m_updateProbesCompute->SetUniformi("u_DebugRayCount", m_debugRayCount);
	m_updateProbesCompute->SetUniformi("u_DebugProbeIndex", m_debugProbeIndex);
	m_updateProbesCompute->SetUniform("u_DirLightCol", dirLightCol);
	m_updateProbesCompute->SetUniformf("u_SkyLightColBlendFac", m_skyLightColBlendFac);

	m_updateProbesCompute->SetUniform("u_VoxelGridCenter", grid.worldOrigin);
	m_updateProbesCompute->SetUniform("u_VoxelGridWorldSize", grid.worldSize);
	m_updateProbesCompute->SetUniform("u_VoxelGridResolution", grid.resolution);

	GLuint totalProbesX = this->m_gridResolution.x;
	GLuint totalProbesY = this->m_gridResolution.y;
	GLuint totalProbesZ = this->m_gridResolution.z;

	const GLuint localSizeX = 1;
	const GLuint localSizeY = 1;
	const GLuint localSizeZ = 1;

	// Calculate number of workgroups needed for each dimension
	GLuint numGroupsX = (totalProbesX + localSizeX - 1) / localSizeX;
	GLuint numGroupsY = (totalProbesY + localSizeY - 1) / localSizeY;
	GLuint numGroupsZ = (totalProbesZ + localSizeZ - 1) / localSizeZ;

    Graphics::API()->DispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
    Graphics::API()->SyncShaderStorageBarrier();

	Graphics::API()->BindShader(0);
	Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, 0);
	Graphics::API()->BindBufferBase(GL_UNIFORM_BUFFER, 5, 0);
	Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, 0);
}
