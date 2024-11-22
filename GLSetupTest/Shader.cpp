#include "Shader.h"

namespace JLEngine
{
	Shader::Shader(int type, std::string name)
	{
		m_shaderId = -1;
		m_type = type;
		m_name = name; 
	}
}
