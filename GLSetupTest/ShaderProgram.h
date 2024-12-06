#ifndef SHADERPROGRAM_H
#define SHADERPROGRAM_H

#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

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
		ShaderProgram(string name);
		ShaderProgram(string name, string path);
		~ShaderProgram();

		void ReloadFromFile();

		void UploadToGPU(Graphics* graphics);

		void SetProgramId(uint32 id) { m_programId = id; }
		uint32 GetProgramId() { return m_programId; }

		void AddShader(Shader& shader) { m_shaders.push_back(shader); }
		void AddShader(Shader& shader, string source);
		Shader GetShader(int type);
		void GetShader(string name, Shader& shader);
		const std::vector<Shader> GetShaders() { return m_shaders; }

		const std::string GetFilePath() { return m_filename; }

		int GetUniformLocation(const std::string& name);

		void SetUniform(const std::string& name, const glm::mat4& matrix);
		void SetUniform(const std::string& name, const glm::vec4& vector);
		void SetUniform(const std::string& name, const glm::vec3& vector);
		void SetUniformf(const std::string& name, float value);
		void SetUniformi(const std::string& name, uint32 value);

		void UnloadFromGraphics();

	private:

		std::vector<std::string> m_shaderTexts;
		uint32 m_programId;
		std::vector<std::string> m_activeUniforms;
		std::string m_filename;
		std::vector<Shader> m_shaders;
		std::unordered_map<std::string, int> m_uniformLocations;

		Graphics* m_graphics;
	};
}

#endif