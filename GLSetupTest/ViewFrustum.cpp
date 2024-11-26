#include "ViewFrustum.h"

namespace JLEngine
{
	ViewFrustum::ViewFrustum(float fov, float aspect, float near, float far)
	{
		m_fov = fov;
		m_near = near;
		m_far = far;
		m_aspect = aspect;
	}

	ViewFrustum::~ViewFrustum()
	{
	}

	void ViewFrustum::UpdatePerspective(const glm::mat4& transform, float fov, float near, float far, float aspect /*= -1.0f*/ )
	{
		m_fov = fov;
		m_near = near;
		m_far = far;

		m_aspect = aspect; // use aspect based on window width/height if set to -1.0f

		UpdatePerspective(transform);
	}

	void ViewFrustum::UpdatePerspective(const glm::mat4& transform)
	{
		float top = m_near * glm::tan(glm::radians(m_fov / 2.0f));
		float right = top * m_aspect;
		float left = -right;
		float bottom = -top;

		float left_far = left * m_far / m_near;
		float right_far = right * m_far / m_near;
		float bottom_far = bottom * m_far / m_near;
		float top_far = top * m_far / m_near;

		m_corners[0] = glm::vec3(left, bottom, -m_near);
		m_corners[1] = glm::vec3(right, bottom, -m_near);
		m_corners[2] = glm::vec3(right, top, -m_near);
		m_corners[3] = glm::vec3(left, top, -m_near);

		m_corners[4] = glm::vec3(left_far, bottom_far, -m_far);
		m_corners[5] = glm::vec3(right_far, bottom_far, -m_far);
		m_corners[6] = glm::vec3(right_far, top_far, -m_far);
		m_corners[7] = glm::vec3(left_far, top_far, -m_far);

		// transform all corners into the camera space (or whatever transform is)
		for (int i = 0; i < 8; ++i)
		{
			glm::vec4 temp = glm::vec4(transform * glm::vec4(m_corners[i], 1.0f));
			m_corners[i] = glm::vec3(temp);
		}

		glm::vec4 tempOrig = transform * glm::vec4(0.0f);
		glm::vec3 origin = glm::vec3(tempOrig);

		m_planes[0] = Plane(origin, m_corners[3], m_corners[0]);		// Left
		m_planes[1] = Plane(origin, m_corners[1], m_corners[2]);		// Right
		m_planes[2] = Plane(origin, m_corners[0], m_corners[1]);		// Bottom
		m_planes[3] = Plane(origin, m_corners[2], m_corners[3]);		// Top
		m_planes[4] = Plane(m_corners[0], m_corners[1], m_corners[2]);	// Near
		m_planes[5] = Plane(m_corners[5], m_corners[4], m_corners[7]);	// Far
	}

	bool ViewFrustum::Contains(AABB box)
	{
		for (int i = 0; i < 6; ++i)
		{
			glm::vec3& norm = m_planes->m_normal;

			glm::vec3 positive = box.min;
			if (norm.x <= 0.0f) positive.x = box.max.x;
			if (norm.y <= 0.0f) positive.y = box.max.y;
			if (norm.z <= 0.0f) positive.z = box.max.z;

			if (m_planes[i].DistanceToPoint(positive) > 0) return true;
		}
		return false;
	}
}