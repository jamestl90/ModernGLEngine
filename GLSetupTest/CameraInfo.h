#ifndef CAMERA_INFO_H
#define CAMERA_INFO_H

#include <glm/glm.hpp>

namespace JLEngine
{
	struct CameraInfo
	{
		glm::mat4 viewMatrix;
		glm::mat4 projMatrix;
		glm::vec4 cameraPos; // w component unused
		glm::vec4 timeInfo; // t/50, t/20, t/5, t
	};
}

#endif