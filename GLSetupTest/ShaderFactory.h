#ifndef SHADER_FACTORY_H
#define SHADER_FACTORY_H

#include "ShaderProgram.h"
#include "ResourceManager.h"
#include "GraphicsAPI.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace JLEngine
{
	class ShaderFactory
	{
	public:
		ShaderFactory(ResourceManager<ShaderProgram>* shaderManager, GraphicsAPI* graphics)
			: m_shaderManager(shaderManager), m_graphicsAPI(graphics) {}

        std::shared_ptr<ShaderProgram> CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
        {
            return m_shaderManager->Load(name, [&]()
                {
                    auto program = std::make_unique<ShaderProgram>(name, folderPath);

                    Shader vertProgram(GL_VERTEX_SHADER, vert);
                    Shader fragProgram(GL_FRAGMENT_SHADER, frag);
                    program->AddShader(vertProgram);
                    program->AddShader(fragProgram);
                    program->UploadToGPU(m_graphicsAPI);

                    auto shaderPathVert = program->GetFilePath() + vertProgram.GetName();
                    auto currentTimestamp = std::filesystem::last_write_time(shaderPathVert);
                    m_shaderTimestamps[shaderPathVert] = currentTimestamp;

                    auto shaderPathFrag = program->GetFilePath() + fragProgram.GetName();
                    currentTimestamp = std::filesystem::last_write_time(shaderPathFrag);
                    m_shaderTimestamps[shaderPathFrag] = currentTimestamp;

                    return program;
                });
        }

        std::shared_ptr<ShaderProgram> CreateShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
        {
            return m_shaderManager->Load(name, [&]()
                {
                    auto program = std::make_unique<ShaderProgram>(name);
                    Shader vertProgram(GL_VERTEX_SHADER, "vert");
                    Shader fragProgram(GL_FRAGMENT_SHADER, "frag");
                    program->AddShader(vertProgram, vertSource);
                    program->AddShader(fragProgram, fragSource);
                    program->UploadToGPU(m_graphicsAPI);
                    return program;
                });
        }
	protected:

        std::unordered_map<std::string, std::filesystem::file_time_type> m_shaderTimestamps;
        GraphicsAPI* m_graphicsAPI;
		ResourceManager<ShaderProgram>* m_shaderManager;
	};
}

#endif