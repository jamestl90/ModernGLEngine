#ifndef JL_VOXEL_GRID_H
#define JL_VOXEL_GRID_H

#include <glm/glm.hpp>

#include "Graphics.h"
#include "ShaderProgram.h"
#include "ResourceLoader.h"
#include "ShaderStorageBuffer.h"

namespace JLEngine
{
	struct alignas(16) DebugVoxelInfo
	{
		glm::vec4 centroid;
		glm::vec4 relativePos;
		glm::ivec4 centerVoxel;
	};

	struct VoxelGrid
	{
		GLuint textureId = 0;
		glm::ivec3 resolution = glm::ivec3(256, 256, 256);
		glm::vec3 worldOrigin = glm::vec3(0, 4, 0);
		glm::vec3 worldSize = glm::vec3(30, 8, 30);
		glm::vec3 voxelSize;
		glm::vec3 invVoxelSize;

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

			glCreateTextures(GL_TEXTURE_3D, 1, &m_voxelGrid.textureId);
			glTextureStorage3D(m_voxelGrid.textureId, 1,
				GL_R32UI,
				m_voxelGrid.resolution.x,
				m_voxelGrid.resolution.y,
				m_voxelGrid.resolution.z);
			glTextureParameteri(m_voxelGrid.textureId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_voxelGrid.textureId, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTextureParameteri(m_voxelGrid.textureId, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTextureParameteri(m_voxelGrid.textureId, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
			glTextureParameteri(m_voxelGrid.textureId, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);
			float borderColour[] = { 0.0f, 0.0f, 0.0f, 0.0f };
			glTextureParameterfv(m_voxelGrid.textureId, GL_TEXTURE_BORDER_COLOR, borderColour);

			GL_CHECK_ERROR();

			Graphics::API()->DebugLabelObject(GL_TEXTURE, m_voxelGrid.textureId, "VoxelGridOccupancy");

			m_trianglesSSBO.GetGPUBuffer().SetImmutable(false);
			Graphics::CreateGPUBuffer(m_trianglesSSBO.GetGPUBuffer());
			Graphics::API()->DebugLabelObject(GL_BUFFER, m_trianglesSSBO.GetGPUBuffer().GetGPUID(), "SceneTrianglesSSBO");

			m_debugVoxelSSBO.GetGPUBuffer().SetSizeInBytes(m_totalSceneTriangles * sizeof(DebugVoxelInfo));
			m_debugVoxelSSBO.GetGPUBuffer().SetUsageFlags(GL_MAP_READ_BIT); 
			Graphics::CreateGPUBuffer(m_debugVoxelSSBO.GetGPUBuffer());
			Graphics::API()->DebugLabelObject(GL_BUFFER, m_debugVoxelSSBO.GetGPUBuffer().GetGPUID(), "VoxelDebugSSBO");

			GL_CHECK_ERROR(); // Check error after clear
		}

		void Render()
		{
			if (m_totalSceneTriangles > 0 && m_clearVoxelsCompute != nullptr)
			{
				if (m_clearVoxelsCompute)
				{
					Graphics::API()->BindShader(m_clearVoxelsCompute->GetProgramId());

					Graphics::API()->BindImageTexture(0, m_voxelGrid.textureId, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
					m_clearVoxelsCompute->SetUniform("u_GridResolution", m_voxelGrid.resolution);

					GLuint numGroupsX = (m_voxelGrid.resolution.x + 7) / 8; 
					GLuint numGroupsY = (m_voxelGrid.resolution.y + 7) / 8;
					GLuint numGroupsZ = (m_voxelGrid.resolution.z + 7) / 8;
					Graphics::API()->DispatchCompute(numGroupsX, numGroupsY, numGroupsZ);
					glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

					Graphics::API()->BindImageTexture(0, 0, 0, GL_TRUE, 0, GL_WRITE_ONLY, GL_R32UI);
				}

				Graphics::API()->BindShader(m_voxelizeCompute->GetProgramId());

				Graphics::API()->BindImageTexture(0, m_voxelGrid.textureId, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
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
				//				<< ", Voxel: " << glm::to_string(glm::ivec3(debugData[i].centerVoxel))
				//				<< ", InBounds: " << debugData[i].centerVoxel.w << std::endl;
				//		}
				//		std::cout << "-----------------------------------------" << std::endl;
				//
				//		Graphics::API()->UnmapNamedBuffer(m_debugVoxelSSBO.GetGPUBuffer().GetGPUID());
				//	}
				//	else 
				//	{
				//		std::cerr << "ERROR: Failed to map debug voxel SSBO!" << std::endl;
				//		GL_CHECK_ERROR(); // Check why mapping failed
				//	}
				//}
				//else {
				//	std::cout << "Debug voxel SSBO size is zero, skipping readback." << std::endl;
				//}

				Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
				Graphics::API()->BindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0); // Unbind debug SSBO
				Graphics::API()->BindImageTexture(0, 0, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
			}
			GL_CHECK_ERROR();
		}

		void UpdateTriangleData(std::vector<glm::vec4>&& tris)
		{
			m_totalSceneTriangles = tris.size() / 3;
			std::cout << "VoxelGridManager: Received " << m_totalSceneTriangles << " triangles." << std::endl; 

			if (!tris.empty())
			{
				size_t dataSizeInBytes = tris.size() * sizeof(glm::vec3);
				std::cout << "VoxelGridManager: Preparing to upload " << dataSizeInBytes << " bytes." << std::endl;

				m_trianglesSSBO.GetDataMutable() = std::move(tris);
				std::cout << "VoxelGridManager: SSBO internal data size: " << m_trianglesSSBO.GetDataImmutable().size() * sizeof(glm::vec3) << " bytes." << std::endl;

				Graphics::UploadToGPUBuffer(m_trianglesSSBO.GetGPUBuffer(), m_trianglesSSBO.GetDataImmutable());
				std::cout << "VoxelGridManager: UploadToGPUBuffer called." << std::endl; 
			}
			else {
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

			GL_CHECK_ERROR(); 
		}

		VoxelGrid& GetVoxelGrid() { return m_voxelGrid; }

	private:
		ShaderProgram* m_voxelizeCompute = nullptr;
		ShaderProgram* m_clearVoxelsCompute = nullptr;

		VoxelGrid m_voxelGrid;
		
		ShaderStorageBuffer<glm::vec4> m_trianglesSSBO;
		ShaderStorageBuffer<DebugVoxelInfo> m_debugVoxelSSBO;
		size_t m_totalSceneTriangles = 0;

		ResourceLoader* m_resLoader = nullptr;
	};
}


#endif