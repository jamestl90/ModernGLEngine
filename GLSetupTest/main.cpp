#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "JLEngine.h"
#include "FlyCamera.h"
#include "ModelLoader.h"
#include "Mesh.h"

JLEngine::FlyCamera* flyCamera;
JLEngine::Input* input;
JLEngine::Mesh* mesh;
JLEngine::Texture* texture;
std::shared_ptr<JLEngine::ShaderProgram> meshShader;
std::shared_ptr<JLEngine::ShaderProgram> basicLit;
GLFWwindow* window;

void gameRender(JLEngine::Graphics& graphics)
{
    // Render the scene
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = flyCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    glm::mat4 vp = projection * view; 
    glm::mat4 mvpA = vp * glm::mat4(1.0f);
    glm::mat4 mvpB = vp * glm::translate(glm::vec3(5.0f, 0.0f, 0.0f));

    uint32 shaderId = -1; // Use the default shader

    graphics.BeginPrimitiveDraw();
    graphics.RenderPrimitive(mvpA, JLEngine::PrimitiveType::Cone, shaderId);
    graphics.RenderPrimitive(mvpB, JLEngine::PrimitiveType::Octahedron, shaderId);
    graphics.EndPrimitiveDraw();

    auto shader = basicLit.get();
    graphics.BindShader(shader->GetProgramId());
    shader->SetUniform("uModel", glm::translate(glm::vec3(-10.0f, 0.0f, 0.0f)));
    shader->SetUniform("uView", view);
    shader->SetUniform("uProjection", projection);
    shader->SetUniform("uLightPos", glm::vec3(5.0f, 15.0f, 5.0f));
    shader->SetUniform("uLightColor", glm::vec3(0.8f, 0.8f, 0.8f));
    shader->SetUniform("uTexture", 0); // Texture unit

    graphics.RenderMeshWithTexture(mesh, texture);
}

void gameLogicUpdate(double deltaTime)
{
    flyCamera->ProcessKeyboard(window, (float)deltaTime);
}

void fixedUpdate(double fixedTimeDelta)
{

}

void KeyboardCallback(int key, int scancode, int action, int mods)
{
    //std::cout << key << " " << scancode << " " << action << " " << mods << std::endl;
}

void MouseCallback(int button, int action, int mods)
{
    //std::cout << button << " " << action << " " << mods << std::endl;
}

void MouseMoveCallback(double x, double y)
{
    //std::cout << x << " " << y << std::endl;
    double deltaX, deltaY;
    input->GetMouseDelta(deltaX, deltaY);

    flyCamera->ProcessMouseMovement(static_cast<float>(deltaX), static_cast<float>(deltaY));
}

int main()
{
    JLEngine::JLEngineCore engine(1280, 720, "JL Engine", 60, 120);

    auto graphics = engine.GetGraphics();
    window = graphics->GetWindow()->GetGLFWwindow();
    auto shaderMgr = engine.GetShaderManager();
    auto textureMgr = engine.GetTextureManager();
    input = engine.GetInput();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);

    input->SetKeyboardCallback(KeyboardCallback);
    input->SetMouseCallback(MouseCallback);
    input->SetMouseMoveCallback(MouseMoveCallback);

    texture = textureMgr->LoadTextureFromFile("DefaulTexture", "../Assets/floor_default_grid.png").get();

    //meshShader = shaderMgr->LoadShaderFromFile("SimpleMeshShader", "simple_mesh_vert.glsl", "simple_mesh_frag.glsl", "../Assets/");
    //meshShader.get()->CacheUniformLocation("uModel");
    //meshShader.get()->CacheUniformLocation("uView");
    //meshShader.get()->CacheUniformLocation("uProjection");
    //meshShader.get()->CacheUniformLocation("uLightPos");
    //meshShader.get()->CacheUniformLocation("uLightColor");
    //meshShader.get()->CacheUniformLocation("uTexture");

    basicLit = shaderMgr->BasicLitShader();

    mesh = JLEngine::LoadModel(std::string("../Assets/cube.glb"), graphics);

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    graphics->DumpInfo();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}