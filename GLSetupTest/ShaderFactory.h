#ifndef SHADER_FACTORY_H
#define SHADER_FACTORY_H

#include "ShaderProgram.h"
#include "ResourceManager.h"
#include "GraphicsAPI.h"

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include "FileHelpers.h"

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

                    std::string shaderFile;
                    if (!ReadTextFile(program->GetFilePath() + vertProgram.GetName(), shaderFile))
                    {
                        throw "Could not find file: " + program->GetFilePath() + vertProgram.GetName(), "Graphics";
                    }
                    vertProgram.SetSource(shaderFile);
                    program->AddShader(vertProgram);

                    if (!ReadTextFile(program->GetFilePath() + fragProgram.GetName(), shaderFile))
                    {
                        throw "Could not find file: " + program->GetFilePath() + fragProgram.GetName(), "Graphics";
                    }
                    fragProgram.SetSource(shaderFile);
                    program->AddShader(fragProgram);

                    Graphics::CreateShader(program.get());

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
                    vertProgram.SetSource(vertSource);
                    fragProgram.SetSource(fragSource);
                    program->AddShader(vertProgram);
                    program->AddShader(fragProgram);
                    Graphics::CreateShader(program.get());
                    return program;
                });
        }

        void PollForChanges(float deltaTime)
        {
            m_accumTime += deltaTime;

            if (m_accumTime > m_pollTimeSeconds)
            {
                //std::cout << "Polling!" << std::endl;
                auto& resources = m_shaderManager->GetResources();
                for (const auto& res : resources)
                {
                    auto shaderProg = res.second.get();

                    if (shaderProg->GetFilePath().empty()) continue;

                    bool needsUpdate = false;
                    for (const auto& shader : shaderProg->GetShaders())
                    {
                        auto shaderPath = shaderProg->GetFilePath() + shader.GetName();
                        auto currentTimestamp = std::filesystem::last_write_time(shaderPath);
                        if (m_shaderTimestamps[shaderPath] != currentTimestamp)
                        {
                            m_shaderTimestamps[shaderPath] = currentTimestamp;
                            needsUpdate = true;
                        }
                    }
                    if (needsUpdate) // if any shader in this program needs an update we will recreate all of them
                    {
                        ReloadFromFile(shaderProg);
                    }
                }
                m_accumTime = 0;
            }
        }

        void ReloadFromFile(ShaderProgram* program)
        {
            Graphics::DisposeShader(program);

            auto shaders = program->GetShaders();
            
            std::string shaderFile;
            if (!ReadTextFile(program->GetFilePath() + shaders[0].GetName(), shaderFile))
            {
                throw "Could not find file: " + program->GetFilePath() + shaders[0].GetName(), "Graphics";
            }
            shaders[0].SetSource(shaderFile);

            if (!ReadTextFile(program->GetFilePath() + shaders[1].GetName(), shaderFile))
            {
                throw "Could not find file: " + program->GetFilePath() + shaders[1].GetName(), "Graphics";
            }
            shaders[1].SetSource(shaderFile);

            Graphics::CreateShader(program);
        }

	protected:

        /* Shader Manager Variables */
        float m_pollTimeSeconds = 1.0;
        float m_accumTime = 0;
        std::unordered_map<std::string, std::filesystem::file_time_type> m_shaderTimestamps;

        GraphicsAPI* m_graphicsAPI;
		ResourceManager<ShaderProgram>* m_shaderManager;
	};
}

#endif