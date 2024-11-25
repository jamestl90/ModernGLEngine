#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "Graphics.h"
#include "ShaderManager.h"

struct OpenGLContextFixture 
{
    GLFWwindow* window;

    OpenGLContextFixture() 
    {
        // Initialize GLFW
        if (!glfwInit()) 
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        // Create a hidden OpenGL context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE); // Hidden window for testing

        window = glfwCreateWindow(1, 1, "Test Context", nullptr, nullptr);
        if (!window) 
        {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        glfwMakeContextCurrent(window);

        // Load OpenGL function pointers with glad
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
        {
            glfwDestroyWindow(window);
            glfwTerminate();
            throw std::runtime_error("Failed to initialize GLAD");
        }
    }

    ~OpenGLContextFixture() 
    {
        // Cleanup
        glfwDestroyWindow(window);
        glfwTerminate();
    }
};

TEST_CASE("ShaderManager loads and initializes a valid shader group", "[ShaderManager]") 
{
    JLEngine::Window window(100, 100, "testWindow", 60, 120);
    JLEngine::Graphics graphics(&window); // Assuming Graphics is properly mockable or stubbed
    JLEngine::ShaderManager shaderManager(&graphics);

    JLEngine::ShaderGroup group;
    std::string vertShader = "vert_shader_test.glsl";
    std::string fragShader = "frag_shader_test.glsl";
    group.AddShader(GL_VERTEX_SHADER, vertShader);
    group.AddShader(GL_FRAGMENT_SHADER, fragShader);

    std::string vertName = "vert_shader_test";
    std::string fragName = "frag_shader_test";
    std::string folderName = "TestFiles/";
    REQUIRE_NOTHROW(shaderManager.Add(group, vertName, folderName));
    REQUIRE(shaderManager.Add(group, fragName, folderName) == 1);
}

TEST_CASE("ShaderManager fails to load missing shaders", "[ShaderManager]") 
{
    JLEngine::Window window(100, 100, "testWindow", 60, 120);
    JLEngine::Graphics graphics(&window); // Assuming Graphics is properly mockable or stubbed
    JLEngine::ShaderManager shaderManager(&graphics);

    JLEngine::ShaderGroup group;
    std::string vertShader = "TestFiles/vert_shader_test.glsl";
    std::string fragShader = "TestFiles/frag_shader_test.glsl";
    group.AddShader(GL_VERTEX_SHADER, vertShader);
    group.AddShader(GL_FRAGMENT_SHADER, fragShader);

    REQUIRE_THROWS_AS(shaderManager.Add(group, "missing_shader", "test_shaders"), std::runtime_error);
}