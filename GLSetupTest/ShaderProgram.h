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
	class GraphicsAPI;

	class ShaderProgram : public Resource
	{
	public:
		ShaderProgram(string name);
		ShaderProgram(string name, string path);
		~ShaderProgram();

		void SetProgramId(uint32_t id) { m_programId = id; }
		uint32_t GetProgramId() { return m_programId; }

		void ClearShaders() { m_shaders.clear(); }
		void AddShader(Shader& shader);
		Shader& GetShader(int type);
		void GetShader(string name, Shader& shader);
		std::vector<Shader>& GetShaders() { return m_shaders; }
		const std::string GetFilePath() { return m_filename; }
		int GetUniformLocation(const std::string& name);

		void SetUniform(const std::string& name, const glm::mat4& matrix);
		void SetUniform(const std::string& name, const glm::vec4& vector);
		void SetUniform(const std::string& name, const glm::vec3& vector);
		void SetUniformf(const std::string& name, float value);
		void SetUniformi(const std::string& name, uint32_t value);

		void ClearUniforms() { m_uniformLocations.clear(); }
		void SetActiveUniform(std::string& name, int location) { m_uniformLocations[name] = location; }

		void UnloadFromGraphics();

	private:

		uint32_t m_programId;
		std::string m_filename;
		std::vector<Shader> m_shaders;
		std::unordered_map<std::string, int> m_uniformLocations;

		GraphicsAPI* m_graphics;
	};
}

#endif