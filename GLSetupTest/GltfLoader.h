#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <string>
#include <tiny_gltf.h>
#include "Mesh.h"
#include "Graphics.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "TextureManager.h"

namespace JLEngine
{
    /* loads a glb file with a single mesh inside it and returns a JLEngine::Mesh */
	JLEngine::Mesh* LoadModelGLB(std::string fileName, Graphics* graphics);
    
    JLEngine::Mesh* PrimitiveFromMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Primitive& primitive, MeshManager* meshMgr);
    JLEngine::Mesh* MergePrimitivesToMesh(const tinygltf::Model& model, std::string& name, const tinygltf::Mesh& mesh, MeshManager* meshMgr);
    JLEngine::Material* LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, MaterialManager* matMgr, TextureManager* textureMgr);
    JLEngine::Texture* LoadTexture(const tinygltf::Model& model, std::string& name, int textureIndex, TextureManager* textureMgr);

    // Function to extract a vec4 from a tinygltf::Value
    glm::vec4 GetVec4FromValue(const tinygltf::Value& value, const glm::vec4& defaultValue = glm::vec4(1.0f));
    glm::vec3 GetVec3FromValue(const tinygltf::Value& value, const glm::vec3& defaultValue = glm::vec3(1.0f));

    void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadTangentAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& tangentData);
    void LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, JLEngine::IndexBuffer& ibo);
}

#endif