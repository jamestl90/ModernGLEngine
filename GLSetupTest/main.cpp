#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "JLEngine.h"

void gameLogicUpdate(double deltaTime)
{

}

void gameRender()
{
    // Render the scene
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
}

void fixedUpdate(double fixedTimeDelta)
{

}

int main()
{
    JLEngine::JLEngineCore engine(1280, 720, "JL Engine", 60, 120);

    auto graphics = engine.GetGraphics();
    auto shaderMgr = engine.GetShaderManager();
    auto textureMgr = engine.GetTextureManager();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}