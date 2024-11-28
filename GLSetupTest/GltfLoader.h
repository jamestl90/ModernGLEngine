#ifndef GLTF_LOADER_H
#define GLTF_LOADER_H

#include <string>
#include <tiny_gltf.h>
#include "Mesh.h"
#include "Graphics.h"
#include "MeshManager.h"
#include "MaterialManager.h"

namespace JLEngine
{
    /* loads a glb file with a single mesh inside it and returns a JLEngine::Mesh */
	JLEngine::Mesh* LoadModelGLB(std::string fileName, Graphics* graphics);
    
    JLEngine::Mesh* PrimitiveFromMesh(const tinygltf::Model& model, const tinygltf::Primitive& primitive, MeshManager* meshMgr);
    JLEngine::Mesh* MergePrimitivesToMesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, MeshManager* meshMgr);
    JLEngine::Material* LoadMaterial(const tinygltf::Model& model, const tinygltf::Material& gltfMaterial, MaterialManager* matMgr);
    

    void LoadPositionAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadNormalAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadTexCoordAttribute(const tinygltf::Model& model, const tinygltf::Primitive& primitive,
        std::vector<float>& vertexData);
    void LoadIndices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, JLEngine::IndexBuffer& ibo);
}

#endif