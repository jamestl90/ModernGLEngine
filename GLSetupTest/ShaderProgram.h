#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <string>
#include <vector>

#include "Shader.h"
#include "Resource.h"

using std::vector;
using std::string;

namespace JLEngine
{
	class Graphics;

	class ShaderProgram : public Resource
	{
	public:

		ShaderProgram(uint32 handle, string name, string path);
		~ShaderProgram();

		void UploadToGPU(Graphics* graphics);

		void SetProgramId(uint32 id) { m_programId = id; }
		uint32 GetProgramId() { return m_programId; }

		void AddShader(Shader& shader) { m_shaders.push_back(shader); }
		void GetShader(int type, Shader& shader);
		void GetShader(string name, Shader& shader);
		const std::vector<Shader> GetShaders() { return m_shaders; }

		const std::string GetFilePath() { return m_filename; }

		void UnloadFromGraphics();

	private:

		uint32 m_programId;
		std::string m_filename;
		std::vector<Shader> m_shaders;

		Graphics* m_graphics;
	};
}

#endif