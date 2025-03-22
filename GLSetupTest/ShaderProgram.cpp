#include "ShaderProgram.h"
#include "GraphicsAPI.h"
#include "Platform.h"
#include "Graphics.h"

namespace JLEngine
{
	ShaderProgram::ShaderProgram(string name)
		: GPUResource(name), m_graphics(nullptr), m_programId(-1)
	{
	}

	ShaderProgram::ShaderProgram(string name, string path)
		: GPUResource(name), m_graphics(nullptr), m_filename(path), m_programId(-1)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		UnloadFromGraphics();
	}

	void ShaderProgram::AddShader(Shader& shader)
	{ 
		m_shaders.push_back(shader); 
	}

	Shader& ShaderProgram::GetShader(int type)
	{
		for (auto& s : m_shaders)
		{
			if (s.GetType() == type)
			{
				return s;
			}
		}
		throw std::exception("No shader found");
	}

	void ShaderProgram::GetShader(string name, Shader& shader)
	{
		for (auto s : m_shaders)
		{
			if (s.GetName() == name)
			{
				shader = s;
				break;
			}
		}
	}

	int ShaderProgram::GetUniformLocation(const std::string& name)
	{
		// Retrieve the cached location
		auto it = m_uniformLocations.find(name);
		if (it != m_uniformLocations.end()) 
		{
			return it->second;
		}
		else 
		{
			GLint location = glGetUniformLocation(m_programId, name.c_str());
			if (location == -1)
			{
				//std::cerr << "Warning: Uniform '" << name << "' not found in shader program!" << std::endl;
				return -1;
			}
			m_uniformLocations[name] = location;
			return location;
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::mat4& matrix)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, matrix);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec3& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, vector);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::ivec3& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, vector);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec2& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, vector);
		}
	}

	void ShaderProgram::SetUniformf(const std::string& name, float value) 
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, value);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec4& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, vector);
		}
	}

	void ShaderProgram::SetUniformi(const std::string& name, uint32_t value)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetProgUniform(GetProgramId(), location, value);
		}
	}

	void ShaderProgram::UnloadFromGraphics()
	{
		Graphics::DisposeShader(this);
	}
}