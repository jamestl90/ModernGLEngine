#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <string>
#include <vector>

#include "Shader.h"
#include "ShaderGroup.h"
#include "GraphicsResource.h"

using std::vector;
using std::string;

namespace JLEngine
{
	class Graphics;

	class ShaderProgram : public GraphicsResource
	{
	public:

		ShaderProgram(uint32 handle, string& name, string& path);
		~ShaderProgram();

		void Init(Graphics* graphics);

		void SetProgramId(uint32 id) { m_programId = id; }
		uint32 GetProgramId() { return m_programId; }

		void SetShaderGroup(ShaderGroup& group) { m_shaderGroup = group; }
		ShaderGroup& GetShaderGroup() { return m_shaderGroup; }

		Shader* GetShader(int type);

		void UnloadFromGraphics();

	private:

		bool m_isBound;

		uint32 m_programId;

		ShaderGroup m_shaderGroup;

		Graphics* m_graphics;
	};
}

#endif