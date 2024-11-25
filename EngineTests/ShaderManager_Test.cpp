#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include "Graphics.h"
#include "ShaderManager.h"

TEST_CASE("ShaderManager loads and initializes a valid shader program", "[ShaderManager]") 
{
    JLEngine::Window window(100, 100, "testWindow", 60, 120);
    JLEngine::Graphics graphics(&window);
    JLEngine::ShaderManager shaderManager(&graphics);

    std::string vertShader = "vert_shader_test.glsl";
    std::string fragShader = "frag_shader_test.glsl";

    auto program = shaderManager.LoadShaderProgram("testShader", vertShader, fragShader, "TestFiles/");
    REQUIRE(program->GetProgramId() > 0);
}
