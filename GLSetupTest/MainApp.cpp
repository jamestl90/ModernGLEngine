#include "MainApp.h"
#include "GlobalVars.h"
#include "HDRISky.h"
#include "ListCycler.h"

JLEngine::FlyCamera* flyCamera;
JLEngine::Input* input;
JLEngine::Mesh* cubeMesh;
JLEngine::Mesh* planeMesh;
JLEngine::Mesh* sphereMesh;
std::shared_ptr<JLEngine::Node> sceneRoot;
std::shared_ptr<JLEngine::Node> cardinalDirections;
std::string m_assetPath;
JLEngine::ShaderProgram* meshShader;
JLEngine::ShaderProgram* basicLit;
JLEngine::DeferredRenderer* m_defRenderer;
GLFWwindow* window;
JLEngine::ListCycler<std::string> hdriSkyCycler;
JLEngine::HdriSkyInitParams skyInitParams;

void gameRender(JLEngine::GraphicsAPI& graphics, double interpolationFactor)
{
    float aspect = (float)graphics.GetWindow()->GetWidth() / (float)graphics.GetWindow()->GetHeight();
    glm::mat4 view = flyCamera->GetViewMatrix();
    auto frustum = graphics.GetViewFrustum();
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, frustum->GetNear(), frustum->GetFar());

    glm::vec3 lightDirection = glm::normalize(m_defRenderer->GetDirectionalLight().direction);
    glm::vec3 upVector = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::quat rotation = glm::quatLookAt(lightDirection, upVector);

    //cardinalDirections->SetTranslationRotation(m_defRenderer->GetDirectionalLight().position, rotation);

    m_defRenderer->Render(flyCamera->GetPosition(), view, projection);
    //m_defRenderer->Render(sceneRoot.get(), flyCamera->GetPosition(), view, projection);
}

void gameLogicUpdate(double deltaTime)
{
    flyCamera->ProcessKeyboard(window, (float)deltaTime);

    ImGui::Begin("HDRI Sky Settings");
    ImGui::SliderInt("Irradiance Map Size", &skyInitParams.irradianceMapSize, 16, 128);
    ImGui::SliderInt("Prefiltered Map Size", &skyInitParams.prefilteredMapSize, 32, 512);
    ImGui::SliderInt("Prefiltered Samples", &skyInitParams.prefilteredSamples, 256, 8192);
    ImGui::SliderFloat("Compression Threshold", &skyInitParams.compressionThreshold, 1.0f, 5.0f);
    ImGui::SliderFloat("Max Value", &skyInitParams.maxValue, 1.0f, 20000.0f);

    if (ImGui::Button("Regenerate"))
    {
        skyInitParams.fileName = hdriSkyCycler.Current();
        auto hdriSky = m_defRenderer->GetHDRISky();
        hdriSky->Reload(m_assetPath, skyInitParams);
    }
    if (ImGui::Button("Next"))
    {
        skyInitParams.fileName = hdriSkyCycler.Next();
        auto hdriSky = m_defRenderer->GetHDRISky();
        hdriSky->Reload(m_assetPath, skyInitParams);
    }
    ImGui::End();
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

int MainApp(std::string assetFolder)
{
    m_assetPath = assetFolder;
    JLEngine::JLEngineCore engine(SCREEN_WIDTH, SCREEN_HEIGHT, "JL Engine", 60, 120);

    auto graphics = engine.GetGraphicsAPI();
    window = graphics->GetWindow()->GetGLFWwindow();
    input = engine.GetInput();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);

    std::vector<std::string> skyTextures = {
        "kloofendal_48d_partly_cloudy_puresky_4k.hdr",
        "metro_noord_4k.hdr",
        "moonless_golf_4k.hdr",
        "rogland_clear_night_4k.hdr",
        "venice_sunset_4k.hdr"
    };
    hdriSkyCycler.Set(std::move(skyTextures));

    input->SetKeyboardCallback(KeyboardCallback);
    input->SetMouseCallback(MouseCallback);
    input->SetMouseMoveCallback(MouseMoveCallback);
    graphics->GetWindow()->SetResizeCallback(WindowResizeCallback);
    engine.InitIMGUI();

    sceneRoot = std::make_shared<JLEngine::Node>("SceneRoot", JLEngine::NodeTag::SceneRoot);

    auto planeNode = engine.GetResourceLoader()->LoadGLB(assetFolder + "/Plane.glb");
    auto mat = engine.GetResourceLoader()->CreateMaterial("planeMat");
    mat->castShadows = false;
    mat->baseColorTexture = engine.GetResourceLoader()->CreateTexture("PlaneTexture", assetFolder + "floor_default_grid.png");
    planeNode->mesh->GetSubmeshes()[0].materialHandle = mat.get()->GetHandle();
    planeNode->translation -= glm::vec3(0, 2.5f, 0);
    //
    auto metallicSpheres = engine.GetResourceLoader()->LoadGLB(assetFolder + "/MetalRoughSpheres.glb");
    metallicSpheres->translation += glm::vec3(0, 2.5, -5);
    
    auto helmet = engine.GetResourceLoader()->LoadGLB(assetFolder + "/DamagedHelmet.glb");
    auto matId = helmet->mesh->GetSubmeshes()[0].materialHandle;
    engine.GetResourceLoader()->GetMaterialManager()->Get(matId)->castShadows = false;
    //
    //auto potofcoals = engine.GetResourceLoader()->LoadGLB(assetFolder + "/PotOfCoals.glb");
    //potofcoals->scale = glm::vec3(15.0f, 15.0f, 15.0f);
    //potofcoals->translation = glm::vec3(5.0f, 0.0f, 0.0f);
    //
    //auto fish = engine.GetResourceLoader()->LoadGLB(assetFolder + "/BarramundiFish.glb");
    //fish->scale = glm::vec3(5.0f, 5.0f, 5.0f);
    //fish->translation = glm::vec3(-5.0f, 0.0f, 0.0f);
    //
    //cardinalDirections = engine.GetResourceLoader()->LoadGLB(assetFolder + "/cardinaldirections.glb");

    //auto bistroScene = engine.GetResourceLoader()->LoadGLB(assetFolder + "/Bistro_Godot2.glb");
    //auto virtualCity = engine.GetResourceLoader()->LoadGLB(assetFolder + "/VirtualCity.glb");

    //sceneRoot->AddChild(bistroScene);
    //sceneRoot->AddChild(virtualCity);
    sceneRoot->AddChild(planeNode);
    sceneRoot->AddChild(metallicSpheres);
    sceneRoot->AddChild(helmet);
    //sceneRoot->AddChild(potofcoals);
    //sceneRoot->AddChild(fish);
    //sceneRoot->AddChild(cardinalDirections);

    m_defRenderer = new JLEngine::DeferredRenderer(graphics, engine.GetResourceLoader(),
        SCREEN_WIDTH, SCREEN_HEIGHT, assetFolder);
    m_defRenderer->Initialize();
    m_defRenderer->AddVAOs(true, engine.GetResourceLoader()->GetGLBLoader()->GetStaticVAOs());
    m_defRenderer->GenerateGPUBuffers(sceneRoot.get());

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}