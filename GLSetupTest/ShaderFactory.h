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
#include <unordered_set>

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
                    auto program = std::make_shared<ShaderProgram>(name, folderPath);

                    Shader vertProgram(GL_VERTEX_SHADER, vert);
                    Shader fragProgram(GL_FRAGMENT_SHADER, frag);

                    std::string vertShaderFile = PreprocessShaderIncludes(program->GetFilePath() + vertProgram.GetName());
                    //if (!ReadTextFile(program->GetFilePath() + vertProgram.GetName(), vertShaderFile))
                    //{
                    //    throw "Could not find file: " + program->GetFilePath() + vertProgram.GetName(), "Graphics";
                    //}
                    vertProgram.SetSource(vertShaderFile);
                    program->AddShader(vertProgram);

                    std::string fragShaderFile = PreprocessShaderIncludes(program->GetFilePath() + fragProgram.GetName());;
                    //if (!ReadTextFile(program->GetFilePath() + fragProgram.GetName(), fragShaderFile))
                    //{
                    //    throw "Could not find file: " + program->GetFilePath() + fragProgram.GetName(), "Graphics";
                    //}
                    fragProgram.SetSource(fragShaderFile);
                    program->AddShader(fragProgram);

                    Graphics::CreateShader(program.get());

                    auto shaderPathVert = program->GetFilePath() + vertProgram.GetName();
                    auto currentTimestamp = std::filesystem::last_write_time(shaderPathVert);
                    m_shaderTimestamps[shaderPathVert] = currentTimestamp;
                    
                    auto shaderPathFrag = program->GetFilePath() + fragProgram.GetName();
                    currentTimestamp = std::filesystem::last_write_time(shaderPathFrag);
                    m_shaderTimestamps[shaderPathFrag] = currentTimestamp;
                    GL_CHECK_ERROR();
                    return program;
                });
        }

        std::shared_ptr<ShaderProgram> CreateComputeFromFile(const std::string& name, const std::string& computeFile, const std::string& folderPath)
        {
            return m_shaderManager->Load(name, [&]()
                {
                    auto program = std::make_shared<ShaderProgram>(name, folderPath);

                    Shader computeProgram(GL_COMPUTE_SHADER, computeFile);

                    std::string computeShaderText = PreprocessShaderIncludes(program->GetFilePath() + computeProgram.GetName());;
                    //if (!ReadTextFile(program->GetFilePath() + computeProgram.GetName(), computeShaderText))
                    //{
                    //    throw "Could not find file: " + program->GetFilePath() + computeProgram.GetName(), "Graphics";
                    //}
                    computeProgram.SetSource(computeShaderText);
                    program->AddShader(computeProgram);

                    Graphics::CreateShader(program.get());

                    auto shaderPathVert = program->GetFilePath() + computeProgram.GetName();
                    auto currentTimestamp = std::filesystem::last_write_time(shaderPathVert);
                    m_shaderTimestamps[shaderPathVert] = currentTimestamp;

                    GL_CHECK_ERROR();
                    return program;
                });
        }

        std::shared_ptr<ShaderProgram> CreateShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
        {
            return m_shaderManager->Load(name, [&]()
                {
                    auto program = std::make_shared<ShaderProgram>(name);
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

        //static ShaderProgram* CreateShaderFromFile(const std::string& vert, const std::string& frag, std::string folderPath)
        //{
        //    auto program = new ShaderProgram("", "");
        //
        //    Shader vertProgram(GL_VERTEX_SHADER, vert);
        //    Shader fragProgram(GL_FRAGMENT_SHADER, frag);
        //
        //    std::string vertShaderFile = PreprocessShaderIncludes(program->GetFilePath() + vertProgram.GetName());
        //    vertProgram.SetSource(vertShaderFile);
        //    program->AddShader(vertProgram);
        //
        //    std::string fragShaderFile = PreprocessShaderIncludes(program->GetFilePath() + fragProgram.GetName());;
        //    fragProgram.SetSource(fragShaderFile);
        //    program->AddShader(fragProgram);
        //
        //    Graphics::CreateShader(program);
        //    return program;
        //}

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

            auto& shaders = program->GetShaders();

            std::cout << "Reload shader: " << program->GetName() << std::endl;
            std::string vertFile = PreprocessShaderIncludes(program->GetFilePath() + shaders[0].GetName());
            //if (!ReadTextFile(program->GetFilePath() + shaders[0].GetName(), vertFile))
            //{
            //    throw "Could not find file: " + program->GetFilePath() + shaders[0].GetName(), "Graphics";
            //}
            shaders[0].SetSource(vertFile);

            // hack, but if the first shader is not a compute shader, its probably a vertex shader
            // so load the frag shader
            if (shaders[0].GetType() != GL_COMPUTE_SHADER)
            {
                std::string fragFile = PreprocessShaderIncludes(program->GetFilePath() + shaders[1].GetName());
                //if (!ReadTextFile(program->GetFilePath() + shaders[1].GetName(), fragFile))
                //{
                //    throw "Could not find file: " + program->GetFilePath() + shaders[1].GetName(), "Graphics";
                //}
                shaders[1].SetSource(fragFile);
            }

            Graphics::CreateShader(program);     
            GL_CHECK_ERROR();
        }

        static std::string PreprocessShaderIncludes(const std::string& shaderPath)
        {
            std::unordered_set<std::string> includedFiles;
            return ProcessIncludesRecursive(shaderPath, includedFiles);
        }

        static std::string ProcessIncludesRecursive(const std::string& path, std::unordered_set<std::string>& includedFiles)
        {
            if (includedFiles.count(path)) return ""; // avoid recursive inclusion

            includedFiles.insert(path);

            std::string source;
            if (!ReadTextFile(path, source)) 
            {
                throw "Could not read shader file: " + path;
            }

            std::istringstream stream(source);
            std::string line;
            std::string result;

            std::filesystem::path basePath = std::filesystem::path(path).parent_path();

            while (std::getline(stream, line)) 
            {
                if (line.find("#include") == 0) 
                {
                    size_t start = line.find("\"");
                    size_t end = line.find("\"", start + 1);
                    if (start != std::string::npos && end != std::string::npos) 
                    {
                        std::string includeFile = line.substr(start + 1, end - start - 1);
                        std::string includePath = (basePath / includeFile).string();
                        result += ProcessIncludesRecursive(includePath, includedFiles);
                    }
                }
                else 
                {
                    result += line + "\n";
                }
            }

            return result;
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