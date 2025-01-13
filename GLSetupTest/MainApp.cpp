#include "MainApp.h"
#include "GlobalVars.h"
#include "HDRISky.h"
#include "ListCycler.h"
#include "JLMath.h"

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
    glm::mat4 projection = glm::perspective(glm::radians(40.0f), aspect, frustum->GetNear(), frustum->GetFar());

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

    // Define allowed texture sizes for each parameter
    const std::vector<int> fullTextureSizes = { 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192 };
    const std::vector<int> irradianceSizes(fullTextureSizes.begin(), fullTextureSizes.begin() + 4); // 16 to 128
    const std::vector<int> prefilteredMapSizes(fullTextureSizes.begin() + 1, fullTextureSizes.begin() + 6); // 32 to 512
    const std::vector<int> prefilteredSamples(fullTextureSizes.begin() + 3, fullTextureSizes.end()); // 128 to 8192

    // Helper function to find the closest index for a value
    auto findClosestIndex = [](const std::vector<int>& values, int value) {
        auto it = std::lower_bound(values.begin(), values.end(), value);
        if (it == values.end()) return static_cast<int>(values.size()) - 1;
        return static_cast<int>(it - values.begin());
        };

    // Indices for each slider
    int irrMapIndex = findClosestIndex(irradianceSizes, skyInitParams.irradianceMapSize);
    int prefilteredMapIndex = findClosestIndex(prefilteredMapSizes, skyInitParams.prefilteredMapSize);
    int prefilteredSamplesIndex = findClosestIndex(prefilteredSamples, skyInitParams.prefilteredSamples);

    // Adjust slider width
    ImGui::PushItemWidth(150);

    // Irradiance Map Size Slider
    if (ImGui::SliderInt("##IrradianceMapSize", &irrMapIndex, 0, static_cast<int>(irradianceSizes.size()) - 1))
    {
        skyInitParams.irradianceMapSize = irradianceSizes[irrMapIndex];
    }
    ImGui::SameLine();
    ImGui::Text("Irradiance Map Size: %d", irradianceSizes[irrMapIndex]);

    // Prefiltered Map Size Slider
    if (ImGui::SliderInt("##PrefilteredMapSize", &prefilteredMapIndex, 0, static_cast<int>(prefilteredMapSizes.size()) - 1))
    {
        skyInitParams.prefilteredMapSize = prefilteredMapSizes[prefilteredMapIndex];
    }
    ImGui::SameLine();
    ImGui::Text("Prefiltered Map Size: %d", prefilteredMapSizes[prefilteredMapIndex]);

    // Prefiltered Samples Slider
    if (ImGui::SliderInt("##PrefilteredSamples", &prefilteredSamplesIndex, 0, static_cast<int>(prefilteredSamples.size()) - 1))
    {
        skyInitParams.prefilteredSamples = prefilteredSamples[prefilteredSamplesIndex];
    }
    ImGui::SameLine();
    ImGui::Text("Prefiltered Samples: %d", prefilteredSamples[prefilteredSamplesIndex]);

    // Restore default slider width
    ImGui::PopItemWidth();

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

    //auto planeNode = engine.GetResourceLoader()->LoadGLB(assetFolder + "/Plane.glb");
    //auto mat = engine.GetResourceLoader()->CreateMaterial("planeMat");
    //mat->castShadows = false;
    //mat->baseColorTexture = engine.GetResourceLoader()->CreateTexture("PlaneTexture", assetFolder + "floor_default_grid.png");
    //planeNode->mesh->GetSubmeshes()[0].materialHandle = mat.get()->GetHandle();
    //planeNode->translation -= glm::vec3(0, 2.5f, 0);
    //
    auto metallicSpheres = engine.GetResourceLoader()->LoadGLB(assetFolder + "/MetalRoughSpheres.glb");
    metallicSpheres->translation += glm::vec3(0, 2.5, -5);
    //
    //auto helmet = engine.GetResourceLoader()->LoadGLB(assetFolder + "/DamagedHelmet.glb");
    //auto matId = helmet->mesh->GetSubmeshes()[0].materialHandle;
    //engine.GetResourceLoader()->GetMaterialManager()->Get(matId)->castShadows = false;
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

    //auto bed = engine.GetResourceLoader()->LoadGLB(assetFolder + "bed_single_01.glb");
    //bed->translation = glm::vec3(5.0f, -2.0f, 5.0f);
    //auto bedMat = JLEngine::ResourceLoader::GetMat(bed.get());
    //bedMat->roughnessFactor = 1.0f;

    //auto house = engine.GetResourceLoader()->LoadGLB(assetFolder + "archviz_2.glb");
    auto boxesInstanced = engine.GetResourceLoader()->LoadGLB(assetFolder + "boxesinstanced.glb");
    boxesInstanced->translation += glm::vec3(0, 2.5, 5);

    //auto bistroScene = engine.GetResourceLoader()->LoadGLB(assetFolder + "/Bistro_Godot2.glb");
    //auto virtualCity = engine.GetResourceLoader()->LoadGLB(assetFolder + "/VirtualCity.glb");

    m_defRenderer = new JLEngine::DeferredRenderer(graphics, engine.GetResourceLoader(),
        SCREEN_WIDTH, SCREEN_HEIGHT, assetFolder);
    m_defRenderer->Initialize();
    auto sceneRoot = m_defRenderer->SceneRoot();

    //sceneRoot->AddChild(bistroScene);
    //sceneRoot->AddChild(virtualCity);
    //sceneRoot->AddChild(house);
    //sceneRoot->AddChild(planeNode);
    sceneRoot->AddChild(boxesInstanced);

    sceneRoot->AddChild(metallicSpheres);
    //sceneRoot->AddChild(bed);
    //sceneRoot->AddChild(helmet);
    //sceneRoot->AddChild(potofcoals);
    //sceneRoot->AddChild(fish);
    //sceneRoot->AddChild(cardinalDirections);
    m_defRenderer->AddVAOs(JLEngine::VAOType::STATIC, engine.GetResourceLoader()->GetGLBLoader()->GetStaticVAOs());
    m_defRenderer->AddVAOs(JLEngine::VAOType::JL_TRANSPARENT, engine.GetResourceLoader()->GetGLBLoader()->GetTransparentVAOs());
    m_defRenderer->GenerateGPUBuffers();

    flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}