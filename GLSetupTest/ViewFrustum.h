#ifndef VIEWFRUSTUM_H
#define VIEWFRUSTUM_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtx/compatibility.hpp>
#include "Primitives.h"
#include "Window.h"

#include <iostream>

using std::cout;
using std::endl;

namespace JLEngine
{
	class ViewFrustum
	{
	public:
		 ViewFrustum(float fov, float aspect, float near, float far);
		 ~ViewFrustum();

		 void UpdatePerspective(const glm::mat4& transform, float fov, float near, float far, float aspect);
		 void UpdatePerspective(const glm::mat4& transform);

		 bool Contains(AABB box);

		 void SetNear(float near) { m_near = near; }
		 float GetNear() { return m_near; }

		 void SetFar(float far) { m_far = far; }
		 float GetFar() { return m_far; }

		 void SetFov(float fov) { m_fov = fov; }
		 float GetFov() { return m_fov; }

		 float SetAspect(float aspect) { m_aspect = aspect; };

	private:

		Window* m_window;

		float m_near;
		float m_far;

		float m_fov;
		float m_aspect; 

		glm::vec3 m_corners[8];
		Plane m_planes[6];
	};
}

#endif