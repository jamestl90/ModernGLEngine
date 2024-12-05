#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <vector>
#include <unordered_map>

#include "ResourceManager.h"
#include "Graphics.h"
#include "ShaderProgram.h"
#include <filesystem>

namespace JLEngine
{
	class Graphics;

	class ShaderManager : public ResourceManager<ShaderProgram>
	{
	public:
        ShaderManager(Graphics* graphics);

        ShaderProgram* CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath);
        ShaderProgram* CreateShaderFromSource(const std::string& name, const std::string& vert, const std::string& frag);

        void SetHotReloading(bool hotReload) { m_hotReload = hotReload; }
        void PollForChanges(float deltaTime);

        ShaderProgram* BasicUnlitShader();
        ShaderProgram* BasicLitShader();
        ShaderProgram* SolidColorShader();
    private:

        std::unordered_map<std::string, std::filesystem::file_time_type> m_shaderTimestamps;
        bool m_hotReload;
        float m_pollTimeSeconds = 1.0;
        float m_accumTime = 0;

        ShaderProgram* m_basicUnlit;
        ShaderProgram* m_basicLit;
        ShaderProgram* m_solidColor;

        Graphics* m_graphics;
	};
}

#endif