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
