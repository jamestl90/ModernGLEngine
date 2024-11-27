#include "ShaderManager.h"

namespace JLEngine
{
    ShaderManager::ShaderManager(Graphics* graphics) : m_graphics(graphics), m_basicLit(nullptr), m_basicUnlit(nullptr), m_solidColor(nullptr)
    {

    }

    std::shared_ptr<ShaderProgram> ShaderManager::LoadShaderFromFile(const std::string& name, const std::string& vert, const std::string& frag, std::string folderPath)
    {
        return Add(name, [&]()
            {
                auto program = std::make_shared<ShaderProgram>(GenerateHandle(), name, folderPath);

                Shader vertProgram(GL_VERTEX_SHADER, vert);
                Shader fragProgram(GL_FRAGMENT_SHADER, frag);
                program->AddShader(vertProgram);
                program->AddShader(fragProgram);
                program->UploadToGPU(m_graphics);

                return program;
            });
    }

    std::shared_ptr<ShaderProgram> ShaderManager::LoadShaderFromSource(const std::string& name, const std::string& vertSource, const std::string& fragSource)
    {
        return Add(name, [&]()
            {
                auto program = std::make_shared<ShaderProgram>(GenerateHandle(), name);

                Shader vertProgram(GL_VERTEX_SHADER, "vert");
                Shader fragProgram(GL_FRAGMENT_SHADER, "frag");
                program->AddShader(vertProgram, vertSource);
                program->AddShader(fragProgram, fragSource);
                program->UploadToGPU(m_graphics);

                return program;
            });
    }

    std::shared_ptr<ShaderProgram> ShaderManager::BasicLitShader()
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
                    FragColor = vec4(lighting, 1.0) * texColor;
                }
            )";

            m_basicLit = LoadShaderFromSource("BasicLitShader", vertexShaderCode, fragmentShaderCode);
            m_basicLit.get()->CacheUniformLocation("uModel");
            m_basicLit.get()->CacheUniformLocation("uView");
            m_basicLit.get()->CacheUniformLocation("uProjection");
            m_basicLit.get()->CacheUniformLocation("uTexture");
            m_basicLit.get()->CacheUniformLocation("uLightPos");
            m_basicLit.get()->CacheUniformLocation("uLightColor");
            m_basicLit.get()->CacheUniformLocation("uTexture");
        }
        return m_basicLit;
    }

    std::shared_ptr<ShaderProgram> ShaderManager::BasicUnlitShader()
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
                out vec4 FragColor;
                void main()
                {
                    FragColor = texture(uTexture, vTexCoord);
                }
            )";

            m_basicUnlit = LoadShaderFromSource("BasicUnlitShader", unlitVertexShaderCode, unlitFragmentShaderCode);
            m_basicUnlit.get()->CacheUniformLocation("uModel");
            m_basicUnlit.get()->CacheUniformLocation("uView");
            m_basicUnlit.get()->CacheUniformLocation("uProjection");
            m_basicUnlit.get()->CacheUniformLocation("uTexture");
        }
        return m_basicUnlit;
    }

    std::shared_ptr<ShaderProgram> ShaderManager::SolidColorShader()
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

            m_solidColor = LoadShaderFromSource("SolidColorShader", solidColorVertexShaderCode, solidColorFragmentShaderCode);
            m_solidColor.get()->CacheUniformLocation("uModel");
            m_solidColor.get()->CacheUniformLocation("uView");
            m_solidColor.get()->CacheUniformLocation("uProjection");
            m_solidColor.get()->CacheUniformLocation("uColor");
        }
        return m_solidColor;
    }
}