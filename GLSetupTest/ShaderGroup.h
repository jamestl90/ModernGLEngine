#ifndef SHADER_GROUP_H
#define SHADER_GROUP_H

#include <vector>

#include "Shader.h"

namespace JLEngine
{
	class ShaderGroup
	{
	public:
		ShaderGroup();

		~ShaderGroup();

		void AddShader(int type, std::string& name);

		std::vector<Shader*>& GetShaders();

		void CleanUp();

	private:

		std::vector<Shader*> m_shaders;
	};
}

#endif