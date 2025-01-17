#include "VertexStructures.h"
#include "JLHelpers.h"

#include <iostream>
#include <unordered_map>
#include "VertexArrayObject.h"

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
				{"tangent", AttributeType::TANGENT},
				{"joints_0", AttributeType::JOINT_0},
				{"weights_0", AttributeType::WEIGHT_0 }
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

	uint32_t CalculateStrideInBytes(VertexArrayObject* vao)
	{
		uint32_t stride = 0;
		int floatSize = static_cast<int>(sizeof(float));

		for (uint32_t i = 0; i < 32; ++i)
		{
			if (vao->GetAttribKey() & (1 << i)) // Check if the bit is set
			{
				switch (static_cast<AttributeType>(1 << i))
				{
				case AttributeType::POSITION:
					stride += vao->GetPosCount() * floatSize; // 3 floats
					break;
				case AttributeType::NORMAL:
					stride += 3 * floatSize; // 3 floats
					break;
				case AttributeType::TEX_COORD_0:
				case AttributeType::TEX_COORD_1:
					stride += 2 * floatSize; // 2 floats
					break;
				case AttributeType::COLOUR:
					stride += 4 * floatSize; // 4 floats
					break;
				case AttributeType::TANGENT:
					stride += 4 * floatSize; // 4 floats
					break;
				case AttributeType::JOINT_0:
					stride += 4 * sizeof(uint16_t);
					break;
				case AttributeType::WEIGHT_0:
					stride += 4 * floatSize;
					break;
				default:
					std::cerr << "Unsupported attribute type!" << std::endl;
					break;
				}
			}
		}
		return stride;
	}

	uint32_t CalculateStrideInBytes(VertexAttribKey key, int posCount)
	{
		uint32_t stride = 0;
		int floatSize = static_cast<int>(sizeof(float));

		for (uint32_t i = 0; i < 32; ++i)
		{
			if (key & (1 << i)) // Check if the bit is set
			{
				switch (static_cast<AttributeType>(1 << i))
				{
				case AttributeType::POSITION:
					stride += posCount * floatSize; // 3 floats
					break;
				case AttributeType::NORMAL:
					stride += 3 * floatSize; // 3 floats
					break;
				case AttributeType::TEX_COORD_0:
				case AttributeType::TEX_COORD_1:
					stride += 2 * floatSize; // 2 floats
					break;
				case AttributeType::COLOUR:
					stride += 4 * floatSize; // 4 floats
					break;
				case AttributeType::TANGENT:
					stride += 4 * floatSize; // 4 floats
					break;				
				case AttributeType::JOINT_0:
					stride += 4 * sizeof(uint16_t);
					break;
				case AttributeType::WEIGHT_0:
					stride += 4 * floatSize;
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