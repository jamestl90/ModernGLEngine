#ifndef JL_VOXEL_GRID_H
#define JL_VOXEL_GRID_H

#include <glm/glm.hpp>

#include "Graphics.h"
#include "ShaderProgram.h"
#include "ResourceLoader.h"
#include "ShaderStorageBuffer.h"

namespace JLEngine
{
	struct alignas(16) TriWithEmisison
	{
		glm::vec3 v0;
		float pad1;
		glm::vec3 v1;
		float pad2;
		glm::vec3 v2;
		float pad3;
		glm::vec3 emission;
		float pad4;
	};

	struct alignas(16) DebugVoxelInfo
	{
		glm::vec4 centroid;
		glm::vec4 relativePos;
		glm::ivec4 centerVoxel;
	};

	struct VoxelGrid
	{
		~VoxelGrid() { if (Graphics::Alive()) DisposeTextures(); }

		GLuint occupancyTexId = 0;
		GLuint emissionTexIds[3] = { 0,0,0 };
		glm::ivec3 resolution = glm::ivec3(256, 256, 256);
		glm::vec3 worldOrigin = glm::vec3(0, 4, 0);
		glm::vec3 worldSize = glm::vec3(30, 8, 30);
		glm::vec3 voxelSize;
		glm::vec3 invVoxelSize;

		void DisposeTextures()
		{
			if (occupancyTexId > 0) Graphics::API()->DeleteTexture(1, &occupancyTexId);
			if (emissionTexIds[0] > 0) Graphics::API()->DeleteTexture(1, &emissionTexIds[0]);
			if (emissionTexIds[0] > 0) Graphics::API()->DeleteTexture(1, &emissionTexIds[1]);
			if (emissionTexIds[0] > 0) Graphics::API()->DeleteTexture(1, &emissionTexIds[2]);
		}

		void CalcSizes()
		{
			voxelSize = worldSize / glm::vec3(resolution);
			invVoxelSize = 1.0f / voxelSize;
		}
	};

	class VoxelGridManager
	{
	public:
		VoxelGridManager(ResourceLoader* loader, std::string& assetPath)
			: m_resLoader(loader)
		{
			auto finalAssetPath = assetPath + "Core/Shaders/Compute/";
			m_voxelizeCompute = m_resLoader->CreateComputeFromFile("VoxelizeCompute",
															  "voxelize.compute",
															  finalAssetPath).get();
			m_clearVoxelsCompute = m_resLoader->CreateComputeFromFile("ClearVoxels",
															   "clear_voxels.compute",
															   finalAssetPath).get();
		}

		~VoxelGridManager()
		{
			Graphics::DisposeGPUBuffer(&m_trianglesSSBO.GetGPUBuffer());
			// all shaders should be cleaned up on program exit by default			
		}

		void Initialise()
		{
			m_voxelGrid.CalcSizes();

			glCreateTextures(GL_TEXTURE_3D, 1, &m_voxelGrid.occupancyTexId);
			glTextureStorage3D(m_voxelGrid.occupancyTexId, 1,
				GL_R32UI,
				m_voxelGrid.resolution.x,
				m_voxelGrid.resolution.y,
				m_voxelGrid.resolution.z);
			glTextureParameteri(m_voxelGrid.occupancyTexId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_voxelGrid.occupancyTexId, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTextureParameteri(m_voxelGrid.occupancyTexId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(m_voxelGrid.occupancyTexId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTextureParameteri(m_voxelGrid.occupancyTexId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
			GLuint borderColourUint[] = { 0u, 0u, 0u, 0u };
			glTextureParameterIuiv(m_voxelGrid.occupancyTexId, GL_TEXTURE_BORDER_COLOR, borderColourUint);

			Graphics::API()->DebugLabelObject(GL_TEXTURE, m_voxelGrid.occupancyTexId, "VoxelGridOccupancy");

			for (int i = 0; i < 3; i++)
			{
				Graphics::API()->CreateTextures(GL_TEXTURE_3D, 1, &m_voxelGrid.emissionTexIds[i]);
				Graphics::API()->TextureStorage3D(m_voxelGrid.emissionTexIds[i], 1, GL_R32UI, m_voxelGrid.resolution.x, m_voxelGrid.resolution.y, m_voxelGrid.resolution.z);
				Graphics::API()->TextureParameter(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				Graphics::API()->TextureParameter(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_MAG_FILTER, GL_NEAREST);
				Graphics::API()->TextureParameter(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				Graphics::API()->TextureParameter(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				Graphics::API()->TextureParameter(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
				GLuint borderColourUint[] = { 0u, 0u, 0u, 0u };
				glTextureParameterIuiv(m_voxelGrid.emissionTexIds[i], GL_TEXTURE_BORDER_COLOR, borderColourUint);
				Graphics::API()->DebugLabelObject(GL_TEXTURE, m_voxelGrid.emissionTexIds[i], "EmissionTex" + i);
			}

			m_trianglesSSBO.GetGPUBuffer().SetImmutable(false);
			Graphics::CreateGPUBuffer(m_trianglesSSBO.GetGPUBuffer());

			m_debugVoxelSSBO.GetGPUBuffer().SetSizeInBytes(m_totalSceneTriangles * sizeof(DebugVoxelInfo));
			m_debugVoxelSSBO.GetGPUBuffer().SetUsageFlags(GL_MAP_READ_BIT); 

			Graphics::CreateGPUBuffer(m_debugVoxelSSBO.GetGPUBuffer());
		}

		void Render()
		{
			if (m_totalSceneTriangles > 0 && m_clearVoxelsCompute != nullptr)
			{
				if (m_clearVoxelsCompute)
				{
					Graphics::API()->BindShader(m_clearVoxelsCompute->GetProgramId());

					Graphics::API()->BindImageTexture(0, m_voxelGrid.occupancyTexId, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(1, m_voxelGrid.emissionTexIds[0], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(2, m_voxelGrid.emissionTexIds[1], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(3, m_voxelGrid.emissionTexIds[2], 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					m_clearVoxelsCompute->SetUniform("u_GridResolution", m_voxelGrid.resolution);

					GLuint numGroupsX = (m_voxelGrid.resolution.x + 7) / 8; 
					GLuint numGroupsY = (m_voxelGrid.resolution.y + 7) / 8;
					GLuint numGroupsZ = (m_voxelGrid.resolution.z + 7) / 8;
					Graphics::API()->DispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
					glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

					// unbinds
					Graphics::API()->BindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(1, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(2, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					Graphics::API()->BindImageTexture(3, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
				}

				Graphics::API()->BindShader(m_voxelizeCompute->GetProgramId());

				Graphics::API()->BindImageTexture(0, m_voxelGrid.occupancyTexId, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
				Graphics::API()->BindImageTexture(1, m_voxelGrid.emissionTexIds[0], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
				Graphics::API()->BindImageTexture(2, m_voxelGrid.emissionTexIds[1], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
				Graphics::API()->BindImageTexture(3, m_voxelGrid.emissionTexIds[2], 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

				Graphics::BindGPUBuffer(m_trianglesSSBO.GetGPUBuffer(), 1);
				Graphics::BindGPUBuffer(m_debugVoxelSSBO.GetGPUBuffer(), 2);

				m_voxelizeCompute->SetUniform("u_GridCenter", m_voxelGrid.worldOrigin);
				m_voxelizeCompute->SetUniform("u_GridResolution", m_voxelGrid.resolution);
				m_voxelizeCompute->SetUniform("u_GridWorldSize", m_voxelGrid.worldSize);

				const uint32_t voxelizeLocalSize = 64;
				auto numTriangleGroups = (m_totalSceneTriangles + voxelizeLocalSize - 1) / voxelizeLocalSize;
				Graphics::API()->DispatchCompute(static_cast<uint32_t>(numTriangleGroups), 1, 1);

				glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT);

				//std::cout << "VoxelGridManager::Render - Barrier after Compute Dispatch." << std::endl;
				// --- READ BACK DEBUG DATA ---
				//std::cout << "Reading back debug voxel info..." << std::endl;
				//size_t bufferSize = m_debugVoxelSSBO.GetGPUBuffer().GetSizeInBytes();
				//if (bufferSize > 0) 
				//{
				//	// Use GL_MAP_READ_BIT when mapping
				//	void* ptr = Graphics::API()->MapNamedBufferRange(m_debugVoxelSSBO.GetGPUBuffer().GetGPUID(), GL_MAP_READ_BIT, 0, bufferSize);
				//	if (ptr) 
				//	{
				//		DebugVoxelInfo* debugData = static_cast<DebugVoxelInfo*>(ptr);
				//		size_t numItems = bufferSize / sizeof(DebugVoxelInfo);
				//		size_t printCount = std::min((size_t)10, numItems); // Print first 10 items
				//
				//		std::cout << "--- Debug Voxel Info (First " << printCount << " / " << numItems << ") ---" << std::endl;
				//		for (size_t i = 0; i < printCount; ++i) {
				//			std::cout << "[" << i << "] Centroid: " << glm::to_string(glm::vec3(debugData[i].centroid))
				//				<< ", RelPos: " << glm::to_string(glm::vec3(debugData[i].relativePos))
				//				<< ", Voxel: " << glm::to_string(glm::ivec3(
				// debugData[i].centerVoxel))
				//				<< ", InBounds: " << debugData[i].cente
				// rVoxel.w << std::endl;
				//		}
				//		std::cout << "-----------------------------------------" << std::endl;
				//
				//		Graphics::API()->UnmapNamedBuffer(m_debugVoxelSSBO.GetGPUBuffer().GetGPUID());
				//	}
				//	else 
				//	{
				//		std::cerr << "ERROR: Failed to map debug voxel SSBO!" << std::endl;
				//		 // Check why mapping failed
				//	}
				//}
				//else {
				//	std::cout << "Debug voxel SSBO size is zero, skipping readback." << std::endl;
				//}
				// 
				// unbinds
				Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
				Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0); // Unbind debug SSBO
				Graphics::API()->BindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
				Graphics::API()->BindImageTexture(1, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
				Graphics::API()->BindImageTexture(2, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
				Graphics::API()->BindImageTexture(3, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
			}
		}

		void UpdateTriangleData(std::vector<TriWithEmisison>&& tris)
		{
			m_totalSceneTriangles = tris.size();
			std::cout << "VoxelGridManager: Received " << m_totalSceneTriangles << " triangles." << std::endl; 

			if (!tris.empty())
			{
				size_t dataSizeInBytes = tris.size() * sizeof(TriWithEmisison);
				std::cout << "VoxelGridManager: Preparing to upload " << dataSizeInBytes << " bytes." << std::endl;

				m_trianglesSSBO.GetDataMutable() = std::move(tris);
				std::cout << "VoxelGridManager: SSBO internal data size: " << 
					m_trianglesSSBO.GetDataImmutable().size() * sizeof(TriWithEmisison) << " bytes." << std::endl;

				Graphics::UploadToGPUBuffer(m_trianglesSSBO.GetGPUBuffer(), m_trianglesSSBO.GetDataImmutable());
				std::cout << "VoxelGridManager: UploadToGPUBuffer called." << std::endl; 
			}
			else 
			{
				std::cout << "VoxelGridManager: Received empty triangle vector." << std::endl;
			}

			size_t requiredDebugBytes = m_totalSceneTriangles * sizeof(DebugVoxelInfo);
			if (m_debugVoxelSSBO.GetGPUBuffer().GetSizeInBytes() < requiredDebugBytes) 
			{
				m_debugVoxelSSBO.GetGPUBuffer().SetSizeInBytes(requiredDebugBytes);

				Graphics::DisposeGPUBuffer(&m_debugVoxelSSBO.GetGPUBuffer());
				Graphics::CreateGPUBuffer(m_debugVoxelSSBO.GetGPUBuffer()); 
				Graphics::API()->DebugLabelObject(GL_BUFFER, m_debugVoxelSSBO.GetGPUBuffer().GetGPUID(), "VoxelDebugSSBO"); 
			}
		}

		VoxelGrid& GetVoxelGrid() { return m_voxelGrid; }

	private:
		ShaderProgram* m_voxelizeCompute = nullptr;
		ShaderProgram* m_clearVoxelsCompute = nullptr;

		VoxelGrid m_voxelGrid;
		
		ShaderStorageBuffer<TriWithEmisison> m_trianglesSSBO;
		ShaderStorageBuffer<DebugVoxelInfo> m_debugVoxelSSBO;
		size_t m_totalSceneTriangles = 0;

		ResourceLoader* m_resLoader = nullptr;
	};
}


#endif