#include "Geometry.h"
#include "GraphicsAPI.h"
#include "Types.h"

#include <unordered_map>
#include <stdexcept>
#include <array>
#include <vector>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtc/constants.hpp> 

namespace JLEngine
{
	void Geometry::GenerateSphereMesh(VertexArrayObject& vao, float radius, unsigned int latitudeSegments, unsigned int longitudeSegments)
	{
        std::vector<float> vertices;
        std::vector<unsigned int> indices;

        constexpr float pi = glm::pi<float>();

        for (unsigned int lat = 0; lat <= latitudeSegments; ++lat) 
        {
            float theta = lat * pi / latitudeSegments; // Latitude angle
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            for (unsigned int lon = 0; lon <= longitudeSegments; ++lon) 
            {
                float phi = lon * 2.0f * pi / longitudeSegments; // Longitude angle
                float sinPhi = sin(phi);
                float cosPhi = cos(phi);

                // Calculate position
                float x = radius * sinTheta * cosPhi;
                float y = radius * cosTheta;
                float z = radius * sinTheta * sinPhi;

                // Calculate normal (normalized position vector for a sphere)
                float nx = sinTheta * cosPhi;
                float ny = cosTheta;
                float nz = sinTheta * sinPhi;

                // Calculate UV coordinates
                float u = static_cast<float>(lon) / longitudeSegments;
                float v = static_cast<float>(lat) / latitudeSegments;

                // Interleave Position, Normal, and UV
                vertices.push_back(x); // Position
                vertices.push_back(y);
                vertices.push_back(z);
                vertices.push_back(nx); // Normal
                vertices.push_back(ny);
                vertices.push_back(nz);
                vertices.push_back(u); // UV
                vertices.push_back(v);
            }
        }

        // Generate indices
        for (unsigned int lat = 0; lat < latitudeSegments; ++lat) 
        {
            for (unsigned int lon = 0; lon < longitudeSegments; ++lon) 
            {
                unsigned int first = (lat * (longitudeSegments + 1)) + lon;
                unsigned int second = first + longitudeSegments + 1;

                // Triangle 1
                indices.push_back(first);
                indices.push_back(second);
                indices.push_back(first + 1);

                // Triangle 2
                indices.push_back(second);
                indices.push_back(second + 1);
                indices.push_back(first + 1);
            }
        }

        VertexBuffer vbo;
        vbo.Set(std::move(vertices));

        vao.AddAttribute(JLEngine::AttributeType::POSITION);
        vao.AddAttribute(JLEngine::AttributeType::NORMAL);
        vao.AddAttribute(JLEngine::AttributeType::TEX_COORD_0);

        vao.CalcStride();
        vao.SetVertexBuffer(vbo);

        IndexBuffer ibo;
        ibo.Set(std::move(indices));
        vao.SetIndexBuffer(ibo);
	}

    void Geometry::CreateBox(VertexArrayObject& vao)
    {
        const float boxVertices[] =
        {
            // Positions          
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        };

        std::vector<float> verts;
        verts.insert(verts.end(), std::begin(boxVertices), std::end(boxVertices));

        VertexBuffer vbo;
        vbo.Set(std::move(verts));

        vao.SetVertexBuffer(vbo);
        vao.AddAttribute(JLEngine::AttributeType::POSITION);
        vao.CalcStride();
    }

    void Geometry::CreateScreenSpaceTriangle(VertexArrayObject& vao)
    {
        const float triangleVertices[] =
        {
            // Positions        // UVs
            -1.0f, -3.0f,       0.0f, 2.0f,
             3.0f,  1.0f,       2.0f, 0.0f,
            -1.0f,  1.0f,       0.0f, 0.0f
        };
        std::vector<float> triVerts;
        triVerts.insert(triVerts.end(), std::begin(triangleVertices), std::end(triangleVertices));

        VertexBuffer vbo;
        vbo.Set(std::move(triVerts));

        vao.SetVertexBuffer(vbo);
        vao.AddAttribute(JLEngine::AttributeType::POSITION);
        vao.SetPosCount(2);
        vao.AddAttribute(JLEngine::AttributeType::TEX_COORD_0);
        vao.CalcStride();
    }

    void Geometry::CreateScreenSpaceQuad(VertexArrayObject& vao)
    {
        const float quadVerticesArray[] = 
        {
            -1.0f, -1.0f,   0.0f, 0.0f, // Bottom-left
             1.0f, -1.0f,   1.0f, 0.0f, // Bottom-right
            -1.0f,  1.0f,   0.0f, 1.0f, // Top-left
             1.0f,  1.0f,   1.0f, 1.0f  // Top-right
        };

        const unsigned int quadIndicesArray[] = 
        {
            0, 1, 2, 
            1, 3, 2  
        };

        std::vector<float> quadVerts;
        quadVerts.insert(quadVerts.end(), std::begin(quadVerticesArray), std::end(quadVerticesArray));

        VertexBuffer vbo;
        vbo.Set(std::move(quadVerts));
        vao.SetVertexBuffer(vbo);

        vao.AddAttribute(JLEngine::AttributeType::POSITION);
        vao.SetPosCount(2);
        vao.AddAttribute(JLEngine::AttributeType::TEX_COORD_0);
        vao.CalcStride();

        std::vector<uint32_t> quadIndices;
        quadIndices.insert(quadIndices.end(), std::begin(quadIndicesArray), std::end(quadIndicesArray));

        IndexBuffer ibo;
        ibo.Set(std::move(quadIndices));
        vao.SetIndexBuffer(ibo);
    }

    void Geometry::GenerateInterleavedVertexData(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& texCoords,
        std::vector<std::byte>& vertexData)
    {
        size_t vertexCount = positions.size() / 3;
        if (positions.size() % 3 != 0) {
            std::cerr << "Error: Positions array size is not a multiple of 3." << std::endl;
            return;
        }

        bool hasNormals = !normals.empty() && normals.size() == vertexCount * 3;
        bool hasTexCoords = !texCoords.empty() && texCoords.size() == vertexCount * 2;

        size_t vertexSize = sizeof(float) * 3;
        if (hasNormals) vertexSize += sizeof(float) * 3;
        if (hasTexCoords) vertexSize += sizeof(float) * 2;

        vertexData.clear();
        vertexData.resize(vertexCount * vertexSize);

        auto insertFloats = [](std::byte* dest, const float* src, size_t count) {
            std::memcpy(dest, src, sizeof(float) * count);
            return dest + sizeof(float) * count;
            };

        std::byte* dataPtr = vertexData.data();
        for (size_t i = 0; i < vertexCount; ++i) {
            dataPtr = insertFloats(dataPtr, &positions[i * 3], 3);

            if (hasNormals)
                dataPtr = insertFloats(dataPtr, &normals[i * 3], 3);

            if (hasTexCoords)
                dataPtr = insertFloats(dataPtr, &texCoords[i * 2], 2);
        }
    }

    void Geometry::GenerateInterleavedVertexData(std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texCoords,
        std::vector<float>& texCoords2,
        std::vector<float>& tangents,
        std::vector<std::byte>& vertexData)
    {
        size_t vertexCount = positions.size() / 3; 
        if (positions.size() % 3 != 0) {
            std::cerr << "Error: Positions array size is not a multiple of 3." << std::endl;
            return;
        }

        bool hasNormals = !normals.empty() && normals.size() == vertexCount * 3;
        bool hasTexCoords = !texCoords.empty() && texCoords.size() == vertexCount * 2;
        bool hasTexCoords2 = !texCoords2.empty() && texCoords2.size() == vertexCount * 2;
        bool hasTangents = !tangents.empty() && tangents.size() == vertexCount * 4;

        size_t vertexSize = sizeof(float) * 3; 
        if (hasNormals) vertexSize += sizeof(float) * 3;   
        if (hasTexCoords) vertexSize += sizeof(float) * 2; 
        if (hasTexCoords2) vertexSize += sizeof(float) * 2;
        if (hasTangents) vertexSize += sizeof(float) * 4;  

        vertexData.clear();
        vertexData.resize(vertexCount * vertexSize);

        auto insertFloats = [](std::byte* dest, const float* src, size_t count) {
            std::memcpy(dest, src, sizeof(float) * count);
            return dest + sizeof(float) * count;
            };

        std::byte* dataPtr = vertexData.data();
        for (size_t i = 0; i < vertexCount; ++i) {
            dataPtr = insertFloats(dataPtr, &positions[i * 3], 3);

            if (hasNormals)
                dataPtr = insertFloats(dataPtr, &normals[i * 3], 3);

            if (hasTexCoords)
                dataPtr = insertFloats(dataPtr, &texCoords[i * 2], 2);

            if (hasTexCoords2)
                dataPtr = insertFloats(dataPtr, &texCoords2[i * 2], 2);

            if (hasTangents)
                dataPtr = insertFloats(dataPtr, &tangents[i * 4], 4);
        }
    }

    void Geometry::GenerateInterleavedVertexData(std::vector<float>& positions,
        std::vector<float>& normals,
        std::vector<float>& texCoords,
        std::vector<float>& tangents,
        std::vector<float>& weights,
        std::vector<uint16_t>& joints,
        std::vector<std::byte>& vertexData)
    {
        size_t vertexCount = positions.size() / 3; // Number of vertices based on positions
        if (positions.size() % 3 != 0) {
            std::cerr << "Error: Positions array size is not a multiple of 3." << std::endl;
            return;
        }

        bool hasNormals = !normals.empty() && normals.size() == vertexCount * 3;
        bool hasTexCoords = !texCoords.empty() && texCoords.size() == vertexCount * 2;
        bool hasTangents = !tangents.empty() && tangents.size() == vertexCount * 4;
        bool hasWeights = !weights.empty() && weights.size() == vertexCount * 4;
        bool hasJoints = !joints.empty() && joints.size() == vertexCount * 4;

        size_t vertexSize = sizeof(float) * 3; // Position is always included
        if (hasNormals) vertexSize += sizeof(float) * 3;
        if (hasTexCoords) vertexSize += sizeof(float) * 2;
        if (hasTangents) vertexSize += sizeof(float) * 4;
        if (hasWeights) vertexSize += sizeof(float) * 4;
        if (hasJoints) vertexSize += sizeof(uint16_t) * 4;

        vertexData.clear();
        vertexData.resize(vertexCount * vertexSize);

        auto insertFloats = [](std::byte* dest, const float* src, size_t count) {
            std::memcpy(dest, src, sizeof(float) * count);
            return dest + sizeof(float) * count;
            };

        auto insertUint16s = [](std::byte* dest, const uint16_t* src, size_t count) {
            std::memcpy(dest, src, sizeof(uint16_t) * count);
            return dest + sizeof(uint16_t) * count;
            };

        std::byte* dataPtr = vertexData.data();
        for (size_t i = 0; i < vertexCount; ++i) 
        {
            dataPtr = insertFloats(dataPtr, &positions[i * 3], 3);

            if (hasNormals)
                dataPtr = insertFloats(dataPtr, &normals[i * 3], 3);

            if (hasTexCoords)
                dataPtr = insertFloats(dataPtr, &texCoords[i * 2], 2);

            if (hasTangents)
                dataPtr = insertFloats(dataPtr, &tangents[i * 4], 4);

            if (hasJoints)
                dataPtr = insertUint16s(dataPtr, &joints[i * 4], 4);

            if (hasWeights)
                dataPtr = insertFloats(dataPtr, &weights[i * 4], 4);
        }
    }

    std::vector<glm::vec3> Geometry::CalculateSmoothNormals(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices) 
    {
        // Map to store accumulated normals for each vertex
        std::unordered_map<glm::vec3, glm::vec3, Vec3Hash> vertexNormals;
        std::unordered_map<glm::vec3, int, Vec3Hash> vertexCounts;

        // Iterate over triangles to calculate face normals
        for (size_t i = 0; i < indices.size(); i += 3) 
        {
            glm::vec3 v0 = positions[indices[i]];
            glm::vec3 v1 = positions[indices[i + 1]];
            glm::vec3 v2 = positions[indices[i + 2]];

            // Calculate face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // Accumulate the normal for each vertex of the triangle
            vertexNormals[v0] += faceNormal;
            vertexNormals[v1] += faceNormal;
            vertexNormals[v2] += faceNormal;

            // Track the number of times each vertex is used
            vertexCounts[v0]++;
            vertexCounts[v1]++;
            vertexCounts[v2]++;
        }

        // Normalize the accumulated normals and produce the smooth normals
        std::vector<glm::vec3> smoothNormals(positions.size());
        for (size_t i = 0; i < positions.size(); ++i) 
        {
            const glm::vec3& pos = positions[i];
            if (vertexCounts[pos] > 0) 
            {
                smoothNormals[i] = glm::normalize(vertexNormals[pos] / static_cast<float>(vertexCounts[pos]));
            }
        }

        return smoothNormals;
    }

    // Function to calculate smooth normals
    std::vector<float> Geometry::CalculateSmoothNormals(const std::vector<float>& positions)
    {
        if (positions.size() % 3 != 0) 
        {
            throw std::invalid_argument("Positions size must be a multiple of 3 (x, y, z for each vertex).");
        }

        // Convert flat positions to glm::vec3 for easier handling
        std::vector<glm::vec3> vertices;
        for (size_t i = 0; i < positions.size(); i += 3) 
        {
            vertices.emplace_back(positions[i], positions[i + 1], positions[i + 2]);
        }

        // Smooth normals storage
        std::vector<glm::vec3> smoothNormals(vertices.size(), glm::vec3(0.0f));

        // Map to accumulate normals per vertex
        std::unordered_map<glm::vec3, glm::vec3> normalMap;

        // Iterate through each triangle
        for (size_t i = 0; i < vertices.size(); i += 3) 
        {
            const glm::vec3& v0 = vertices[i];
            const glm::vec3& v1 = vertices[i + 1];
            const glm::vec3& v2 = vertices[i + 2];

            // Compute face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // Accumulate the face normal for each vertex
            normalMap[v0] += faceNormal;
            normalMap[v1] += faceNormal;
            normalMap[v2] += faceNormal;
        }

        // Flatten the normals into a vector<float>
        std::vector<float> flatNormals;
        flatNormals.reserve(vertices.size() * 3); // Preallocate memory

        for (size_t i = 0; i < vertices.size(); ++i) {
            glm::vec3 normalized = glm::normalize(normalMap[vertices[i]]);
            flatNormals.push_back(normalized.x);
            flatNormals.push_back(normalized.y);
            flatNormals.push_back(normalized.z);
        }

        return flatNormals;
    }

    std::vector<float> Geometry::CalculateFlatNormals(const std::vector<float>& positions) 
    {
        if (positions.size() % 9 != 0) 
        {
            throw std::invalid_argument("Positions size must be a multiple of 9 (3 vertices per triangle).");
        }

        // Storage for flat normals
        std::vector<float> flatNormals;
        flatNormals.reserve(positions.size()); // Preallocate memory

        // Iterate through each triangle
        for (size_t i = 0; i < positions.size(); i += 9) 
        {
            // Extract triangle vertices
            glm::vec3 v0(positions[i], positions[i + 1], positions[i + 2]);
            glm::vec3 v1(positions[i + 3], positions[i + 4], positions[i + 5]);
            glm::vec3 v2(positions[i + 6], positions[i + 7], positions[i + 8]);

            // Compute face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // Add the same normal for all three vertices
            for (int j = 0; j < 3; ++j) 
            {
                flatNormals.push_back(faceNormal.x);
                flatNormals.push_back(faceNormal.y);
                flatNormals.push_back(faceNormal.z);
            }
        }

        return flatNormals;
    }

    std::vector<float> Geometry::CalculateSmoothNormals(const std::vector<float>& positions, const std::vector<uint32_t>& indices) 
    {
        if (positions.size() % 3 != 0) 
        {
            throw std::invalid_argument("Positions size must be a multiple of 3 (x, y, z per vertex).");
        }
        if (indices.size() % 3 != 0) 
        {
            throw std::invalid_argument("Indices size must be a multiple of 3 (triangle indices).");
        }

        // Number of vertices
        size_t vertexCount = positions.size() / 3;

        // Storage for smooth normals
        std::vector<glm::vec3> smoothNormals(vertexCount, glm::vec3(0.0f));

        // Iterate through each triangle
        for (size_t i = 0; i < indices.size(); i += 3) 
        {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            // Get vertex positions
            glm::vec3 v0(positions[i0 * 3], positions[i0 * 3 + 1], positions[i0 * 3 + 2]);
            glm::vec3 v1(positions[i1 * 3], positions[i1 * 3 + 1], positions[i1 * 3 + 2]);
            glm::vec3 v2(positions[i2 * 3], positions[i2 * 3 + 1], positions[i2 * 3 + 2]);

            // Compute face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // Accumulate normals for each vertex
            smoothNormals[i0] += faceNormal;
            smoothNormals[i1] += faceNormal;
            smoothNormals[i2] += faceNormal;
        }

        // Normalize the accumulated normals for each vertex
        std::vector<float> flatNormals;
        flatNormals.reserve(positions.size());
        for (const auto& normal : smoothNormals) 
        {
            glm::vec3 normalized = glm::normalize(normal);
            flatNormals.push_back(normalized.x);
            flatNormals.push_back(normalized.y);
            flatNormals.push_back(normalized.z);
        }

        return flatNormals;
    }

    std::vector<float> Geometry::CalculateFlatNormals(
        const std::vector<float>& positions,
        const std::vector<uint32_t>& indices)
    {
        if (positions.size() % 3 != 0)
        {
            throw std::invalid_argument("Positions size must be a multiple of 3 (x, y, z per vertex).");
        }
        if (indices.size() % 3 != 0)
        {
            throw std::invalid_argument("Indices size must be a multiple of 3 (triangle indices).");
        }

        // Normal array: One normal per vertex (same size as positions)
        std::vector<float> flatNormals(positions.size(), 0.0f); // Initialized to zero

        // Iterate through each triangle
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i] * 3;
            uint32_t i1 = indices[i + 1] * 3;
            uint32_t i2 = indices[i + 2] * 3;

            // Get vertex positions
            glm::vec3 v0(positions[i0], positions[i0 + 1], positions[i0 + 2]);
            glm::vec3 v1(positions[i1], positions[i1 + 1], positions[i1 + 2]);
            glm::vec3 v2(positions[i2], positions[i2 + 1], positions[i2 + 2]);

            // Compute face normal
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec3 faceNormal = glm::normalize(glm::cross(edge1, edge2));

            // Accumulate normals for each vertex (flat shading)
            for (int j = 0; j < 3; ++j)
            {
                flatNormals[i0 + j] = faceNormal[j];
                flatNormals[i1 + j] = faceNormal[j];
                flatNormals[i2 + j] = faceNormal[j];
            }
        }

        return flatNormals;
    }

    std::vector<float> Geometry::CalculateTangents(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& uvs,
        const std::vector<uint32_t>& indices)
    {
        if (positions.size() % 3 != 0 || normals.size() % 3 != 0 || uvs.size() % 2 != 0)
        {
            throw std::invalid_argument("Invalid input sizes for positions, normals, or UVs.");
        }

        if (positions.size() / 3 != uvs.size() / 2)
        {
            throw std::invalid_argument("Mismatch between positions and UVs.");
        }

        size_t vertexCount = positions.size() / 3;
        std::vector<glm::vec3> tangents(vertexCount, glm::vec3(0.0f));
        std::vector<glm::vec3> bitangents(vertexCount, glm::vec3(0.0f));

        // Iterate over the indices in groups of three (triangle list format)
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            glm::vec3 v0(positions[i0 * 3], positions[i0 * 3 + 1], positions[i0 * 3 + 2]);
            glm::vec3 v1(positions[i1 * 3], positions[i1 * 3 + 1], positions[i1 * 3 + 2]);
            glm::vec3 v2(positions[i2 * 3], positions[i2 * 3 + 1], positions[i2 * 3 + 2]);

            glm::vec2 uv0(uvs[i0 * 2], uvs[i0 * 2 + 1]);
            glm::vec2 uv1(uvs[i1 * 2], uvs[i1 * 2 + 1]);
            glm::vec2 uv2(uvs[i2 * 2], uvs[i2 * 2 + 1]);

            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * r;

            // Accumulate tangents and bitangents
            tangents[i0] += tangent;
            tangents[i1] += tangent;
            tangents[i2] += tangent;

            bitangents[i0] += bitangent;
            bitangents[i1] += bitangent;
            bitangents[i2] += bitangent;
        }

        // Normalize and flatten tangents with w-component
        std::vector<float> flatTangents;
        flatTangents.reserve(vertexCount * 4);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            glm::vec3 normal(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);

            // Orthogonalize tangent to normal
            glm::vec3 tangent = glm::normalize(tangents[i] - normal * glm::dot(normal, tangents[i]));

            // Calculate handedness (w-component)
            float w = (glm::dot(glm::cross(normal, tangent), bitangents[i]) < 0.0f) ? -1.0f : 1.0f;

            flatTangents.push_back(tangent.x);
            flatTangents.push_back(tangent.y);
            flatTangents.push_back(tangent.z);
            flatTangents.push_back(w); // Handedness
        }

        return flatTangents;
    }

    std::vector<float> Geometry::CalculateTangents(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& uvs)
    {
        if (positions.size() % 3 != 0 || normals.size() % 3 != 0 || uvs.size() % 2 != 0)
        {
            throw std::invalid_argument("Invalid input sizes for positions, normals, or UVs.");
        }

        if (positions.size() / 3 != uvs.size() / 2)
        {
            throw std::invalid_argument("Mismatch between positions and UVs.");
        }

        size_t vertexCount = positions.size() / 3;
        std::vector<glm::vec3> tangents(vertexCount, glm::vec3(0.0f));
        std::vector<glm::vec3> bitangents(vertexCount, glm::vec3(0.0f));

        // Iterate through the vertices in groups of three (triangle list format)
        for (size_t i = 0; i < vertexCount; i += 3)
        {
            // Get vertex positions
            glm::vec3 v0(positions[i * 3], positions[i * 3 + 1], positions[i * 3 + 2]);
            glm::vec3 v1(positions[(i + 1) * 3], positions[(i + 1) * 3 + 1], positions[(i + 1) * 3 + 2]);
            glm::vec3 v2(positions[(i + 2) * 3], positions[(i + 2) * 3 + 1], positions[(i + 2) * 3 + 2]);

            // Get UV coordinates
            glm::vec2 uv0(uvs[i * 2], uvs[i * 2 + 1]);
            glm::vec2 uv1(uvs[(i + 1) * 2], uvs[(i + 1) * 2 + 1]);
            glm::vec2 uv2(uvs[(i + 2) * 2], uvs[(i + 2) * 2 + 1]);

            // Calculate edges and delta UVs
            glm::vec3 edge1 = v1 - v0;
            glm::vec3 edge2 = v2 - v0;
            glm::vec2 deltaUV1 = uv1 - uv0;
            glm::vec2 deltaUV2 = uv2 - uv0;

            float r = 1.0f / (deltaUV1.x * deltaUV2.y - deltaUV1.y * deltaUV2.x);
            glm::vec3 tangent = (edge1 * deltaUV2.y - edge2 * deltaUV1.y) * r;
            glm::vec3 bitangent = (edge2 * deltaUV1.x - edge1 * deltaUV2.x) * r;

            // Accumulate tangents and bitangents for the triangle's vertices
            tangents[i] += tangent;
            tangents[i + 1] += tangent;
            tangents[i + 2] += tangent;

            bitangents[i] += bitangent;
            bitangents[i + 1] += bitangent;
            bitangents[i + 2] += bitangent;
        }

        // Normalize and flatten tangents with w-component
        std::vector<float> flatTangents;
        flatTangents.reserve(vertexCount * 4);

        for (size_t i = 0; i < vertexCount; ++i)
        {
            glm::vec3 normal(normals[i * 3], normals[i * 3 + 1], normals[i * 3 + 2]);

            // Orthogonalize tangent to normal
            glm::vec3 tangent = glm::normalize(tangents[i] - normal * glm::dot(normal, tangents[i]));

            // Calculate handedness (w-component)
            float w = (glm::dot(glm::cross(normal, tangent), bitangents[i]) < 0.0f) ? -1.0f : 1.0f;

            flatTangents.push_back(tangent.x);
            flatTangents.push_back(tangent.y);
            flatTangents.push_back(tangent.z);
            flatTangents.push_back(w); // Handedness
        }

        return flatTangents;
    }

    uint32_t Polygon::AddFace(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32_t>&>& geomData,
        int lastTriCount, glm::vec3 p1, glm::vec3 p2, glm::vec3 p3, 
        glm::vec3 p4, glm::vec3 normal, glm::vec2 uvBotLeft, glm::vec2 uvOffset, bool flip)
    {
        auto& positions = std::get<0>(geomData);
        auto& normals = std::get<1>(geomData);
        auto& uvs = std::get<2>(geomData);
        auto& indices = std::get<3>(geomData);

        positions.push_back(p1);
        positions.push_back(p2);
        positions.push_back(p3);
        positions.push_back(p4);

        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);

        uvs.push_back(uvBotLeft);
        uvs.push_back(uvBotLeft + glm::vec2(uvOffset.x, 0.0f));
        uvs.push_back(uvBotLeft + glm::vec2(0.0f, uvOffset.y));
        uvs.push_back(uvBotLeft + uvOffset);

        uint32_t bLeft = lastTriCount++; 
        uint32_t bRight = lastTriCount++;
        uint32_t tLeft = lastTriCount++;
        uint32_t tRight = lastTriCount++;

        if (flip) 
        {
            std::array<uint32_t, 6> triIndices = { bLeft, bRight, tLeft, bRight, tRight, tLeft };
            indices.insert(indices.end(), triIndices.begin(), triIndices.end());
        }
        else 
        {
            std::array<uint32_t, 6> triIndices = { bLeft, tLeft, bRight, bRight, tLeft, tRight };
            indices.insert(indices.end(), triIndices.begin(), triIndices.end());
        }

        return lastTriCount;
    }

    uint32_t Polygon::AddBox(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32_t>&>& geomData,
        int triCount, const glm::vec3& center, float width, float length, float height)
    {
        float halfWidth = width * 0.5f;
        float halfLength = length * 0.5f;
        float halfHeight = height * 0.5f;

        // Calculate the 8 corners of the box
        glm::vec3 bLeft = glm::vec3(-halfWidth, -halfHeight, -halfLength) + center;
        glm::vec3 bRight = glm::vec3(halfWidth, -halfHeight, -halfLength) + center;
        glm::vec3 tLeft = glm::vec3(-halfWidth, -halfHeight, halfLength) + center;
        glm::vec3 tRight = glm::vec3(halfWidth, -halfHeight, halfLength) + center;

        glm::vec3 bLeft2 = glm::vec3(-halfWidth, halfHeight, -halfLength) + center;
        glm::vec3 bRight2 = glm::vec3(halfWidth, halfHeight, -halfLength) + center;
        glm::vec3 tLeft2 = glm::vec3(-halfWidth, halfHeight, halfLength) + center;
        glm::vec3 tRight2 = glm::vec3(halfWidth, halfHeight, halfLength) + center;

        glm::vec3 UP(0.0f, 1.0f, 0.0f);
        glm::vec3 DOWN(0.0f, -1.0f, 0.0f);
        glm::vec3 RIGHT(1.0f, 0.0f, 0.0f);
        glm::vec3 LEFT(-1.0f, 0.0f, 0.0f);
        glm::vec3 FORWARD(0.0f, 0.0f, -1.0f); // Assuming -Z is forward
        glm::vec3 BACKWARD(0.0f, 0.0f, 1.0f);

        // Add bottom and top faces
        triCount = Polygon::AddFace(geomData, triCount, bLeft, bRight, tLeft, tRight, DOWN, glm::vec2(0.0f), glm::vec2(1.0f), true);
        triCount = Polygon::AddFace(geomData, triCount, bLeft2, bRight2, tLeft2, tRight2, UP, glm::vec2(0.0f), glm::vec2(1.0f));

        // Add front and back faces
        triCount = Polygon::AddFace(geomData, triCount, bLeft, bRight, bLeft2, bRight2, FORWARD , glm::vec2(0.0f), glm::vec2(1.0f));
        triCount = Polygon::AddFace(geomData, triCount, tRight, tLeft, tRight2, tLeft2, BACKWARD, glm::vec2(0.0f), glm::vec2(1.0f));

        // Add left and right faces
        triCount = Polygon::AddFace(geomData, triCount, tLeft, bLeft, tLeft2, bLeft2, LEFT, glm::vec2(0.0f), glm::vec2(1.0f));
        triCount = Polygon::AddFace(geomData, triCount, bRight, tRight, bRight2, tRight2, RIGHT, glm::vec2(0.0f), glm::vec2(1.0f));

        return triCount;
    }
}