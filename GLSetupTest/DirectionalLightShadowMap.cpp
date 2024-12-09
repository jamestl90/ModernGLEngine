#include "DirectionalLightShadowMap.h"
#include "Node.h"
#include "ShaderProgram.h"
#include "Graphics.h"

#include <glad/glad.h> 

namespace JLEngine
{
    DirectionalLightShadowMap::DirectionalLightShadowMap(Graphics* graphics, ShaderProgram* shaderProg) 
        : m_shadowMapShader(shaderProg), m_graphics(graphics)
    {
        
    }

    void DirectionalLightShadowMap::Initialise()
    {
        glGenFramebuffers(1, &m_shadowFBO);

        // Create depth texture
        glGenTextures(1, &m_shadowMap);
        glBindTexture(GL_TEXTURE_2D, m_shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, m_shadowMapSize, m_shadowMapSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach depth texture to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void DirectionalLightShadowMap::ShadowMapPassSetup(const glm::mat4& lightSpaceMatrix)
    {
        glViewport(0, 0, m_shadowMapSize, m_shadowMapSize);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFBO);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        m_graphics->BindShader(m_shadowMapShader->GetProgramId());
        m_shadowMapShader->SetUniform("u_LightSpaceMatrix", lightSpaceMatrix);
    }

    void DirectionalLightShadowMap::SetModelMatrix(glm::mat4& model)
    {
        m_shadowMapShader->SetUniform("u_Model", model);
    }
}