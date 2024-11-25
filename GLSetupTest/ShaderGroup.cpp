#include "ShaderGroup.h"

namespace JLEngine
{
	ShaderGroup::ShaderGroup()
	{
	}

	ShaderGroup::~ShaderGroup()
	{
		m_shaders.clear();
	}

	void ShaderGroup::AddShader(int type, std::string name)
	{
		m_shaders.push_back(new Shader(type, name));
	}

	std::vector<Shader*>& ShaderGroup::GetShaders()
	{
		return m_shaders;
	}

	void ShaderGroup::CleanUp()
	{
		for (uint32 i = 0; i < m_shaders.size(); ++i)
		{
			if (m_shaders[i] != nullptr)
				delete m_shaders[i];
		}
		m_shaders.clear();
	}
}