#ifndef HDRI_SKY_H
#define HDRI_SKY_H

#include <glm/glm.hpp>

#include "VertexBuffers.h"

namespace JLEngine
{
	class Texture;
	class ShaderProgram;
	class Graphics;
	class VertexBuffer;

	class HDRISky
	{
	public:
		HDRISky(Texture* enviroMap, ShaderProgram* skyShader); 
		~HDRISky();

		void Initialise(Graphics* graphics);

		void Render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix);

	protected:

		Graphics* m_graphics;
        VertexBuffer m_skyboxVBO;
		Texture* m_enviroCubeMap;
		ShaderProgram* m_skyShader;
	};
}

#endif