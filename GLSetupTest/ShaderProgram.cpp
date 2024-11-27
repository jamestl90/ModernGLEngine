#include "ShaderProgram.h"
#include "Graphics.h"
#include "Platform.h"

namespace JLEngine
{
	ShaderProgram::ShaderProgram(uint32 handle, string name, string path) 
		: Resource(handle, name), m_graphics(nullptr), m_filename(path), m_programId(-1)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		UnloadFromGraphics();
	}

	void ShaderProgram::UploadToGPU(Graphics* graphics)
	{
		m_graphics = graphics;

		try
		{
			m_graphics->CreateShader(this);
		}
		catch (const std::exception& ex)
		{
			if (!Platform::AlertBox(std::string(ex.what()), "Shader Error"))
			{
				std::cout << ex.what() << std::endl;
			}
		}
	}

	void ShaderProgram::GetShader(int type, Shader& shader)
	{
		for (auto s : m_shaders)
		{
			if (s.GetType() == type)
			{
				shader = s;
				break;
			}
		}
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

	void ShaderProgram::CacheUniformLocation(const std::string& name)
	{
		GLint location = glGetUniformLocation(m_programId, name.c_str());
		if (location == -1) {
			std::cerr << "Warning: Uniform '" << name << "' not found in shader program!" << std::endl;
		}
		m_uniformLocations[name] = location;
	}

	int ShaderProgram::GetUniformLocation(const std::string& name) const
	{
		// Retrieve the cached location
		auto it = m_uniformLocations.find(name);
		if (it != m_uniformLocations.end()) {
			return it->second;
		}
		else {
			std::cerr << "Error: Uniform '" << name << "' not cached!" << std::endl;
			return -1;
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::mat4& matrix) const
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(m_programId, name, 1, false, matrix);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec3& vector) const
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(m_programId, name, vector);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, uint32 value) const
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(m_programId, name, value);
		}
	}

	void ShaderProgram::UnloadFromGraphics()
	{
		m_graphics->DisposeShader(this);
	}
}