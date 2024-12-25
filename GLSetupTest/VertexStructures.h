#ifndef VERTEX_STRUCTURES_H
#define VERTEX_STRUCTURES_H

#include <map>
#include <string>
#include <glm/glm.hpp>

#include "Types.h"

namespace JLEngine
{
	enum class AttributeType
	{
		POSITION	= 1 << 0,
		NORMAL		= 1 << 1,
		TEX_COORD_0 = 1 << 2,
		COLOUR		= 1 << 3,
		TANGENT		= 1 << 4,
		TEX_COORD_1 = 1 << 5,
		POSITION2F	= 1 << 6,
		COUNT		= 7			// num elements in this enum 
	};

	using VertexAttribKey = uint32_t;

	struct VertexAttribute
	{
	public:
		VertexAttribute(AttributeType type, uint16_t offset, uint16_t count) 
			: m_type(type), m_offset(offset), m_count(count) {}

		AttributeType m_type;
		uint16_t m_offset;
		uint16_t m_count;

		// for use in std::set
		friend bool operator < (const VertexAttribute& lhs, const VertexAttribute& rhs)
		{
			return (lhs.m_type < rhs.m_type);
		}
	};

	AttributeType AttribTypeFromString(const std::string& str);

	VertexAttribKey GenerateVertexAttribKey(const std::map<std::string, int>& glftAttributes);

	void AddToVertexAttribKey(VertexAttribKey& key, AttributeType type);
	void RemoveFromVertexAttribKey(VertexAttribKey& key, AttributeType type);
	bool HasVertexAttribKey(uint32_t mask, AttributeType attribute);

	uint32_t CalculateStride(VertexAttribKey vertexAttribKey);
}

#endif