#include "ShaderManager.h"

namespace JLEngine
{
	ShaderManager::ShaderManager(Graphics* graphics) 
		: GraphicsResourceManager<ShaderProgram>(graphics)
	{
	}

	ShaderManager::~ShaderManager()
	{

	}
	uint32 ShaderManager::Add(string vert, string frag, string path)
	{
		string shaderName = vert + frag;
		ShaderProgram* program = GraphicsResourceManager<ShaderProgram>::Add(shaderName, path);

		ShaderGroup group;
		group.AddShader(GL_VERTEX_SHADER, vert);
		group.AddShader(GL_FRAGMENT_SHADER, frag);

		program->SetShaderGroup(group);

		program->Init(m_graphics);

		return program->GetHandle();
	}

	uint32 ShaderManager::Add(ShaderGroup& group, string name, string path)
	{
		ShaderProgram* program = GraphicsResourceManager<ShaderProgram>::Add(name, path);

		program->SetShaderGroup(group);

		program->Init(m_graphics);

		return program->GetHandle();
	}
}