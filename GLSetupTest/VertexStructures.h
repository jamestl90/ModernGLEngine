#ifndef VERTEX_STRUCTURES_H
#define VERTEX_STRUCTURES_H

#include <vector>
#include <glm/glm.hpp>

#include "Types.h"

namespace JLEngine
{
	enum AttributeType
	{
		POSITION = 0,
		NORMAL = 1,
		TEX_COORD_2D = 2,
		COLOUR = 3,
		TANGENT = 4,
		BINORMAL = 5
	};

	struct VertexAttribute
	{
	public:
		VertexAttribute(AttributeType type, uint16 offset, uint16 count) 
			: m_type(type), m_offset(offset), m_count(count) {}

		AttributeType m_type;
		uint16 m_offset;
		uint16 m_count;

		// for use in std::set
		friend bool operator < (const VertexAttribute& lhs, const VertexAttribute& rhs)
		{
			return (lhs.m_type < rhs.m_type);
		}
	};
}

#endif