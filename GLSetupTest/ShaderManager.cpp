#include "ShaderManager.h"

namespace JLEngine
{
    ShaderManager::ShaderManager(Graphics* graphics) 
        : m_graphics(graphics), m_basicLit(nullptr), m_basicUnlit(nullptr), 
        m_solidColor(nullptr), m_hotReload(false), m_pollTimeSeconds(1.0f), m_accumTime(0.0f)
    {

    }

    ShaderProgram* ShaderManager::CreateShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
    {
        return Add(name, [&]()
            {
                auto program = std::make_unique<ShaderProgram>(GenerateHandle(), name, folderPath);

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

    ShaderProgram* ShaderManager::CreateShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
    {
        return Add(name, [&]()
            {
                auto program = std::make_unique<ShaderProgram>(GenerateHandle(), name);

                Shader vertProgram(GL_VERTEX_SHADER, "vert");
                Shader fragProgram(GL_FRAGMENT_SHADER, "frag");
                program->AddShader(vertProgram, vertSource);
                program->AddShader(fragProgram, fragSource);
                program->UploadToGPU(m_graphics);

                return program;
            });
    }

    ShaderProgram* ShaderManager::BasicLitShader()
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

    ShaderProgram* ShaderManager::BasicUnlitShader()
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

    ShaderProgram* ShaderManager::SolidColorShader()
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


    void ShaderManager::PollForChanges(float deltaTime)
    {
        if (!m_hotReload) return;

        m_accumTime += deltaTime;

        if (m_accumTime > m_pollTimeSeconds)
        {
            //std::cout << "Polling!" << std::endl;
            auto& resources = GetResources();
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
                    Shader vertShader = shaderProg->GetShader(GL_VERTEX_SHADER);
                    Shader fragShader = shaderProg->GetShader(GL_FRAGMENT_SHADER);

                    //auto newProgram = CreateShaderFromFile()
                }
            }
            m_accumTime = 0;
        }
    }
}