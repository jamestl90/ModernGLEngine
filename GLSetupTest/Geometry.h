#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <string>
#include <tuple>
#include <glm/glm.hpp>
#include "Mesh.h"
#include "VertexArrayObject.h"

namespace JLEngine
{
	class GraphicsAPI;

	struct Vec3Hash 
	{
		std::size_t operator()(const glm::vec3& v) const 
		{
			return std::hash<float>()(v.x) ^ (std::hash<float>()(v.y) << 1) ^ (std::hash<float>()(v.z) << 2);
		}
	};

	class Geometry
	{
	public:
		static void GenerateSphereMesh(VertexArrayObject& vao, float radius, unsigned int latitudeSegments, unsigned int longitudeSegments);

		static void CreateBox(VertexArrayObject& vao);
		static void CreateScreenSpaceTriangle(VertexArrayObject& vao);
		static void CreateScreenSpaceQuad(VertexArrayObject& vao);

		static void GenerateInterleavedVertexData(const std::vector<float>& positions,
			const std::vector<float>& normals,
			const std::vector<float>& texCoords,
			std::vector<float>& vertexData);
		static void GenerateInterleavedVertexData(std::vector<float>& positions, std::vector<float>& normals, std::vector<float>& texCoords, std::vector<float>& texCoords2, std::vector<float>& tangents, std::vector<float>& vertexData);

		static std::vector<glm::vec3> CalculateSmoothNormals(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices);
		static std::vector<float> CalculateSmoothNormals(const std::vector<float>& positions, const std::vector<uint32_t>& indices);
		static std::vector<float> CalculateFlatNormals(const std::vector<float>& positions, const std::vector<uint32_t>& indices);
		static std::vector<float> CalculateSmoothNormals(const std::vector<float>& positions);
		static std::vector<float> CalculateFlatNormals(const std::vector<float>& positions);

		static std::vector<float> CalculateTangents(const std::vector<float>& positions,
			const std::vector<float>& normals,
			const std::vector<float>& uvs,
			const std::vector<uint32_t>& indices);
		static std::vector<float> CalculateTangents(const std::vector<float>& positions,
			const std::vector<float>& normals,
			const std::vector<float>& uvs);
	};

	class Polygon
	{
	public:
		static uint32_t AddFace(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32_t>&>& geomData,
			int lastTriCount, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, glm::vec3 p4, glm::vec3 normal, glm::vec2 ufBotLeft, glm::vec2 uvOffset, bool flip = false);

		static uint32_t AddBox(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32_t>&>& geomData,
			int triCount, const glm::vec3& center, float width, float length, float height);
	};
}

#endif