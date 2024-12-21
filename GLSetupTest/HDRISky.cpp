#include "HDRISky.h"
#include "Texture.h"
#include "ShaderProgram.h"
#include "Graphics.h"
#include "Geometry.h"

namespace JLEngine
{
    HDRISky::HDRISky(Texture* enviroMap, ShaderProgram* skyShader)
        : m_enviroCubeMap(enviroMap), m_skyShader(skyShader), m_graphics(nullptr)
    {
        
    }

    HDRISky::~HDRISky() 
    { 
        m_graphics->DisposeVertexBuffer(m_skyboxVBO);
        m_graphics->DisposeShader(m_skyShader);
        m_graphics->DisposeTexture(m_enviroCubeMap);
    }

    void HDRISky::Initialise(Graphics* graphics)
    {
        m_graphics = graphics;

        m_skyboxVBO = Geometry::CreateBox(m_graphics);
    }

    void HDRISky::Render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix) 
    {
        GLint previousDepthFunc;
        glGetIntegerv(GL_DEPTH_FUNC, &previousDepthFunc);

        m_graphics->SetDepthFunc(GL_LEQUAL);
        m_graphics->BindShader(m_skyShader->GetProgramId());
        
        m_skyShader->SetUniform("u_Projection", projMatrix);
        m_skyShader->SetUniform("u_View", glm::mat4(glm::mat3(viewMatrix)));
        m_graphics->SetActiveTexture(0);
        m_graphics->BindTexture(GL_TEXTURE_CUBE_MAP, m_enviroCubeMap->GetGPUID());
        m_skyShader->SetUniformi("u_Skybox", 0);

        m_graphics->BindVertexArray(m_skyboxVBO.GetVAO());
        m_graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);

        m_graphics->BindVertexArray(0);
        m_graphics->SetDepthFunc(previousDepthFunc);
    }
}