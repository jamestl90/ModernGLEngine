#include "DirectionalLightShadowMap.h"
#include "Node.h"
#include "ShaderProgram.h"
#include "Graphics.h"

#include <glad/glad.h> 
#include <imgui.h>

namespace JLEngine
{
    DirectionalLightShadowMap::DirectionalLightShadowMap(ShaderProgram* shaderProg,
        ShaderProgram* shaderSkinning,
        int numCascades,
        float maxShadowDistance)
        : m_shadowMapShader(shaderProg),
        m_shadowMapSkinningShader(shaderSkinning),
        m_shadowFBO(0),
        m_cascadeShadowMapArrayTexture(0),
        m_numCascades(std::max(1, numCascades)),
        m_shadowMapResolution(4096),
        m_maxShadowDistance(maxShadowDistance),
        m_PCFKernelSize(0),
        m_bias(0.00058f)
    {
        m_cascadeLightSpaceMatrices.resize(m_numCascades);
        m_cascadeFarSplitsViewSpace.resize(m_numCascades + 1);
    }

    void DirectionalLightShadowMap::Initialise(int shadowMapResolutionPerCascade)
    {
        m_shadowMapResolution = shadowMapResolutionPerCascade;

        if (m_shadowFBO == 0) Graphics::API()->CreateFrameBuffer(1, &m_shadowFBO);

        if (m_cascadeShadowMapArrayTexture != 0) Graphics::API()->DeleteTexture(1, &m_cascadeShadowMapArrayTexture);

        Graphics::API()->CreateTextures(GL_TEXTURE_2D_ARRAY, 1, &m_cascadeShadowMapArrayTexture);
        Graphics::API()->TextureStorage3D(m_cascadeShadowMapArrayTexture,
            1,
            GL_DEPTH_COMPONENT32F,
            m_shadowMapResolution,
            m_shadowMapResolution,
            m_numCascades);

        Graphics::API()->TextureParameter(m_cascadeShadowMapArrayTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        Graphics::API()->TextureParameter(m_cascadeShadowMapArrayTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        Graphics::API()->TextureParameter(m_cascadeShadowMapArrayTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        Graphics::API()->TextureParameter(m_cascadeShadowMapArrayTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        Graphics::API()->TextureParameter(m_cascadeShadowMapArrayTexture, GL_TEXTURE_BORDER_COLOR, borderColor);

        Graphics::API()->BindFrameBuffer(m_shadowFBO);
        Graphics::API()->NamedFramebufferTextureLayer(
            m_shadowFBO,
            GL_DEPTH_ATTACHMENT,
            m_cascadeShadowMapArrayTexture,
            0, 0);
        Graphics::API()->NamedFramebufferDrawBuffer(m_shadowFBO, GL_NONE);
        Graphics::API()->NamedFramebufferReadBuffer(m_shadowFBO, GL_NONE);
        if (Graphics::API()->CheckNamedFramebufferStatus(m_shadowFBO, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cout << "DirectionalShadowMap Error: Framebuffer not complete." << std::endl;
        }
    }
    
    DirectionalLightShadowMap::~DirectionalLightShadowMap()
    {
        if (m_shadowFBO != 0) Graphics::API()->DeleteFrameBuffer(1, &m_shadowFBO);
        if (m_cascadeShadowMapArrayTexture != 0) Graphics::API()->DeleteTexture(1, &m_cascadeShadowMapArrayTexture);
    }

    void DirectionalLightShadowMap::SetModelMatrix(const glm::mat4& model)
    {
        m_shadowMapShader->SetUniform("u_Model", model);
    }

    void DirectionalLightShadowMap::CalculateCascadeSplits(const glm::mat4& cameraProjectionMatrix, float nearClip)
    {
        m_cascadeFarSplitsViewSpace.resize(m_numCascades + 1);
        
        float clipRange = m_maxShadowDistance - nearClip;
        float minZ = nearClip;
        float maxZ = m_maxShadowDistance;

        m_cascadeFarSplitsViewSpace[0] = minZ;
        float lambda = 0.75f;

        for (int i = 1; i <= m_numCascades; i++)
        {
            float p = (float)i / (float)m_numCascades;
            float logSplit = minZ * glm::pow(maxZ / minZ, p);
            float uniformSplit = minZ + (maxZ - minZ) * p;
            m_cascadeFarSplitsViewSpace[i] = lambda * logSplit + (1.0f - lambda) * uniformSplit;
        }
    }

    std::vector<glm::vec4> DirectionalLightShadowMap::GetFrustumCornersWorldSpace(const glm::mat4& proj, const glm::mat4& view)
    {
        const glm::mat4 inv = glm::inverse(proj * view);
        std::vector<glm::vec4> frustumCorners;
        for (unsigned int x = 0; x < 2; ++x) 
        {
            for (unsigned int y = 0; y < 2; ++y) 
            {
                for (unsigned int z = 0; z < 2; ++z) 
                {
                    const glm::vec4 pt = inv * glm::vec4(
                        2.0f * x - 1.0f,
                        2.0f * y - 1.0f,
                        2.0f * z - 1.0f, 
                        1.0f);
                    frustumCorners.push_back(pt / pt.w);
                }
            }
        }

        // debug
        //for (auto i = 0; i < frustumCorners.size(); i++)
        //{
        //    std::cout << "Corner 1" << std::endl;
        //    std::cout << glm::to_string(frustumCorners[i]) << std::endl;
        //}

        return frustumCorners;
    }

    glm::mat4 DirectionalLightShadowMap::CalculateLightSpaceMatrixForCascade(
        const glm::vec3& lightDir, 
        int cascadeIndex, 
        const glm::mat4& cameraViewMatrix, 
        float fovRad, 
        float aspect)
    {
        float cascadeNear = m_cascadeFarSplitsViewSpace[cascadeIndex];
        float cascadeFar = m_cascadeFarSplitsViewSpace[cascadeIndex + 1];

        float fovYRadians = fovRad; 
        float aspectRatio = aspect;     

        glm::mat4 cascadeCamProj = glm::perspective(fovYRadians, aspectRatio, cascadeNear, cascadeFar);
        std::vector<glm::vec4> cascadeFrustumCorners = GetFrustumCornersWorldSpace(cascadeCamProj, cameraViewMatrix);

        //for (auto i = 0; i < cascadeFrustumCorners.size(); i++)
        //{
        //    std::cout << "corner: " << i << std::endl;
        //    std::cout << glm::to_string(cascadeFrustumCorners[i]) << std::endl;
        //}

        // average corners to get centre 
        glm::vec3 frustumCenterWorld = glm::vec3(0.0f);
        for (const auto& v : cascadeFrustumCorners) 
        {
            frustumCenterWorld += glm::vec3(v);
        }
        frustumCenterWorld /= cascadeFrustumCorners.size();

        float backupDistance = cascadeFar; 
        glm::vec3 lightPosition = frustumCenterWorld + lightDir; 
        glm::mat4 lightViewMatrix = glm::lookAt(lightPosition, frustumCenterWorld, glm::vec3(0.0f, 1.0f, 0.0f));

        if (abs(lightDir.y) > 0.99f) 
        {
            lightViewMatrix = glm::lookAt(lightPosition, frustumCenterWorld, glm::vec3(1.0f, 0.0f, 0.0f));
        }

        // Transform frustum corners to light's view space
        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max(); 
        float maxZ = std::numeric_limits<float>::lowest();

        for (const auto& wc : cascadeFrustumCorners) 
        {
            glm::vec4 tc = lightViewMatrix * wc;
            minX = std::min(minX, tc.x);
            maxX = std::max(maxX, tc.x);
            minY = std::min(minY, tc.y);
            maxY = std::max(maxY, tc.y);
            minZ = std::min(minZ, tc.z);
            maxZ = std::max(maxZ, tc.z);
        }

        //std::cout << "Cascade " << cascadeIndex << std::endl;
        //std::cout << "LightPos: " << glm::to_string(lightPosition) << " target: " << glm::to_string(frustumCenterWorld) <<
        //    " minZ: " << minZ << " maxZ: " << maxZ << std::endl;

        //std::cout << "Cascade: " << cascadeIndex << std::endl;
        //std::cout << "minX: " << minX << " maxX: " << maxX << " minY: " << minY << " maxY: " << maxY <<
        //    " minZ: " << minZ << " maxZ: " << maxZ << std::endl;

        constexpr float zMult = 10.0f;
        if (minZ < 0)
        {
            minZ *= zMult;
        }
        else
        {
            minZ /= zMult;
        }
        if (maxZ < 0)
        {
            maxZ /= zMult;
        }
        else
        {
            maxZ *= zMult;
        }

        const glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);

        return lightProjection * lightViewMatrix;
    }

    void DirectionalLightShadowMap::UpdateCascades(const glm::mat4& cameraViewMatrix,
        const glm::mat4& cameraProjectionMatrix,
        const glm::vec3& lightDirection, float nearClip, float fovRad, float aspect)
    {
        CalculateCascadeSplits(cameraProjectionMatrix, nearClip);

        for (int i = 0; i < m_numCascades; ++i) 
        {
            m_cascadeLightSpaceMatrices[i] = CalculateLightSpaceMatrixForCascade(lightDirection, i, cameraViewMatrix, fovRad, aspect);
            
            //std::cout << "Cascade at " << i << std::endl;
            //std::cout << glm::to_string(m_cascadeLightSpaceMatrices[i]) << std::endl;
        }
    }

    void DirectionalLightShadowMap::BeginShadowMapPassForCascade(int cascadeIndex)
    {
        Graphics::API()->SetViewport(0, 0, m_shadowMapResolution, m_shadowMapResolution);
        Graphics::API()->BindFrameBuffer(m_shadowFBO);
        
        Graphics::API()->NamedFramebufferTextureLayer(m_shadowFBO, GL_DEPTH_ATTACHMENT,
            m_cascadeShadowMapArrayTexture, 0, cascadeIndex);

        Graphics::API()->SetDepthMask(GL_TRUE);
        Graphics::API()->Enable(GL_DEPTH_TEST);
        Graphics::API()->SetDepthFunc(GL_LESS); // GL_LEQUAL?
        Graphics::API()->Clear(GL_DEPTH_BUFFER_BIT);
    }

    void DirectionalLightShadowMap::EndShadowMapPass()
    {
        Graphics::API()->BindFrameBuffer(0);
    }
}