#include "AssetLoader.h"
#include "Geometry.h"
#include "Node.h"
#include "GLBLoader.h"
#include "RenderTarget.h"
#include "ShaderProgram.h"
#include "Node.h"
#include "TextureReader.h"
#include "Material.h"
#include "ShaderStorageBuffer.h"

#include <glm/glm.hpp>

#undef APIENTRY
#define TINYGLTF_IMPLEMENTATION
#include <tiny_gltf.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace JLEngine
{
    AssetLoader::AssetLoader(Graphics* graphics, ResourceManager<ShaderProgram>* shaderManager, ResourceManager<Mesh>* meshManager,
        ResourceManager<Material>* materialManager, ResourceManager<Texture>* textureManager, ResourceManager<RenderTarget>* rtManager,
        ResourceManager<ShaderStorageBuffer>* ssboManager)
        : m_shaderManager(shaderManager), m_meshManager(meshManager), m_materialManager(materialManager),
        m_textureManager(textureManager), m_graphics(graphics), m_renderTargetManager(rtManager), m_shaderStorageManager(ssboManager)
    {
        m_defaultMat = CreateMaterial("DefaultMaterial");
    }
    std::vector<std::shared_ptr<Node>> AssetLoader::LoadGLB(const std::string& glbFile)
    {
        auto glbLoader = new GLBLoader();
        return std::vector<std::shared_ptr<Node>>();
    }

    Texture* AssetLoader::CreateTextureFromFile(const std::string& name, const std::string& filename, bool clamped, bool mipmaps)
    {
        return m_textureManager->Add(name, [&]()
            {
                TextureReader reader;
                std::vector<unsigned char> data;
                int width, height, channels;

                if (!reader.ReadTexture(filename, data, width, height, channels))
                {
                    std::cerr << "Failed to load texture: " << filename << std::endl;
                    return std::unique_ptr<Texture>(nullptr);
                }

                // Create a new Texture object
                auto texture = std::make_unique<Texture>(m_textureManager->GenerateHandle(), name, width, height, data.data(), channels);
                texture->InitFromData(data, width, height, channels, clamped, mipmaps);
                texture->UploadToGPU(m_graphics, true);
                return texture;
            });
    }

    Texture* AssetLoader::CreateTextureFromData(const std::string& name, uint32 width, uint32 height, int channels, void* data,
        int internalFormat, int format, int dataType, bool clamped, bool mipmaps)
    {
        return m_textureManager->Add(name, [&]()
            {
                auto texture = std::make_unique<Texture>(m_textureManager->GenerateHandle(), name, width, height, data, channels);
                texture->SetFormat(internalFormat, format, dataType);
                texture->SetClamped(clamped);
                texture->EnableMipmaps(mipmaps);
                texture->UploadToGPU(m_graphics, false);
                return texture;
            });
    }

    Texture* AssetLoader::CreateTextureFromData(const std::string& name, uint32 width, uint32 height, int channels, vector<unsigned char>& data, bool clamped, bool mipmaps)
    {
        return m_textureManager->Add(name, [&]()
            {
                auto texture = std::make_unique<Texture>(m_textureManager->GenerateHandle(), name, width, height, data.data(), channels);
                texture->InitFromData(data, width, height, channels, clamped, mipmaps);
                texture->UploadToGPU(m_graphics, false);
                return texture;
            });
    }

    ShaderProgram* AssetLoader::CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
    {
        return m_shaderManager->Add(name, [&]()
            {
                auto program = std::make_unique<ShaderProgram>(m_shaderManager->GenerateHandle(), name, folderPath);

                Shader vertProgram(GL_VERTEX_SHADER, vert);
                Shader fragProgram(GL_FRAGMENT_SHADER, frag);
                program->AddShader(vertProgram);
                program->AddShader(fragProgram);
                program->UploadToGPU(m_graphics);

                auto shaderPathVert = program->GetFilePath() + vertProgram.GetName();
                auto currentTimestamp = std::filesystem::last_write_time(shaderPathVert);
                m_shaderTimestamps[shaderPathVert] = currentTimestamp;

                auto shaderPathFrag = program->GetFilePath() + fragProgram.GetName();
                currentTimestamp = std::filesystem::last_write_time(shaderPathFrag);
                m_shaderTimestamps[shaderPathFrag] = currentTimestamp;

                return program;
            });
    }

    ShaderProgram* AssetLoader::CreateShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
    {
        return m_shaderManager->Add(name, [&]()
            {
                auto program = std::make_unique<ShaderProgram>(m_shaderManager->GenerateHandle(), name);

                Shader vertProgram(GL_VERTEX_SHADER, "vert");
                Shader fragProgram(GL_FRAGMENT_SHADER, "frag");
                program->AddShader(vertProgram, vertSource);
                program->AddShader(fragProgram, fragSource);
                program->UploadToGPU(m_graphics);

                return program;
            });
    }

    ShaderProgram* AssetLoader::BasicLitShader()
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

            m_basicLit = CreateShaderFromSource("BasicLitShader", vertexShaderCode, fragmentShaderCode);
        }
        return m_basicLit;
    }

    ShaderProgram* AssetLoader::BasicUnlitShader()
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

            m_basicUnlit = CreateShaderFromSource("BasicUnlitShader", unlitVertexShaderCode, unlitFragmentShaderCode);
        }
        return m_basicUnlit;
    }

    ShaderProgram* AssetLoader::SolidColorShader()
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

            m_solidColor = CreateShaderFromSource("SolidColorShader", solidColorVertexShaderCode, solidColorFragmentShaderCode);
        }
        return m_solidColor;
    }

    Material* AssetLoader::CreateMaterial(const std::string& name)
    {
        return m_materialManager->Add(name, [&]()
            {
                auto mat = std::make_unique<Material>(m_materialManager->GenerateHandle(), name);

                return mat;
            });
    }

    void AssetLoader::PollForChanges(float deltaTime)
    {
        if (!m_hotReload) return;

        m_accumTime += deltaTime;

        if (m_accumTime > m_pollTimeSeconds)
        {
            //std::cout << "Polling!" << std::endl;
            auto& resources = m_shaderManager->GetResources();
            for (const auto& res : resources)
            {
                auto shaderProg = res.second.get();

                if (shaderProg->GetFilePath().empty()) continue;

                bool needsUpdate = false;
                for (const auto& shader : shaderProg->GetShaders())
                {
                    auto shaderPath = shaderProg->GetFilePath() + shader.GetName();
                    auto currentTimestamp = std::filesystem::last_write_time(shaderPath);
                    if (m_shaderTimestamps[shaderPath] != currentTimestamp)
                    {
                        m_shaderTimestamps[shaderPath] = currentTimestamp;
                        needsUpdate = true;
                    }
                }
                if (needsUpdate) // if any shader in this program needs an update we will recreate all of them
                {
                    shaderProg->ReloadFromFile();
                }
            }
            m_accumTime = 0;
        }
    }

    RenderTarget* AssetLoader::CreateRenderTarget(const std::string& name, int width, int height, TextureAttribute& texAttrib, JLEngine::DepthType depthType, uint32 numSources)
    {
        return m_renderTargetManager->Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(m_renderTargetManager->GenerateHandle(), name, numSources);
                renderTarget->SetDepthType(depthType);
                renderTarget->SetWidth(width);
                renderTarget->SetHeight(height);
                renderTarget->SetTextureAttribute(0, texAttrib);
                renderTarget->UploadToGPU(m_graphics);
                return renderTarget;
            });
    }
    
    RenderTarget* AssetLoader::CreateRenderTarget(const std::string& name, int width, int height, SmallArray<TextureAttribute>& texAttribs, JLEngine::DepthType depthType, uint32 numSources)
    {
        return m_renderTargetManager->Add(name, [&]()
            {
                auto renderTarget = std::make_unique<RenderTarget>(m_renderTargetManager->GenerateHandle(), name, numSources);
                renderTarget->SetDepthType(depthType);
                renderTarget->SetWidth(width);
                renderTarget->SetHeight(height);

                for (uint32 i = 0; i < numSources; i++)
                {
                    auto& attrib = texAttribs[i];
                    renderTarget->SetTextureAttribute(i, attrib);
                }
                renderTarget->UploadToGPU(m_graphics);
                return renderTarget;
            });
    }

    Mesh* JLEngine::AssetLoader::LoadMeshFromData(const std::string& name, VertexBuffer& vbo, IndexBuffer& ibo)
    {
        return m_meshManager->Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(m_meshManager->GenerateHandle(), name);
                mesh->SetVertexBuffer(vbo);
                mesh->AddIndexBuffer(ibo);
                mesh->UploadToGPU(m_graphics, true);
                return mesh;
            });
    }

    Mesh* AssetLoader::LoadMeshFromData(const std::string& name, VertexBuffer& vbo)
    {
        return m_meshManager->Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(m_meshManager->GenerateHandle(), name);
                mesh->SetVertexBuffer(vbo);
                mesh->UploadToGPU(m_graphics, true);
                return mesh;
            });
    }

    Mesh* AssetLoader::LoadMeshFromData(const std::string& name, VertexBuffer& vbo, std::vector<IndexBuffer>& ibos)
    {
        return m_meshManager->Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(m_meshManager->GenerateHandle(), name);
                mesh->SetVertexBuffer(vbo);
                for (auto& ibo : ibos)
                {
                    mesh->AddIndexBuffer(ibo);
                }
                mesh->UploadToGPU(m_graphics, true);
                return mesh;
            });
    }

    Mesh* AssetLoader::CreateEmptyMesh(const std::string& name)
    {
        return m_meshManager->Add(name, [&]()
            {
                auto mesh = std::make_unique<Mesh>(m_meshManager->GenerateHandle(), name);
                return mesh;
            });
    }

    ShaderStorageBuffer* AssetLoader::CreateSSBO(const std::string& name, size_t size)
    {
        return m_shaderStorageManager->Add(name, [&]()
            {
                auto ssbo = std::make_unique<ShaderStorageBuffer>(m_shaderStorageManager->GenerateHandle(), name, size, m_graphics);
                ssbo->Initialize(); // Custom initialization for the SSBO
                return ssbo;
            });
    }

    void AssetLoader::UpdateSSBO(const std::string& name, const void* data, size_t size)
    {
        ShaderStorageBuffer* ssbo = m_shaderStorageManager->Get(name);
        if (ssbo)
        {
            ssbo->UpdateData(data, size); // Assuming ShaderStorageBuffer has an UpdateData method
        }
    }
}