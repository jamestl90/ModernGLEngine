#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include "Types.h"

#include <glm/glm.hpp>

namespace JLEngine
{
	struct AABB
	{
		AABB();

		~AABB();

		glm::vec3 min;

		glm::vec3 max;

		bool HasCollided(AABB& other);
	};

	struct Plane
	{
		Plane();

		Plane(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2);

		~Plane();

		float DistanceToPoint(glm::vec3& point);

		float m_distance;
		glm::vec3 m_normal;
	};
}

#endif