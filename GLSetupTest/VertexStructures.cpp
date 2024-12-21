#include "VertexStructures.h"
#include "JLHelpers.h"

#include <iostream>
#include <unordered_map>

namespace JLEngine
{
	AttributeType AttribTypeFromString(const std::string& str)
	{
		auto lowerStr = JLEngine::Str::ToLower(str);
		static const std::unordered_map<std::string, AttributeType> attribMap = 
		{
				{"position", AttributeType::POSITION},
				{"normal", AttributeType::NORMAL},
				{"texcoord_0", AttributeType::TEX_COORD_0},
				{"texcoord_1", AttributeType::TEX_COORD_1},
				{"color", AttributeType::COLOUR},
				{"tangent", AttributeType::TANGENT}
		};

		auto it = attribMap.find(lowerStr);
		if (it != attribMap.end()) return it->second;

		throw std::invalid_argument("Unrecognized attribute name: " + str);
	}

	VertexAttribKey GenerateVertexAttribKey(const std::map<std::string, int>& glftAttributes)
	{
		VertexAttribKey key = 0;
		try
		{
			for (const auto& [name, accessorIndex] : glftAttributes)
			{
				auto attribType = AttribTypeFromString(name);
				key |= static_cast<uint32_t>(attribType);
			}
		}
		catch (const std::exception& e)
		{
			std::cerr << "Error generating vertex attribute key: " << e.what() << std::endl;
		}
		return key;
	}

	void AddToVertexAttribKey(VertexAttribKey& key, AttributeType type)
	{
		key |= static_cast<uint32_t>(type);
	}

	void RemoveFromVertexAttribKey(VertexAttribKey& key, AttributeType type)
	{
		key &= ~static_cast<uint32_t>(type);
	}

	bool HasVertexAttribKey(uint32_t mask, AttributeType attribute)
	{
		return (mask & static_cast<uint32_t>(attribute)) != 0;
	}

	uint32_t CalculateStride(VertexAttribKey vertexAttribKey)
	{
		uint32_t stride = 0;

		for (uint32_t i = 0; i < 32; ++i)
		{
			if (vertexAttribKey & (1 << i)) // Check if the bit is set
			{
				switch (static_cast<AttributeType>(1 << i))
				{
				case AttributeType::POSITION:
					stride += 3 * sizeof(float); // 3 floats
					break;
				case AttributeType::NORMAL:
					stride += 3 * sizeof(float); // 3 floats
					break;
				case AttributeType::TEX_COORD_0:
				case AttributeType::TEX_COORD_1:
					stride += 2 * sizeof(float); // 2 floats
					break;
				case AttributeType::COLOUR:
					stride += 4 * sizeof(float); // 4 floats
					break;
				case AttributeType::TANGENT:
					stride += 4 * sizeof(float); // 4 floats
					break;
				default:
					std::cerr << "Unsupported attribute type!" << std::endl;
					break;
				}
			}
		}
		return stride;
	}

}