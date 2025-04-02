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
std::string m_assetPath;
JLEngine::ShaderProgram* meshShader;
JLEngine::ShaderProgram* basicLit;
GLFWwindow* window;
JLEngine::ListCycler<std::string> hdriSkyCycler;
JLEngine::HdriSkyInitParams skyInitParams;
JLEngine::DeferredRenderer* renderer;

void gameRender(JLEngine::GraphicsAPI& graphics, double dt)
{
    glm::mat4 view = flyCamera->GetViewMatrix();
    auto frustum = graphics.GetViewFrustum();

    renderer->Render(flyCamera->GetPosition(), flyCamera->GetForward(), view, frustum->GetProjectionMatrix(), dt);
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
        auto hdriSky = renderer->GetHDRISky();
        hdriSky->Reload(m_assetPath, skyInitParams);
    }
    if (ImGui::Button("Next"))
    {
        skyInitParams.fileName = hdriSkyCycler.Next();
        auto hdriSky = renderer->GetHDRISky();
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
        renderer->CycleDebugMode();
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
    renderer->Resize(width, height);
}

// demonstrates instancing for both normal meshes and animated (skinned) meshes
void DemoInstancing(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    auto helmet = engine.LoadAndAttachToRoot(assetFolder + "DamagedHelmet.glb", glm::vec3(0, 5, 0));

    auto runningGuy = engine.LoadAndAttachToRoot(assetFolder + "CesiumMan.glb", glm::vec3(0, 0, 0));
    auto anim = engine.GetResourceLoader()->Get<JLEngine::Animation>("Anim_Skeleton_torso_joint_1_idx:0");
    auto skeletonNode = JLEngine::Node::FindSkeletonNode(runningGuy);
    skeletonNode->animController->SetCurrentAnimation(anim.get());

    glm::vec3 pos = glm::vec3(0.0f);
    for (int i = 0; i < 55; i++)
    {
        for (int j = 0; j < 55; j++)
        {
            if (i == 0.0f && j == 0.0f) continue;

            pos.x = (float)i * 1.5f;
            pos.z = (float)j * 1.5f;
            glm::vec3 pos(i * 2.0f, 0, j * 2.0f);
            glm::vec3 pos2(i * 2.0f, 5, j * 2.0f);
            auto newNode = engine.MakeInstanceOf(skeletonNode, pos, true);
            auto newSkeletonNode = JLEngine::Node::FindSkeletonNode(newNode);
            newSkeletonNode->animController->SetCurrentAnimation(anim.get());

            auto newHelmetInstance = engine.MakeInstanceOf(helmet, pos2, true);
            newHelmetInstance->UpdateHierarchy();
        }
    }
}
void DemoSkinning(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    auto runningGuy = engine.LoadAndAttachToRoot(assetFolder + "CesiumMan.glb", glm::vec3(-6, 0, 0));
    auto anim = engine.GetResourceLoader()->Get<JLEngine::Animation>("Anim_Skeleton_torso_joint_1_idx:0");
    auto skeletonNode = JLEngine::Node::FindSkeletonNode(runningGuy);
    skeletonNode->animController->SetCurrentAnimation(anim.get());
    //skeletonNode->animController->SetPlaybackSpeed(0.25f);
}

int MainApp(std::string assetFolder)
{
    m_assetPath = assetFolder;
    JLEngine::JLEngineCore engine(SCREEN_WIDTH, SCREEN_HEIGHT, "JL Engine", 60, 120, assetFolder);

    auto graphics = engine.GetGraphicsAPI();
    window = graphics->GetWindow()->GetGLFWwindow();
    input = engine.GetInput();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);
    input->SetKeyboardCallback(KeyboardCallback);
    input->SetMouseCallback(MouseCallback);
    input->SetMouseMoveCallback(MouseMoveCallback);
    graphics->GetWindow()->SetResizeCallback(WindowResizeCallback);
    engine.InitIMGUI();

    std::vector<std::string> skyTextures = {
    "kloofendal_48d_partly_cloudy_puresky_4k.hdr",
    "metro_noord_4k.hdr",
    "moonless_golf_4k.hdr",
    "rogland_clear_night_4k.hdr",
    "venice_sunset_4k.hdr"
    };
    hdriSkyCycler.Set(std::move(skyTextures));

    flyCamera = engine.GetFlyCamera();

    //engine.LoadAndAttachToRoot(assetFolder + "PotOfCoals.glb", glm::vec3(5.0f, 0.0f, 0.0f), glm::quat(), glm::vec3(15, 15, 15));
    //engine.LoadAndAttachToRoot(assetFolder + "BarramundiFish.glb", glm::vec3(-5.0f, 0.0f, 0.0f), glm::quat(), glm::vec3(5, 5, 5));
    //engine.LoadAndAttachToRoot(assetFolder + "bed_single_01.glb", glm::vec3(5, 0, 5));
    //
    //engine.LoadAndAttachToRoot(assetFolder + "archviz_2.glb");
    //engine.LoadAndAttachToRoot(assetFolder + "Bistro_Godot2.glb");
    
    auto plane = engine.LoadAndAttachToRoot(assetFolder + "Plane.glb", glm::vec3(0, 0, 0));
    auto planeMat = engine.GetResourceLoader()->CreateMaterial("planeMat");
    planeMat->metallicFactor = 0.0f;
    planeMat->roughnessFactor = 0.2f;
    planeMat->castShadows = false;
    planeMat->baseColorTexture = engine.GetResourceLoader()->CreateTexture("PlaneTexture", assetFolder + "floor_default_grid.png");
    plane->mesh->GetSubmeshes()[0].materialHandle = planeMat.get()->GetHandle();
    
    //engine.LoadAndAttachToRoot(assetFolder + "Wood_Tower.glb", glm::vec3(10, 0, 10), glm::quat(), glm::vec3(0.15f, 0.15f, 0.15f));
    //engine.LoadAndAttachToRoot(assetFolder + "MetalRoughSpheres.glb", glm::vec3(0, 5, -5));
    //engine.LoadAndAttachToRoot(assetFolder + "boxesinstanced.glb", glm::vec3(15, 2.5, 5));

    //auto cubewithanim = engine.LoadAndAttachToRoot(assetFolder + "cubewithanim.glb", glm::vec3(10,0,0));

    auto helmet = engine.LoadAndAttachToRoot(assetFolder + "DamagedHelmet.glb", glm::vec3(5, 1, 5));

    auto building = engine.LoadAndAttachToRoot(assetFolder + "talesfromtheloop.glb", glm::vec3(0, 0, 0));
    auto tree1 = engine.LoadAndAttachToRoot(assetFolder + "Tree1.glb", glm::vec3(4, 0, 0));
    engine.MakeInstanceOf(tree1, glm::vec3(-4, 0, 0), true);

    auto depthTestCube = engine.LoadAndAttachToRoot(assetFolder + "depthTest.glb", glm::vec3(-6, 1, 6));

    //DemoInstancing(engine, m_assetPath);
    DemoSkinning(engine, m_assetPath);

    engine.FinalizeLoading();
    renderer = engine.GetRenderer();

    engine.run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}