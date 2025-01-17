#ifndef VERTEX_STRUCTURES_H
#define VERTEX_STRUCTURES_H

#include <map>
#include <string>
#include <glm/glm.hpp>

#include "Types.h"

namespace JLEngine
{
	using VertexAttribKey = uint32_t;

	enum class AttributeType
	{
		POSITION	= 1 << 0,
		NORMAL		= 1 << 1,
		TEX_COORD_0 = 1 << 2,
		COLOUR		= 1 << 3,
		TANGENT		= 1 << 4,
		TEX_COORD_1 = 1 << 5,	
		JOINT_0		= 1 << 6,	
		WEIGHT_0	= 1 << 7,	
		COUNT		= 8			// num elements in this enum 
	};

	class VertexArrayObject;

	AttributeType AttribTypeFromString(const std::string& str);
	VertexAttribKey GenerateVertexAttribKey(const std::map<std::string, int>& glftAttributes);

	void AddToVertexAttribKey(VertexAttribKey& key, AttributeType type);
	void RemoveFromVertexAttribKey(VertexAttribKey& key, AttributeType type);
	bool HasVertexAttribKey(uint32_t mask, AttributeType attribute);

	uint32_t CalculateStrideInBytes(VertexArrayObject* vao);
	uint32_t CalculateStrideInBytes(VertexAttribKey key, int posCount = 3);
}

#endif