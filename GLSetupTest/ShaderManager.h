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
        ShaderManager(Graphics* graphics);

        std::shared_ptr<ShaderProgram> LoadShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
        std::shared_ptr<ShaderProgram> LoadShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);

        std::shared_ptr<ShaderProgram> BasicUnlitShader();
        std::shared_ptr<ShaderProgram> BasicLitShader();
        std::shared_ptr<ShaderProgram> SolidColorShader();
    private:

        std::shared_ptr<ShaderProgram> m_basicUnlit;
        std::shared_ptr<ShaderProgram> m_basicLit;
        std::shared_ptr<ShaderProgram> m_solidColor;

        Graphics* m_graphics;
	};
}

#endif