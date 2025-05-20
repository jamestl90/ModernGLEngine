#include "MainApp.h"
#include "GlobalVars.h"
#include "HDRISky.h"
#include "ListCycler.h"
#include "JLMath.h"
#include "DDGI.h"

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
JLEngine::DeferredRenderer* renderer;

void gameRender(JLEngine::GraphicsAPI& graphics, double dt)
{
    renderer->Render(*flyCamera, dt);
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

void TestScene1(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
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

    engine.LoadAndAttachToRoot(assetFolder + "Wood_Tower.glb", glm::vec3(10, 0, 10), glm::quat(), glm::vec3(0.15f, 0.15f, 0.15f));
    //engine.LoadAndAttachToRoot(assetFolder + "MetalRoughSpheres.glb", glm::vec3(0, 5, -5));
    //engine.LoadAndAttachToRoot(assetFolder + "boxesinstanced.glb", glm::vec3(15, 2.5, 5));

    //auto cubewithanim = engine.LoadAndAttachToRoot(assetFolder + "cubewithanim.glb", glm::vec3(10,0,0));

    auto helmet = engine.LoadAndAttachToRoot(assetFolder + "DamagedHelmet.glb", glm::vec3(5, 1, 5));

    auto building = engine.LoadAndAttachToRoot(assetFolder + "talesfromtheloop.glb", glm::vec3(0, 0, 0));
    auto tree1 = engine.LoadAndAttachToRoot(assetFolder + "Tree1.glb", glm::vec3(4, 0, 0));
    engine.MakeInstanceOf(tree1, glm::vec3(-4, 0, 0), true);

    //auto boxHouse = engine.LoadAndAttachToRoot(assetFolder + "indoorTest.glb", glm::vec3(-10, 2, -10));

    //DemoInstancing(engine, m_assetPath);
    DemoSkinning(engine, m_assetPath);
}

void TestScene2(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    auto plane = engine.LoadAndAttachToRoot(assetFolder + "Plane.glb", glm::vec3(0, -1.0f, 0));
    auto planeMat = engine.GetResourceLoader()->CreateMaterial("planeMat");
    planeMat->metallicFactor = 0.0f;
    planeMat->roughnessFactor = 0.2f;
    planeMat->castShadows = false;
    planeMat->baseColorTexture = engine.GetResourceLoader()->CreateTexture("PlaneTexture", assetFolder + "floor_default_grid.png");
    plane->mesh->GetSubmeshes()[0].materialHandle = planeMat.get()->GetHandle();

    auto* ddgi = renderer->GetDDGI();
    ddgi->SetGridOrigin({ 0.0f, 0.0f, 0.0f });
    ddgi->SetGridResolution({ 8.0f, 7.0f, 8.0f });
    ddgi->SetProbeSpacing({ 0.7f, 0.7f, 0.7f });

    ddgi->SetRaysPerProbe(128);
    ddgi->SetDebugRayCount(128);

    auto* vgm = renderer->GetVoxelGridManager();
    auto& vg = vgm->GetVoxelGrid();
    vg.worldOrigin = glm::vec3({ 0.0f, 0.0f, 0.0f });
    vg.resolution = glm::vec3({ 256, 256, 256 });
    vg.worldSize = glm::vec3({ 5, 5, 5 });

    auto indoor = engine.LoadAndAttachToRoot(assetFolder + "indoorTest.glb");

    //auto runningGuy = engine.LoadAndAttachToRoot(assetFolder + "CesiumMan.glb", glm::vec3(0, -1.8f, 0));
    //auto anim = engine.GetResourceLoader()->Get<JLEngine::Animation>("Anim_Skeleton_torso_joint_1_idx:0");
    //auto skeletonNode = JLEngine::Node::FindSkeletonNode(runningGuy);
    //skeletonNode->animController->SetCurrentAnimation(anim.get());

    //auto helmet = engine.LoadAndAttachToRoot(assetFolder + "DamagedHelmet.glb", glm::vec3(0, 0, 0));

    auto glowCube = engine.LoadAndAttachToRoot(assetFolder + "glowingSphere.glb", glm::vec3(-1, 0.0f, 0));
}

void TestScene3(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    auto* ddgi = renderer->GetDDGI();
    ddgi->SetGridOrigin({ 0.0f, 0.0f, 0.0f });
    ddgi->SetGridResolution({ 18.0f, 8.0f, 12.0f });
    ddgi->SetProbeSpacing({ 1.0f, 0.7f, 1.1f });

    ddgi->SetRaysPerProbe(128);
    ddgi->SetDebugRayCount(128);

    auto* vgm = renderer->GetVoxelGridManager();
    auto& vg = vgm->GetVoxelGrid();
    vg.worldOrigin = glm::vec3({ 0.0f, 0.0f, 0.0f });
    vg.resolution = glm::vec3({ 256, 256, 256 });
    vg.worldSize = glm::vec3({ 15, 7, 10 });

    auto house = engine.LoadAndAttachToRoot(assetFolder + "gihousetest.glb", glm::vec3(0.0f, 0.0f, 0.0f));

    auto glowCube = engine.LoadAndAttachToRoot(assetFolder + "glowingCube.glb", glm::vec3(-5, -0.25f, -3.5));
    auto glowSphere = engine.LoadAndAttachToRoot(assetFolder + "glowingSpheres.glb", glm::vec3(-1, 0.0f, 2));
}

void TestScene4(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    auto plane = engine.LoadAndAttachToRoot(assetFolder + "Plane.glb", glm::vec3(0, 0, 0));
    auto planeMat = engine.GetResourceLoader()->CreateMaterial("planeMat");
    planeMat->metallicFactor = 0.0f;
    planeMat->roughnessFactor = 0.2f;
    planeMat->castShadows = false;
    planeMat->baseColorTexture = engine.GetResourceLoader()->CreateTexture("PlaneTexture", assetFolder + "floor_default_grid.png");
    plane->mesh->GetSubmeshes()[0].materialHandle = planeMat.get()->GetHandle();

    engine.LoadAndAttachToRoot(assetFolder + "MetalRoughSpheres.glb", glm::vec3(0, 5, -5));
}

void TestScene5(JLEngine::JLEngineCore& engine, const std::string& assetFolder)
{
    engine.LoadAndAttachToRoot(assetFolder + "Bistro_Part.glb");
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
    renderer = engine.GetRenderer();
    flyCamera = engine.GetFlyCamera();

    TestScene5(engine, assetFolder);

    engine.FinalizeLoading();

    engine.Run(gameLogicUpdate, gameRender, fixedUpdate);

    return 0;
}