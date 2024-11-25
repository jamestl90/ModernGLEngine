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

	void ShaderProgram::UnloadFromGraphics()
	{
		m_graphics->DisposeShader(this);
	}
}