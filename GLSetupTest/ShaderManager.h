#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <vector>

#include "ResourceManager.h"
#include "Graphics.h"
#include "ShaderProgram.h"

namespace JLEngine
{
	class Graphics;

	class ShaderManager : public ResourceManager<ShaderProgram>
	{
	public:
        ShaderManager(Graphics* graphics) : m_graphics(graphics) {}

        std::shared_ptr<ShaderProgram> LoadShaderProgram(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
        {
            return Add(name, [&]()
                {
                    auto program = std::make_shared<ShaderProgram>(GenerateHandle(), name, folderPath);
                    
                    Shader vertProgram(GL_VERTEX_SHADER, vert);
                    Shader fragProgram(GL_FRAGMENT_SHADER, frag);
                    program->AddShader(vertProgram);
                    program->AddShader(fragProgram);
                    program->UploadToGPU(m_graphics);

                    return program;
                });
        }    
    private:
        Graphics* m_graphics;
	};
}

#endif