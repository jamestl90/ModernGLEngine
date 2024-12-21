#include "GenerateBRDFLUT.h"
#include "JLEngine.h"
#include "GlobalVars.h"

int GenerateBRDFLUT(const std::string& assetFolder)
{
    JLEngine::JLEngineCore engine(SCREEN_WIDTH, SCREEN_HEIGHT, "JL Engine", 60, 120);
    engine.InitIMGUI();
    auto input = engine.GetInput();
    auto window = engine.GetGraphics()->GetWindow()->GetGLFWwindow();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);
    auto graphics = engine.GetGraphics();



    graphics->SetViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    auto renderFunc = [](JLEngine::Graphics& g, double interpolationFac)
    {
            
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

    engine.run(logicUpdate, renderFunc, [](double dt) {});

    return 0;
}