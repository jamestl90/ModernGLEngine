#include "CollisionSHapes.h"

namespace JLEngine
{
	bool JLEngine::AABB::HasCollided( AABB& other )
	{
		if (min.x > other.max.x) return false;
		if (min.y > other.max.y) return false;
		if (min.z > other.max.z) return false;
		if (max.x < other.min.x) return false;
		if (max.y < other.min.y) return false;
		if (max.z < other.min.z) return false;

		return true;
	}

	bool AABB::ContainsPoint(const glm::vec3& point, const glm::mat4& worldTransform) const
	{
		// transform all 8 corners to get the world-space AABB
		glm::vec3 corners[8] =
		{
			{ min.x, min.y, min.z },
			{ min.x, min.y, max.z },
			{ min.x, max.y, min.z },
			{ min.x, max.y, max.z },
			{ max.x, min.y, min.z },
			{ max.x, min.y, max.z },
			{ max.x, max.y, min.z },
			{ max.x, max.y, max.z }
		};

		glm::vec3 worldMin = glm::vec3(worldTransform * glm::vec4(corners[0], 1.0f));
		glm::vec3 worldMax = worldMin;

		for (int i = 1; i < 8; ++i)
		{
			glm::vec3 transformed = glm::vec3(worldTransform * glm::vec4(corners[i], 1.0f));
			worldMin = glm::min(worldMin, transformed);
			worldMax = glm::max(worldMax, transformed);
		}

		return point.x >= worldMin.x && point.x <= worldMax.x &&
			point.y >= worldMin.y && point.y <= worldMax.y &&
			point.z >= worldMin.z && point.z <= worldMax.z;
	}

	Plane::Plane( glm::vec3& v0, glm::vec3& v1, glm::vec3& v2 )
	{
		m_normal = v1 - v0;
		m_normal = glm::cross(m_normal, v2 - v0);
		glm::normalize(m_normal);
		m_distance = -glm::dot(m_normal, v0);
	}

	Plane::~Plane()
	{
	}

	float Plane::DistanceToPoint( glm::vec3& point )
	{
		return glm::dot(m_normal, point) + m_distance;
	}

	AABB CalculateAABB(const std::vector<float>& positions)
	{
		if (positions.empty())
		{
			std::cerr << "Cannot create AABB from empty positions" << std::endl;
			return { glm::vec3(0.0f), glm::vec3(0.0f) };
		}

		glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
		glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());

		for (auto i = 0; i < positions.size(); i += 3)
		{
			glm::vec3 position(positions[i], positions[i + 1], positions[i + 2]);
			min = glm::min(min, position);
			max = glm::max(max, position);
		}

		return { min, max };
	}
}
