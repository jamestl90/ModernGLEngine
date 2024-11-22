#include "VertexStructures.h"

#include <iostream>

namespace JLEngine
{
	//void BuildInterleavedArray(std::vector<float>& vbo, std::vector<uint32>& ibo, aiMesh* mesh)
	//{
	//	for (uint32 i = 0; i < mesh->mNumFaces; i++)
	//	{
	//		aiFace face = mesh->mFaces[i];
	//
	//		ibo.push_back(face.mIndices[0]);
	//		ibo.push_back(face.mIndices[1]);
	//		ibo.push_back(face.mIndices[2]);
	//	}
	//
	//	for (uint32 i = 0; i < mesh->mNumVertices; i++)
	//	{
	//		vbo.push_back(mesh->mVertices[i].x);
	//		vbo.push_back(mesh->mVertices[i].y);
	//		vbo.push_back(mesh->mVertices[i].z);
	//		vbo.push_back(mesh->mNormals[i].x);
	//		vbo.push_back(mesh->mNormals[i].y);
	//		vbo.push_back(mesh->mNormals[i].z);
	//		vbo.push_back(mesh->mTextureCoords[0][i].x);
	//		vbo.push_back(mesh->mTextureCoords[0][i].y);
	//		//vbo.push_back(mesh->mTextureCoords[0][i].z);
	//
	//		if (mesh->HasTangentsAndBitangents())
	//		{
	//			vbo.push_back(mesh->mTangents[i].x);
	//			vbo.push_back(mesh->mTangents[i].y);
	//			vbo.push_back(mesh->mTangents[i].z);
	//			vbo.push_back(mesh->mBitangents[i].x);
	//			vbo.push_back(mesh->mBitangents[i].y);
	//			vbo.push_back(mesh->mBitangents[i].z);
	//		}
	//	}
	//}
}