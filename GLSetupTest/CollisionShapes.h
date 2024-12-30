#ifndef COLLISION_SHAPES_H
#define COLLISION_SHAPES_H

#include "Types.h"

#include <glm/glm.hpp>
#include <vector>
#include <iostream>

namespace JLEngine
{
	struct AABB
	{
		glm::vec3 min;
		glm::vec3 max;

		bool HasCollided(AABB& other);
	};

	struct Plane
	{
		Plane() : m_distance(0.0f), m_normal(0.0f) {}
		Plane(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2);
		~Plane();

		float DistanceToPoint(glm::vec3& point);

		float m_distance;
		glm::vec3 m_normal;
	};

	AABB CalculateAABB(const std::vector<float>& positions);
}

#endif