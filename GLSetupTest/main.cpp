#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "JLEngine.h"
#include "FlyCamera.h"
#include "GltfLoader.h"
#include "Mesh.h"
#include "Node.h"
#include "Geometry.h"
#include "DeferredRenderer.h"

JLEngine::FlyCamera* flyCamera;
JLEngine::Input* input;
JLEngine::Mesh* cubeMesh;
JLEngine::Mesh* planeMesh;
JLEngine::Mesh* sphereMesh;
std::shared_ptr<JLEngine::Node> sceneRoot;
JLEngine::Texture* texture;
JLEngine::ShaderProgram* meshShader;
JLEngine::ShaderProgram* basicLit;
JLEngine::DeferredRenderer* m_defRenderer;
GLFWwindow* window;

bool debugGBuffer = false;

void gameRender(JLEngine::Graphics& graphics, double interpolationFactor)
{
    // Render the scene
    //glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::mat4 view = flyCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    glm::mat4 vp = projection * view; 
    glm::mat4 mvpA = vp * glm::translate(glm::vec3(5.0f, 0.0f, 0.0f));

    //auto shader = basicLit;
    //graphics.BindShader(shader->GetProgramId());
    //shader->SetUniform("uModel", glm::translate(glm::vec3(0.0f, 0.0f, 0.0f)));
    //shader->SetUniform("uView", view);
    //shader->SetUniform("uProjection", projection);
    //shader->SetUniform("uLightPos", glm::vec3(5.0f, 15.0f, 5.0f));
    //shader->SetUniform("uLightColor", glm::vec3(0.8f, 0.8f, 0.8f)); 
    //shader->SetUniformi("uUseTexture", 0);
    //shader->SetUniformi("uTexture", 0);
    //shader->SetUniform("uSolidColor", glm::vec4(1.8f, 0.5f, 0.2f, 1.0f));
    //graphics.RenderMeshWithTexture(sphereMesh, texture);
    //
    //shader->SetUniform("uModel", glm::translate(glm::vec3(-5.0f, 0.0f, 0.0f)));
    //shader->SetUniformi("uTexture", 0);
    //shader->SetUniformi("uUseTexture", 1);
    //graphics.RenderMeshWithTexture(cubeMesh, texture);
    //
    //shader->SetUniform("uModel", glm::translate(glm::vec3(0.0f, -1.0f, 0.0f)));
    //graphics.RenderMeshWithTexture(planeMesh, texture);

    //graphics.RenderNodeHierarchy(duckScene, [shader](JLEngine::Node* node)
    //{
    //    glm::mat4 modelMatrix = glm::translate(glm::vec3(5.0f, 0.0f, 0.0f)) * node->GetGlobalTransform();
    //    shader->SetUniform("uModel", modelMatrix);
    //});
    //
    //graphics.RenderNodeHierarchy(fishScene, [shader](JLEngine::Node* node)
    //{
    //    glm::mat4 modelMatrix = glm::translate(glm::vec3(0.0f, 0.0f, 3.0f)) * node->GetGlobalTransform();
    //    shader->SetUniform("uModel", modelMatrix);
    //});

    m_defRenderer->Render(sceneRoot.get(), flyCamera->GetPosition(), view, projection, debugGBuffer);
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
    if (glfwGetKey(window, GLFW_KEY_ESCAPE))
    {
        glfwSetWindowShouldClose(window, 1);
    }

    if (glfwGetKey(window, GLFW_KEY_G))
    {
        debugGBuffer = !debugGBuffer;
    }
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

void WindowResizeCallback(int width, int height)
{
    m_defRenderer->Resize(width, height);
}

int main(int argc, char* argv[])
{
    std::string assetFolder = argv[1];
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
    graphics->GetWindow()->SetResizeCallback(WindowResizeCallback);

    texture = textureMgr->CreateTextureFromFile("DefaulTexture", assetFolder + "floor_default_grid.png");

    //meshShader = shaderMgr->LoadShaderFromFile("SimpleMeshShader", "simple_mesh_vert.glsl", "simple_mesh_frag.glsl", "../Assets/");
    //meshShader.get()->CacheUniformLocation("uModel");
    //meshShader.get()->CacheUniformLocation("uView");
    //meshShader.get()->CacheUniformLocation("uProjection");
    //meshShader.get()->CacheUniformLocation("uLightPos");
    //meshShader.get()->CacheUniformLocation("uLightColor");
    //meshShader.get()->CacheUniformLocation("uTexture");

    basicLit = shaderMgr->BasicLitShader();

    sceneRoot = std::make_shared<JLEngine::Node>("SceneRoot", JLEngine::NodeTag::SceneRoot);

    //cubeMesh = JLEngine::LoadModelGLB(std::string("../Assets/cube.glb"), graphics);
    planeMesh = JLEngine::LoadModelGLB(std::string(assetFolder + "plane.glb"), graphics);
    auto mat = engine.GetMaterialManager()->CreateMaterial("planeMat");
    mat->baseColorTexture = texture;
    planeMesh->AddMaterial(mat);
    auto planeNode = std::make_shared<JLEngine::Node>("Plane", JLEngine::NodeTag::Mesh);
    planeNode->meshes.push_back(planeMesh);
    auto aduckScene = engine.GetAssetLoader()->LoadGLB(assetFolder + "Duck.glb")[0];
    auto afishScene = engine.GetAssetLoader()->LoadGLB(assetFolder + "BarramundiFish.glb")[0];
    afishScene->translation += glm::vec3(5, 0, 0);
    afishScene->scale = glm::vec3(3.0f, 3.0f, 3.0f);
    cubeMesh = JLEngine::Geometry::GenerateBoxMesh(graphics, "Box1", 2.0f, 2.0f, 2.0f);
    sphereMesh = JLEngine::Geometry::GenerateSphereMesh(graphics, "Sphere1", 1.0f, 15, 15);
    
    auto test = engine.GetAssetLoader()->LoadGLB(assetFolder + "/VirtualCity.glb");
    for (auto item : test)
    {
        sceneRoot->AddChild(item);
    }
    //sceneRoot->AddChild(std::move(aduckScene));
    //sceneRoot->AddChild(std::move(afishScene));
    //sceneRoot->AddChild(planeNode);

    m_defRenderer = new JLEngine::DeferredRenderer(graphics, 
        engine.GetRenderTargetManager(), 
        engine.GetShaderManager(), 
        engine.GetShaderStorageManager(),
        1280, 720, assetFolder);
    m_defRenderer->Initialize();

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    graphics->DumpInfo();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}