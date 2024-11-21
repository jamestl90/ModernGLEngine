#include "Primitives.h"

namespace JLEngine
{
	AABB::AABB() : max(1.0f), min(1.0f)
	{
	}

	AABB::~AABB()
	{
	}

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

	Plane::Plane()
	{
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
}
