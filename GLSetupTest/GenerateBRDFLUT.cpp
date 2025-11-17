#include "GenerateBRDFLUT.h"
#include "JLEngine.h"
#include "GlobalVars.h"
#include "Geometry.h"
#include "TextureWriter.h"
#include "CubemapBaker.h"
#include <GLFW/glfw3.h>

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

int GenerateBRDFLUT(const std::string& assetFolder)
{
    JLEngine::JLEngineCore engine(SCREEN_WIDTH, SCREEN_HEIGHT, "JL Engine", 60, 120, assetFolder);
    engine.InitIMGUI();
    auto input = engine.GetInput();
    auto window = engine.GetGraphicsAPI()->GetWindow()->GetGLFWwindow();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);
    auto graphics = engine.GetGraphicsAPI();
    auto finalPath = assetFolder + "Core/";

    JLEngine::VertexArrayObject vao;
    JLEngine::Geometry::CreateScreenSpaceQuad(vao);
    JLEngine::Graphics::CreateVertexArray(&vao);
    auto vaoId = vao.GetGPUID();
    auto& ibo = vao.GetIBO();
    auto quadShader = engine.GetResourceLoader()->CreateShaderFromFile("QuadShader", "pos_uv_vert.glsl", "pos_uv_frag.glsl", finalPath + "/Shaders/");

    int brdfLutSize = 512;

    auto baker = new JLEngine::CubemapBaker(assetFolder, engine.GetResourceLoader());
    uint32_t brdfLUTTexture = baker->GenerateBRDFLUT(brdfLutSize, 1024);

    

    JLEngine::ImageData brdfLutData;
    graphics->ReadTexture2D(brdfLUTTexture, brdfLutData, brdfLutSize, brdfLutSize, 3, true, false);
    JLEngine::TextureWriter::WriteTexture(assetFolder + "HDRI/testoutput.hdr", brdfLutData);

    graphics->SetViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    auto renderFunc = [&graphics,&vaoId, &quadShader, &ibo, &brdfLUTTexture](JLEngine::GraphicsAPI& g, double interpolationFac)
    {
            graphics->BindShader(quadShader->GetProgramId());
            graphics->SetActiveTexture(0);
            graphics->BindTexture(GL_TEXTURE_2D, brdfLUTTexture);
            quadShader->SetUniformi("u_Texture", 0);

            graphics->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            graphics->BindVertexArray(vaoId);
            graphics->DrawElementBuffer(GL_TRIANGLES, (uint32_t)ibo.GetDataImmutable().size(), GL_UNSIGNED_INT, nullptr);
    };

    auto mouseMove = [](double a, double b)
    {
            
    };

    auto logicUpdate = [&window](double dt)
    {
            
    };

    auto keyboardCallback = [&window](int a, int b, int c, int d)
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE))
            glfwSetWindowShouldClose(window, 1);
    };

    input->SetMouseMoveCallback(mouseMove);
    input->SetKeyboardCallback(keyboardCallback);

    engine.Run(logicUpdate, renderFunc, [](double dt) {});

    return 0;
}