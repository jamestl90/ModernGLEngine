#ifndef IM3D_MANAGER_H
#define IM3D_MANAGER_H

#include <glm/glm.hpp>

#include <string>

#define IM3D_IMPLEMENTATION
#include "im3d/im3d.h"
#include "Window.h"

namespace JLEngine
{
	class FlyCamera;
	class Input;
	class ResourceLoader;
	class ShaderProgram;
	class UniformBuffer;

	class IM3DManager
	{
	public:
		void Initialise(Window* window, ResourceLoader* resLoader, const std::string& assetFolder);
		void BeginFrame(Input* input, FlyCamera* flyCam, const glm::mat4& proj, float fov, float deltaTime);
		void EndFrameAndRender(const glm::mat4& mvp);
		void Shutdown();

		void DrawBox(const glm::vec3& center, const glm::vec3& halfExtents, const Im3d::Color& color);
		void DrawSphere(const glm::vec3& center, float radius, const Im3d::Color& color);

	private:

		static void DrawCallback(const Im3d::DrawList& drawList);
		void SetupBuffers();
		void LoadShader(const std::string& assetFolder);

		Window* m_window = nullptr;
		GLuint m_vao;
		GLuint m_vbo;
		ShaderProgram* m_shader;

		ResourceLoader* m_resourceLoader;

		glm::mat4 m_mvp;
	};
}

#endif