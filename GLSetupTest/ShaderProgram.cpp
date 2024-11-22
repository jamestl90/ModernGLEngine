#include "ShaderProgram.h"
#include "Graphics.h"
#include "Platform.h"

namespace JLEngine
{
	ShaderProgram::ShaderProgram(uint32 handle, string& name, string& path) 
		: GraphicsResource(handle, name, path)
	{
	}

	ShaderProgram::~ShaderProgram()
	{
		UnloadFromGraphics();
		m_shaderGroup.CleanUp();
	}

	void ShaderProgram::Init(Graphics* graphics)
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

	Shader* ShaderProgram::GetShader(int type)
	{
		for (int i = 0; i < (signed)m_shaderGroup.GetShaders().size(); i++)
		{
			if (m_shaderGroup.GetShaders()[i]->GetType() == type)
			{
				return m_shaderGroup.GetShaders()[i];
			}
		}
		return nullptr;
	}

	void ShaderProgram::UnloadFromGraphics()
	{
		m_graphics->DisposeShader(this);
	}
}