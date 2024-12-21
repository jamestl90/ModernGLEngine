#include "ShaderProgram.h"
#include "Graphics.h"
#include "Platform.h"

namespace JLEngine
{
	ShaderProgram::ShaderProgram(string name)
		: Resource(name), m_graphics(nullptr), m_programId(-1)
	{
	}

	ShaderProgram::ShaderProgram(string name, string path)
		: Resource(name), m_graphics(nullptr), m_filename(path), m_programId(-1)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		UnloadFromGraphics();
	}

	void ShaderProgram::ReloadFromFile()
	{
		UnloadFromGraphics();
		
		UploadToGPU(m_graphics);
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

			auto activeUniforms = m_graphics->GetActiveUniforms(m_programId);
			for (auto& uniform : activeUniforms)
			{
				auto& name = std::get<0>(uniform);
				auto& loc = std::get<1>(uniform);
				m_uniformLocations[name] = loc;
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

	Shader ShaderProgram::GetShader(int type)
	{
		for (auto s : m_shaders)
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

	void ShaderProgram::SetUniformi(const std::string& name, uint32_t value)
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