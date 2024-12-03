#include "Geometry.h"
#include "Graphics.h"
#include "Types.h"

#include <array>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp> 

namespace JLEngine
{
	Mesh* Geometry::GenerateSphereMesh(Graphics* graphics, string name, float radius, unsigned int latitudeSegments, unsigned int longitudeSegments)
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

        JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
        vbo.Set(vertices);
        JLEngine::VertexAttribute posAttri(JLEngine::POSITION, 0, 3);
        JLEngine::VertexAttribute normAttri(JLEngine::NORMAL, sizeof(float) * 3, 3);
        JLEngine::VertexAttribute uvAttri(JLEngine::TEX_COORD_2D, sizeof(float) * 6, 2);
        vbo.AddAttribute(posAttri);
        vbo.AddAttribute(normAttri);
        vbo.AddAttribute(uvAttri);
        vbo.CalcStride();

        JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
        ibo.Set(indices);

        auto mesh = new Mesh(0, name);
        mesh->SetVertexBuffer(vbo);
        mesh->AddIndexBuffer(ibo);
        graphics->CreateMesh(mesh);
        return mesh;
	}

    Mesh* Geometry::GenerateBoxMesh(Graphics* graphics, std::string name, float width, float length, float height)
    {
        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<uint32> indices;
        std::vector<glm::vec2> uvs;

        std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32>&> geomData = std::tie(positions, normals, uvs, indices);

        glm::vec3 center(0.0f); 
        int triCount = 0;
        Polygon::AddBox(geomData, triCount, center, width, length, height);

        // Interleave vertex data: positions, normals (calculated), and UVs
        std::vector<float> vertices;
        for (size_t i = 0; i < positions.size(); ++i) {
            // Add position
            vertices.push_back(positions[i].x);
            vertices.push_back(positions[i].y);
            vertices.push_back(positions[i].z);

            vertices.push_back(normals[i].x);
            vertices.push_back(normals[i].y);
            vertices.push_back(normals[i].z);

            // Add UV
            vertices.push_back(uvs[i].x);
            vertices.push_back(uvs[i].y);
        }

        // Create the VertexBuffer
        JLEngine::VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
        vbo.Set(vertices);

        JLEngine::VertexAttribute posAttri(JLEngine::POSITION, 0, 3); // Position attribute
        JLEngine::VertexAttribute normAttri(JLEngine::NORMAL, sizeof(float) * 3, 3); // Normal attribute
        JLEngine::VertexAttribute uvAttri(JLEngine::TEX_COORD_2D, sizeof(float) * 6, 2); // UV attribute
        vbo.AddAttribute(posAttri);
        vbo.AddAttribute(normAttri);
        vbo.AddAttribute(uvAttri);
        vbo.CalcStride();

        // Create the IndexBuffer
        JLEngine::IndexBuffer ibo(GL_ELEMENT_ARRAY_BUFFER, GL_UNSIGNED_INT, GL_STATIC_DRAW);
        ibo.Set(indices);

        // Build the Mesh
        auto mesh = new Mesh(0, name);
        mesh->SetVertexBuffer(vbo);
        mesh->AddIndexBuffer(ibo);
        graphics->CreateMesh(mesh);

        return mesh;
    }

    void Geometry::GenerateInterleavedVertexData(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& texCoords,
        std::vector<float>& vertexData)
    {
        size_t vertexCount = positions.size() / 3; // Assuming vec3 positions

        // Resize the interleaved array
        vertexData.resize(vertexCount * (3 + 3 + 2)); // 3 for position, 3 for normal, 2 for texCoords

        for (size_t i = 0; i < vertexCount; ++i) {
            // Interleave position
            vertexData[i * 8 + 0] = positions[i * 3 + 0];
            vertexData[i * 8 + 1] = positions[i * 3 + 1];
            vertexData[i * 8 + 2] = positions[i * 3 + 2];

            // Interleave normal (if available)
            if (!normals.empty()) {
                vertexData[i * 8 + 3] = normals[i * 3 + 0];
                vertexData[i * 8 + 4] = normals[i * 3 + 1];
                vertexData[i * 8 + 5] = normals[i * 3 + 2];
            }
            else {
                vertexData[i * 8 + 3] = 0.0f;
                vertexData[i * 8 + 4] = 0.0f;
                vertexData[i * 8 + 5] = 1.0f; // Default normal
            }

            // Interleave texture coordinates (if available)
            if (!texCoords.empty()) {
                vertexData[i * 8 + 6] = texCoords[i * 2 + 0];
                vertexData[i * 8 + 7] = texCoords[i * 2 + 1];
            }
            else {
                vertexData[i * 8 + 6] = 0.0f;
                vertexData[i * 8 + 7] = 0.0f; // Default UV
            }
        }
    }

    std::vector<glm::vec3> Geometry::CalculateSmoothNormals(const std::vector<glm::vec3>& positions, const std::vector<uint32>& indices) 
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

    void Geometry::GenerateInterleavedVertexData(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& texCoords,
        const std::vector<float>& tangents,
        std::vector<float>& vertexData)
    {
        size_t vertexCount = positions.size() / 3; // Assuming vec3 positions
        bool hasTexCoords = !texCoords.empty();
        bool hasTangents = !tangents.empty();

        vertexData.clear();
        vertexData.reserve(vertexCount * (3 + 3 + (hasTexCoords ? 2 : 0) + (hasTangents ? 3 : 0)));

        for (size_t i = 0; i < vertexCount; ++i)
        {
            // Add position (vec3)
            vertexData.insert(vertexData.end(), positions.begin() + i * 3, positions.begin() + (i + 1) * 3);

            // Add normal (vec3)
            vertexData.insert(vertexData.end(), normals.begin() + i * 3, normals.begin() + (i + 1) * 3);

            // Add texCoords (vec2) if available
            if (hasTexCoords)
            {
                vertexData.insert(vertexData.end(), texCoords.begin() + i * 2, texCoords.begin() + (i + 1) * 2);
            }

            // Add tangent (vec3) if available
            if (hasTangents)
            {
                vertexData.insert(vertexData.end(), tangents.begin() + i * 3, tangents.begin() + (i + 1) * 3);
            }
        }
    }


    uint32 Polygon::AddFace(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32>&>& geomData,
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

        uint32 bLeft = lastTriCount++; 
        uint32 bRight = lastTriCount++;
        uint32 tLeft = lastTriCount++;
        uint32 tRight = lastTriCount++;

        if (flip) 
        {
            std::array<uint32, 6> triIndices = { bLeft, bRight, tLeft, bRight, tRight, tLeft };
            indices.insert(indices.end(), triIndices.begin(), triIndices.end());
        }
        else 
        {
            std::array<uint32, 6> triIndices = { bLeft, tLeft, bRight, bRight, tLeft, tRight };
            indices.insert(indices.end(), triIndices.begin(), triIndices.end());
        }

        return lastTriCount;
    }

    uint32 Polygon::AddBox(std::tuple<std::vector<glm::vec3>&, std::vector<glm::vec3>&, std::vector<glm::vec2>&, std::vector<uint32>&>& geomData,
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