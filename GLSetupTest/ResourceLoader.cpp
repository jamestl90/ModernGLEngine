#include "ResourceLoader.h"
#include "Geometry.h"
#include "Node.h"
#include "GLBLoader.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "TextureReader.h"
#include "Material.h"

#include <glm/glm.hpp>

#undef APIENTRY
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace JLEngine
{
    std::unordered_map<std::type_index, std::any> ResourceLoader::m_managers;

    ResourceLoader::ResourceLoader(GraphicsAPI* graphics)
        : m_graphics(graphics), m_hotReload(false)
    {
        m_textureManager = new ResourceManager<Texture>();
        m_shaderManager = new ResourceManager<ShaderProgram>();
        m_materialManager = new ResourceManager<Material>();
        m_renderTargetManager = new ResourceManager<RenderTarget>();
        m_meshManager = new ResourceManager<Mesh>();
        m_cubemapManager = new ResourceManager<Cubemap>();
        m_animManager = new ResourceManager<Animation>();

        m_managers[typeid(Texture)] = m_textureManager;
        m_managers[typeid(ShaderProgram)] = m_shaderManager;
        m_managers[typeid(Mesh)] = m_meshManager;
        m_managers[typeid(Material)] = m_materialManager;
        m_managers[typeid(RenderTarget)] = m_renderTargetManager;
        m_managers[typeid(Cubemap)] = m_cubemapManager;
        m_managers[typeid(Animation)] = m_animManager;

        m_textureFactory = new TextureFactory(m_textureManager, graphics);
        m_cubemapFactory = new CubemapFactory(m_cubemapManager, graphics);
        m_shaderFactory = new ShaderFactory(m_shaderManager, graphics);
        m_materialFactory = new MaterialFactory(m_materialManager);
        m_renderTargetfactory = new RenderTargetFactory(m_renderTargetManager);
        m_meshFactory = new MeshFactory(m_meshManager);
        m_animFactory = new AnimationFactory(m_animManager);

        auto mat = CreateMaterial("DefaultMaterial");
        m_defaultMat = mat.get();
    }

    ResourceLoader::~ResourceLoader() 
    {
        delete m_textureFactory;
        delete m_cubemapFactory;

        delete m_textureManager;
        delete m_shaderManager;
        delete m_materialManager;
        delete m_renderTargetManager;
        delete m_meshManager;
        delete m_cubemapManager;

        if (m_basicLit)
            m_shaderManager->Remove(m_basicLit->GetName());
        if (m_basicUnlit)
            m_shaderManager->Remove(m_basicUnlit->GetName());
        if (m_solidColor)
            m_shaderManager->Remove(m_solidColor->GetName());
        if (m_screenSpaceQuad)
            m_shaderManager->Remove(m_screenSpaceQuad->GetName());
    }

    std::shared_ptr<Node> ResourceLoader::LoadGLB(const std::string& glbFile)
    {
        if (!m_glbLoader)
            m_glbLoader = new GLBLoader(this);

        auto scene = m_glbLoader->LoadGLB(glbFile);
        m_glbLoader->ClearCaches();
        return scene;
    }

    std::shared_ptr<Cubemap> ResourceLoader::CreateCubemapFromFile(const std::string& name, std::array<std::string, 6> fileNames, std::string folderPath)
    {
        return m_cubemapFactory->CreateCubemapFromFiles(name, fileNames, folderPath);
    }

    std::shared_ptr<Texture> ResourceLoader::CreateTexture(const std::string& name, const std::string& filePath, const TexParams& texParams, int outputChannels)
    {
        return m_textureFactory->CreateFromFile(name, filePath, texParams, outputChannels);
    }

    std::shared_ptr<Texture> ResourceLoader::CreateTexture(const std::string& name, const std::string& filePath)
    {
        return m_textureFactory->CreateFromFile(name, filePath);
    }

    std::shared_ptr<Texture> ResourceLoader::CreateTexture(const std::string& name, ImageData& imageData, const TexParams& texParams)
    {
        return m_textureFactory->CreateFromData(name, imageData, texParams);
    }



    std::shared_ptr<Texture> ResourceLoader::CreateTextureEmpty(const std::string& name)
    {
        return m_textureFactory->CreateEmpty(name);
    }

    void ResourceLoader::DeleteTexture(const std::string& name) { m_textureFactory->Delete(name); }

    std::shared_ptr<ShaderProgram> ResourceLoader::CreateComputeFromFile(const std::string& name, const std::string& computeFile, const std::string& folderPath)
    {
        return m_shaderFactory->CreateComputeFromFile(name, computeFile, folderPath);
    }

    std::shared_ptr<ShaderProgram> ResourceLoader::CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
    {
        return m_shaderFactory->CreateShaderFromFile(name, vert, frag, folderPath);
    }

    std::shared_ptr<ShaderProgram> ResourceLoader::CreateShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
    {
        return m_shaderFactory->CreateShaderFromSource(name, vertSource, fragSource);
    }

    void ResourceLoader::DeleteShader(const std::string& name)
    {
        return m_shaderManager->Remove(name);
    }

    void ResourceLoader::PollForChanges(float deltaTime)
    {
        if (!m_hotReload) return;

        m_shaderFactory->PollForChanges(deltaTime);
    }

    ShaderProgram* ResourceLoader::BasicLitShader()
    {
        if (m_basicLit == nullptr)
        {
            std::string vertexShaderCode = R"(
                #version 400 core
                layout(location = 0) in vec3 aPosition;
                layout(location = 1) in vec3 aNormal;
                layout(location = 2) in vec2 aTexCoord;
                uniform mat4 uModel;
                uniform mat4 uView;
                uniform mat4 uProjection;
                out vec3 vFragPos;
                out vec3 vNormal;
                out vec2 vTexCoord;
                void main()
                {
                    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
                    vFragPos = vec3(uModel * vec4(aPosition, 1.0));
                    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
                    vTexCoord = aTexCoord;
                }
            )";

            std::string fragmentShaderCode = R"(
                #version 400 core
                in vec3 vFragPos;
                in vec3 vNormal;
                in vec2 vTexCoord;
                uniform sampler2D uTexture;
                uniform vec3 uLightPos;
                uniform vec3 uLightColor;
                uniform int uUseTexture;
                uniform vec4 uSolidColor;
                out vec4 FragColor;
                void main()
                {
                    vec3 normal = normalize(vNormal);
                    vec3 lightDir = normalize(uLightPos - vFragPos);
                    float diff = max(dot(normal, lightDir), 0.0);
                    vec3 diffuse = diff * uLightColor;
                    vec3 ambient = 0.1 * uLightColor;
                    vec3 lighting = ambient + diffuse;
                    vec4 texColor = texture(uTexture, vTexCoord);
                    if (uUseTexture == 1) {
                        FragColor = vec4(lighting, 1.0) * texColor;
                    }
                    else {
                        FragColor = vec4(lighting, 1.0) * uSolidColor;
                    }       
                }
            )";

            m_basicLit = CreateShaderFromSource("BasicLitShader", vertexShaderCode, fragmentShaderCode).get();
        }
        return m_basicLit;
    }

    ShaderProgram* ResourceLoader::BasicUnlitShader()
    {
        if (m_basicUnlit == nullptr)
        {
            std::string unlitVertexShaderCode = R"(
                #version 400 core
                layout(location = 0) in vec3 aPosition;
                layout(location = 1) in vec3 aNormal; // Included but unused in the unlit shader
                layout(location = 2) in vec2 aTexCoord;
                uniform mat4 uModel;
                uniform mat4 uView;
                uniform mat4 uProjection;
                out vec2 vTexCoord;
                void main()
                {
                    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
                    vTexCoord = aTexCoord;
                }
            )";

            std::string unlitFragmentShaderCode = R"(
                #version 400 core
                in vec2 vTexCoord;
                uniform sampler2D uTexture;
                uniform int uUseTexture;
                uniform vec4 uSolidColor;
                out vec4 FragColor;
                void main()
                {
                    if (uUseTexture == 1) {
                        FragColor = texture(uTexture, vTexCoord);
                    }
                    else {
                        FragColor = vec4(lighting, 1.0) * uSolidColor;
                    }
                }
            )";

            m_basicUnlit = CreateShaderFromSource("BasicUnlitShader", unlitVertexShaderCode, unlitFragmentShaderCode).get();
        }
        return m_basicUnlit;
    }

    ShaderProgram* ResourceLoader::SolidColorShader()
    {
        if (m_solidColor == nullptr)
        {
            std::string solidColorVertexShaderCode = R"(
                #version 400 core
                layout(location = 0) in vec3 aPosition;
                uniform mat4 uModel;
                uniform mat4 uView;
                uniform mat4 uProjection;
                void main()
                {
                    gl_Position = uProjection * uView * uModel * vec4(aPosition, 1.0);
                }
            )";

            std::string solidColorFragmentShaderCode = R"(
                #version 400 core
                uniform vec4 uColor; // The solid color to render
                out vec4 FragColor;
                void main()
                {
                    FragColor = uColor;
                }
            )";

            m_solidColor = CreateShaderFromSource("SolidColorShader", solidColorVertexShaderCode, solidColorFragmentShaderCode).get();
        }
        return m_solidColor;
    }

    ShaderProgram* ResourceLoader::ScreenSpaceQuadShader()
    {
        if (m_screenSpaceQuad == nullptr)
        {
            std::string vertexShaderSource = R"(
            #version 460 core

            layout(location = 0) in vec2 a_Position;
            layout(location = 1) in vec2 a_TexCoords;

            out vec2 v_TexCoords;

            void main() 
            {
                v_TexCoords = a_TexCoords;
                v_TexCoords = vec2(a_TexCoords.x, 1.0 - a_TexCoords.y);
                gl_Position = vec4(a_Position, 0.0, 1.0);
            }
            )";

            std::string fragmentShaderSource = R"(
            #version 460 core

            in vec2 v_TexCoords;

            layout(binding = 0) uniform sampler2D u_Texture;

            out vec4 FragColor;

            void main() 
            {
                vec3 tex = texture(u_Texture, v_TexCoords).rgb;
                FragColor = vec4(tex, 1.0);
            }
            )";

            m_screenSpaceQuad = CreateShaderFromSource("ScreenSpaceQuadShader", vertexShaderSource, fragmentShaderSource).get();
        }
        return m_screenSpaceQuad;
    }

    std::shared_ptr<Material> ResourceLoader::CreateMaterial(const std::string& name)
    {
        return m_materialFactory->CreateMaterial(name);
    }

    std::shared_ptr<RenderTarget> ResourceLoader::CreateRenderTarget(const std::string& name, int width, int height, std::vector<RTParams>& texAttribs, JLEngine::DepthType depthType, uint32_t numSources)
    {
        return m_renderTargetfactory->CreateRenderTarget(name, width, height, texAttribs, depthType, numSources);
    }

    std::shared_ptr<RenderTarget> ResourceLoader::CreateRenderTarget(const std::string& name, int width, int height, RTParams& texAttribs, JLEngine::DepthType depthType, uint32_t numSources)
    {
        std::vector<RTParams> paramsVector;
        paramsVector.push_back(texAttribs);
        return m_renderTargetfactory->CreateRenderTarget(name, width, height, paramsVector, depthType, numSources);
    }

    std::shared_ptr<Mesh> ResourceLoader::CreateMesh(const std::string& name)
    {
        return m_meshFactory->CreateMesh(name);
    }

    std::shared_ptr<Animation> ResourceLoader::CreateAnimation(const std::string& name)
    {
        return m_animFactory->CreateAnimation(name);
    }
}