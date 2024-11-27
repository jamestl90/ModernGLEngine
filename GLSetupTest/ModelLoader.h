#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <string>
#include <tiny_gltf.h>
#include "Mesh.h"
#include "Graphics.h"

namespace JLEngine
{
	JLEngine::Mesh* LoadModel(std::string fileName, Graphics* graphics);

    void GenerateInterleavedVertexData(const std::vector<float>& positions,
        const std::vector<float>& normals,
        const std::vector<float>& texCoords,
        std::vector<float>& vertexData);

    void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, JLEngine::IndexBuffer& ibo);
}

#endif