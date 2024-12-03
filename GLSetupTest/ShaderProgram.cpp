#include "ShaderProgram.h"
#include "Graphics.h"
#include "Platform.h"

namespace JLEngine
{
	ShaderProgram::ShaderProgram(uint32 handle, string name)
		: Resource(handle, name), m_graphics(nullptr), m_programId(-1)
	{
	}

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
			if (m_shaderTexts.size() == 0)
			{
				m_graphics->CreateShaderFromFile(this);
			}
			else
			{				
				m_graphics->CreateShaderFromText(this, m_shaderTexts);
				m_shaderTexts.clear();
			}

		}
		catch (const std::exception& ex)
		{
			if (!Platform::AlertBox(std::string(ex.what()), "Shader Error"))
			{
				std::cout << ex.what() << std::endl;
			}
		}
	}

	void ShaderProgram::AddShader(Shader& shader, string source) 
	{ 
		m_shaderTexts.push_back(source);
		m_shaders.push_back(shader); 
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
		if (location == -1) 
		{
			std::cerr << "Warning: Uniform '" << name << "' not found in shader program!" << std::endl;
		}
		m_uniformLocations[name] = location;
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
				std::cerr << "Warning: Uniform '" << name << "' not found in shader program!" << std::endl;
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
			m_graphics->SetUniform(location, 1, false, matrix);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec3& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(location, vector);
		}
	}

	void ShaderProgram::SetUniformf(const std::string& name, float value) 
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(location, value);
		}
	}

	void ShaderProgram::SetUniform(const std::string& name, const glm::vec4& vector)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(location, vector);
		}
	}

	void ShaderProgram::SetUniformi(const std::string& name, uint32 value)
	{
		GLint location = GetUniformLocation(name);
		if (location != -1)
		{
			m_graphics->SetUniform(location, value);
		}
	}

	void ShaderProgram::UnloadFromGraphics()
	{
		m_graphics->DisposeShader(this);
	}
}