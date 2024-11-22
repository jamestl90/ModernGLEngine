#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace JLEngine
{
	struct Transform3
	{
	public:
		Transform3(glm::vec3& scale, glm::vec3& position, glm::mat4& orientation)
			: m_scale(scale), m_position(position), m_orientation(orientation) {}

		Transform3() 
			: m_scale(glm::vec3(1.0f, 1.0f, 1.0f)), m_position(glm::vec3(0.0f, 0.0f, 0.0f)), m_orientation(glm::mat4()) {}

		~Transform3() {}

		glm::vec3 m_scale;

		glm::vec3 m_position;

		glm::mat4 m_orientation;
	};
}

#endif