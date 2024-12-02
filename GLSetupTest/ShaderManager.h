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

        ShaderProgram* CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
        ShaderProgram* CreateShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);

        ShaderProgram* BasicUnlitShader();
        ShaderProgram* BasicLitShader();
        ShaderProgram* SolidColorShader();
    private:

        ShaderProgram* m_basicUnlit;
        ShaderProgram* m_basicLit;
        ShaderProgram* m_solidColor;

        Graphics* m_graphics;
	};
}

#endif