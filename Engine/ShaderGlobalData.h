#ifndef CAMERA_INFO_H
#define CAMERA_INFO_H

#include <glm/glm.hpp>

namespace JLEngine
{
	struct ShaderGlobalData
	{
		glm::mat4 viewMatrix;
		glm::mat4 projMatrix;
		glm::vec4 cameraPos;
		glm::vec4 camDir;		// forward direction camera vector
		glm::vec2 timeInfo;		// t/50, t/20, t/5, t
		glm::vec2 windowSize;	// screen/window size
		int		  frameCount;	// num frames rendered (wraps once it hits int max
	};
}

#endif