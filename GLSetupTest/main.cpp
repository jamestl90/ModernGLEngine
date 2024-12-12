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
std::shared_ptr<JLEngine::Node> cardinalDirections;
JLEngine::ShaderProgram* meshShader;
JLEngine::ShaderProgram* basicLit;
JLEngine::DeferredRenderer* m_defRenderer;
GLFWwindow* window;

int width = 1920;
int height = 1080;

void gameRender(JLEngine::Graphics& graphics, double interpolationFactor)
{
    float aspect = (float)graphics.GetWindow()->GetWidth() / (float)graphics.GetWindow()->GetHeight();
    glm::mat4 view = flyCamera->GetViewMatrix();
    auto frustum = graphics.GetViewFrustum();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, frustum->GetNear(), frustum->GetFar());

    glm::vec3 lightDirection = glm::normalize(m_defRenderer->GetDirectionalLight().direction);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::quat rotation = glm::quatLookAt(lightDirection, upVector);

    cardinalDirections->SetTranslationRotation(m_defRenderer->GetDirectionalLight().position, rotation);

    m_defRenderer->Render(sceneRoot.get(), flyCamera->GetPosition(), view, projection);
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
        m_defRenderer->CycleDebugMode();
    }

    if (glfwGetKey(window, GLFW_KEY_T))
    {
        flyCamera->ToggleFreeMouse(input);
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
    engine.InitIMGUI();

    sceneRoot = std::make_shared<JLEngine::Node>("SceneRoot", JLEngine::NodeTag::SceneRoot);

    //auto planeNode = engine.GetAssetLoader()->LoadGLB(assetFolder + "/Plane.glb");
    //auto mat = engine.GetAssetLoader()->CreateMaterial("planeMat");
    //mat->baseColorTexture = engine.GetAssetLoader()->CreateTextureFromFile("PlaneTexture", assetFolder + "floor_default_grid.png");
    //planeNode->mesh->GetBatches()[0]->SetMaterial(mat);
    //planeNode->translation -= glm::vec3(0, 2.5f, 0);
    //
    //auto metallicSpheres = engine.GetAssetLoader()->LoadGLB(assetFolder + "/MetalRoughSpheres.glb");
    //metallicSpheres->translation += glm::vec3(0, 2.5, -5);
    //metallicSpheres->receiveShadows = false;
    //
    //auto helmet = engine.GetAssetLoader()->LoadGLB(assetFolder + "/DamagedHelmet.glb");
    //
    //auto potofcoals = engine.GetAssetLoader()->LoadGLB(assetFolder + "/PotOfCoals.glb");
    //potofcoals->scale = glm::vec3(15.0f, 15.0f, 15.0f);
    //potofcoals->translation = glm::vec3(5.0f, 0.0f, 0.0f);
    //
    //auto fish = engine.GetAssetLoader()->LoadGLB(assetFolder + "/BarramundiFish.glb");
    //fish->scale = glm::vec3(5.0f, 5.0f, 5.0f);
    //fish->translation = glm::vec3(-5.0f, 0.0f, 0.0f);
    
    cardinalDirections = engine.GetAssetLoader()->LoadGLB(assetFolder + "/cardinaldirections.glb");

    auto bistroScene = engine.GetAssetLoader()->LoadGLB(assetFolder + "/Bistro_Godot2.glb");
    //auto virtualCity = engine.GetAssetLoader()->LoadGLB(assetFolder + "/VirtualCity.glb");

    sceneRoot->AddChild(bistroScene);
    //sceneRoot->AddChild(virtualCity);
    //sceneRoot->AddChild(planeNode);
    //sceneRoot->AddChild(metallicSpheres);
    //sceneRoot->AddChild(helmet);
    //sceneRoot->AddChild(potofcoals);
    //sceneRoot->AddChild(fish);
    sceneRoot->AddChild(cardinalDirections);
    
    m_defRenderer = new JLEngine::DeferredRenderer(graphics, engine.GetAssetLoader(),
        width, height, assetFolder);
    m_defRenderer->Initialize();

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    graphics->DumpInfo();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}