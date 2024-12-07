#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "JLEngine.h"
#include "FlyCamera.h"
#include "Mesh.h"
#include "Node.h"
#include "Geometry.h"
#include "DeferredRenderer.h"
#include "Material.h"

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

int width = 1920;
int height = 1080;
bool debugGBuffer = false;

void gameRender(JLEngine::Graphics& graphics, double interpolationFactor)
{

    float aspect = (float)graphics.GetWindow()->GetWidth() / (float)graphics.GetWindow()->GetHeight();
    glm::mat4 view = flyCamera->GetViewMatrix();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 1000.0f);

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
    JLEngine::JLEngineCore engine(width, height, "JL Engine", 60, 120);

    auto graphics = engine.GetGraphics();
    window = graphics->GetWindow()->GetGLFWwindow();
    input = engine.GetInput();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);

    input->SetKeyboardCallback(KeyboardCallback);
    input->SetMouseCallback(MouseCallback);
    input->SetMouseMoveCallback(MouseMoveCallback);
    graphics->GetWindow()->SetResizeCallback(WindowResizeCallback);

    //meshShader = shaderMgr->LoadShaderFromFile("SimpleMeshShader", "simple_mesh_vert.glsl", "simple_mesh_frag.glsl", "../Assets/");
    //meshShader.get()->CacheUniformLocation("uModel");
    //meshShader.get()->CacheUniformLocation("uView");
    //meshShader.get()->CacheUniformLocation("uProjection");
    //meshShader.get()->CacheUniformLocation("uLightPos");
    //meshShader.get()->CacheUniformLocation("uLightColor");
    //meshShader.get()->CacheUniformLocation("uTexture");

    sceneRoot = std::make_shared<JLEngine::Node>("SceneRoot", JLEngine::NodeTag::SceneRoot);

    //cubeMesh = JLEngine::LoadModelGLB(std::string("../Assets/cube.glb"), graphics);
    //planeMesh = JLEngine::LoadModelGLB(std::string(assetFolder + "plane.glb"), graphics);
    auto mat = engine.GetAssetLoader()->CreateMaterial("planeMat");
    mat->baseColorTexture = texture;
    //planeMesh->AddMaterial(mat);
    auto planeNode = std::make_shared<JLEngine::Node>("Plane", JLEngine::NodeTag::Mesh);
    //planeNode->meshes.push_back(planeMesh);
    //auto aduckScene = engine.GetAssetLoader()->LoadGLB(assetFolder + "Duck.glb");
    //auto afishScene = engine.GetAssetLoader()->LoadGLB(assetFolder + "BarramundiFish.glb");
    //afishScene->translation += glm::vec3(5, 0, 0);
    //afishScene->scale = glm::vec3(3.0f, 3.0f, 3.0f);
    //cubeMesh = JLEngine::Geometry::GenerateBoxMesh(graphics, "Box1", 2.0f, 2.0f, 2.0f);
    //sphereMesh = JLEngine::Geometry::GenerateSphereMesh(graphics, "Sphere1", 1.0f, 15, 15);
    
    auto test = engine.GetAssetLoader()->LoadGLB(assetFolder + "/DamagedHelmet.glb");
    test->scale = glm::vec3(0.1f, 0.1f, 0.1f);
    //sceneRoot->AddChild(test);
    sceneRoot = test;
    sceneRoot->children[0]->UpdateTransforms(sceneRoot->localMatrix);

    //sceneRoot->AddChild(std::move(aduckScene));
    //sceneRoot->AddChild(std::move(afishScene));
    //sceneRoot->AddChild(planeNode);

    m_defRenderer = new JLEngine::DeferredRenderer(graphics, engine.GetAssetLoader(),
        width, height, assetFolder);
    m_defRenderer->Initialize();

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    graphics->DumpInfo();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}