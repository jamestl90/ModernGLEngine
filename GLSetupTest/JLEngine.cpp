#include "JLEngine.h"
#include "GraphicsAPI.h"
#include "ShaderStorageBuffer.h"
#include "DeferredRenderer.h"

#include <iostream> 
#include <thread>
#include <chrono>

#include <glm/gtx/matrix_decompose.hpp>
#include "UIHelperFunctions.h"
namespace JLEngine
{
    JLEngineCore::JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate, const std::string& assetFolder) :
        m_maxFrameRate(maxFrameRate), m_maxFrameRateInterval(0), m_assetFolder(assetFolder),
        m_deltaTime(0.0), m_accumulatedTime(0.0), m_fixedUpdateRate(fixedUpdates) 
    {
        // Set up fixed update interval based on target FPS
        m_fixedUpdateInterval = 1.0 / static_cast<double>(m_fixedUpdateRate);
        m_maxFrameRateInterval = 1.0 / static_cast<double>(m_maxFrameRate);

        m_window = std::make_unique<Window>(windowWidth, windowHeight, windowTitle, fixedUpdates);
        m_input = std::make_unique<Input>(m_window.get());
        
        Graphics::Initialise(m_window.get());

        m_resourceLoader = new ResourceLoader(Graphics::API());

        m_resourceLoader->SetHotReloading(true);
        m_input->SetRawMouseMotion(true);
        setVsync(false);

        Graphics::API()->DumpInfo();

        m_renderer = new DeferredRenderer(Graphics::API(), m_resourceLoader, windowWidth, windowHeight, assetFolder);
    }

    JLEngineCore::~JLEngineCore()
    {
        m_im3dManager.Shutdown();
        m_imguiManager.Shutdown();

        delete m_flyCamera;
    }

    void JLEngineCore::setFixedUpdateRate(int fps)
    {
        m_fixedUpdateRate = fps;
        m_fixedUpdateInterval = 1.0 / static_cast<double>(m_fixedUpdateRate);
    }

    void JLEngineCore::setMaxFrameRate(int fps)
    {
        m_maxFrameRate = fps;
        m_maxFrameRateInterval = 1.0 / static_cast<double>(m_maxFrameRate);
    }

    void JLEngineCore::setVsync(bool toggle)
    {
        if (m_window)
        {
            glfwSwapInterval(toggle ? 1 : 0);
        }
    }

    void JLEngineCore::run(std::function<void(double deltaTime)> logicUpdate,
        std::function<void(GraphicsAPI& graphics, double interpolationFactor)> render,
        std::function<void(double fixedDeltaTime)> fixedUpdate)
    {
        auto lastTime = std::chrono::high_resolution_clock::now();
        float fps = 0.0f;
        double frameTimeAccumulator = 0.0;

        SceneManager& sceneManager = m_renderer->GetSceneManager();
        ViewFrustum* frustum = Graphics::API()->GetViewFrustum();

        // The main game loop
        while (!m_window->ShouldClose()) 
        {
            auto frameStart = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> frameDuration = frameStart - lastTime;
            lastTime = frameStart;
            m_deltaTime = frameDuration.count(); // Delta time in seconds
            m_accumulatedTime += m_deltaTime;
            frameTimeAccumulator += m_deltaTime;

            m_im3dManager.BeginFrame(m_input.get(), m_flyCamera, frustum->GetProjectionMatrix(), frustum->GetFov(), (float)m_deltaTime);
            m_imguiManager.BeginFrame();           

            logicUpdate(m_deltaTime);

            // Fixed Update (runs at the fixed time step)
            while (m_accumulatedTime >= m_fixedUpdateInterval)
            {
                auto& skinnedAnimControllers = sceneManager.GetSkinnedAnimationControllers();
                auto& rigidAnimControllers = sceneManager.GetRigidAnimationControllers();
                for (auto& [controller, node] : skinnedAnimControllers)
                {
                    controller->Update(static_cast<float>(m_fixedUpdateInterval));
                }
                for (auto& [controller, node] : rigidAnimControllers)
                {
                    controller->Update(static_cast<float>(m_fixedUpdateInterval));
                }
                fixedUpdate(m_fixedUpdateInterval); 
                m_accumulatedTime -= m_fixedUpdateInterval; 
                m_fixedUpdateCount++;   
            }

            if (frameTimeAccumulator >= 1.0)
            {
                fps = (float)m_frameCount;
                m_frameCount = 0;
                frameTimeAccumulator = 0.0f;
            }

            // any im3D gizmo/debug drawing must be called before render

            //double interpolationFactor = m_accumulatedTime / m_fixedUpdateInterval;
            render(*Graphics::API(), m_deltaTime);
            m_frameCount++;

            auto view = m_flyCamera->GetViewMatrix();
            auto proj = frustum->GetProjectionMatrix();

            ImGui::Begin("Info");
            ImGui::Text("FPS: %.1f", fps);
            ImGui::Text("Delta Time: %.3f ms", m_deltaTime * 1000.0);
            ImGui::Separator();
            ImGui::Text("Controls:");
            ImGui::BulletText("W / A / S / D - Move");
            ImGui::BulletText("G - Cycle Debug");
            ImGui::BulletText("T - Lock/Free Cursor");
            ImGui::BulletText("Esc - Quit");
            ImGui::End();

            ShowNodeHierarchy(m_renderer->GetSceneManager().GetRoot().get());
            m_imguiManager.EndFrame();

            m_window->SwapBuffers();
            m_window->PollEvents();

            //logPerformanceMetrics();
            m_resourceLoader->PollForChanges((float)m_deltaTime);
        }
    }

    std::shared_ptr<Node> JLEngineCore::Load(const std::string& fileName)
    {
        auto node = m_resourceLoader->LoadGLB(fileName);
        return node;
    }

    std::shared_ptr<Node> JLEngineCore::LoadAndAttachToRoot(const std::string& fileName)
    {
        auto node = m_resourceLoader->LoadGLB(fileName);
        m_renderer->SceneRoot()->AddChild(node);
        return node;
    }

    std::shared_ptr<Node> JLEngineCore::LoadAndAttachToRoot(const std::string& fileName, const glm::vec3& pos)
    {
        auto node = m_resourceLoader->LoadGLB(fileName);
        node->translation = pos;
        m_renderer->SceneRoot()->AddChild(node);
        return node;
    }

    std::shared_ptr<Node> JLEngineCore::LoadAndAttachToRoot(const std::string& fileName, const glm::vec3& pos, const glm::quat& rotation, const glm::vec3& scale)
    {
        auto node = m_resourceLoader->LoadGLB(fileName);
        node->translation = pos;
        node->rotation = rotation;
        node->scale = scale;
        m_renderer->SceneRoot()->AddChild(node);
        return node;
    }

    std::shared_ptr<Node> JLEngineCore::LoadAndAttachToRoot(const std::string& fileName, const glm::mat4& transform)
    {
        auto node = m_resourceLoader->LoadGLB(fileName);
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::decompose(transform, node->scale, node->rotation, node->translation, skew, perspective);
        m_renderer->SceneRoot()->AddChild(node);
        return node;
    }

    std::shared_ptr<Node> JLEngineCore::MakeInstanceOf(std::shared_ptr<Node>& existingNode, const glm::vec3& pos, bool attachToRoot)
    {
        auto& submesh = existingNode->mesh->GetSubmesh(0);
        auto instanceCount = submesh.instanceTransforms == nullptr ? 2 : submesh.instanceTransforms->size() + 1;
        auto newNode = std::make_shared<Node>(existingNode->name + std::to_string(instanceCount));

        // Step 1: Clone Mesh and Animation (if applicable)
        auto& mesh = existingNode->mesh;
        if (mesh != nullptr)
        {
            newNode->mesh = mesh;
            newNode->tag = existingNode->tag;

            if (mesh->GetSkeleton() != nullptr)
            {
                newNode->mesh->SetSkeleton(mesh->GetSkeleton());
                newNode->animController = std::make_shared<AnimationController>();
            }

            // Handle instancing for skinned meshes
            auto& submesh = newNode->mesh->GetSubmesh(0);
            if (instanceCount == 2)
            {
                if (submesh.instanceTransforms == nullptr)
                {
                    submesh.instanceTransforms = std::make_shared<std::vector<Node*>>();
                    submesh.command.instanceCount = 1;
                    submesh.instanceTransforms->push_back(existingNode.get());
                }
            }
            submesh.command.instanceCount++;
            submesh.instanceTransforms->push_back(newNode.get());
        }

        // Step 2: Check if the parent is SceneRoot and Skip Hierarchy Reconstruction
        std::weak_ptr<Node> parentWeak = existingNode->parent;
        auto parentLocked = parentWeak.lock();

        if (parentLocked && parentLocked->tag == NodeTag::SceneRoot)
        {
            // The mesh is in the SceneRoot, no parent hierarchy needed
            newNode->translation = pos;
            newNode->rotation = existingNode->rotation;
            newNode->scale = existingNode->scale;

            // Recursively clone child nodes
            CloneChildNodes(existingNode, newNode);

            if (attachToRoot)
            {
                m_renderer->SceneRoot()->AddChild(newNode);
            }

            return newNode; // Skip hierarchy cloning and return immediately
        }

        // Step 3: Reconstruct the Parent Hierarchy (Only if Parent is NOT SceneRoot)
        std::shared_ptr<Node> lastInstance = newNode;
        std::shared_ptr<Node> rootInstance = nullptr;

        while (auto currentOriginal = parentWeak.lock()) // Lock weak_ptr before using it
        {
            auto newParent = std::make_shared<Node>(currentOriginal->name + "_inst");

            // Copy Transform Data
            newParent->translation = currentOriginal->translation;
            newParent->rotation = currentOriginal->rotation;
            newParent->scale = currentOriginal->scale;
            newParent->tag = currentOriginal->tag;

            // Attach the last created node as its child
            newParent->AddChild(lastInstance);

            // Recursively clone child nodes
            CloneChildNodes(currentOriginal, newParent);

            // Move up the hierarchy
            lastInstance = newParent;
            rootInstance = newParent;
            parentWeak = currentOriginal->parent;
        }

        // Step 4: Apply Position Offset **Only to the Top-Most Parent**
        if (rootInstance)
        {
            rootInstance->translation += pos;
            rootInstance->localMatrix = glm::translate(glm::mat4(1.0f), rootInstance->translation);
        }
        else
        {
            newNode->translation += pos;
            newNode->localMatrix = glm::translate(glm::mat4(1.0f), newNode->translation);
        }

        // Step 5: Attach to the scene root if requested
        if (attachToRoot)
        {
            m_renderer->SceneRoot()->AddChild(rootInstance ? rootInstance : newNode);
        }

        return rootInstance ? rootInstance : newNode;
    }

    // Helper function to recursively clone child nodes
    void JLEngineCore::CloneChildNodes(const std::shared_ptr<Node>& originalNode, const std::shared_ptr<Node>& clonedNode)
    {
        for (const auto& child : originalNode->children)
        {
            auto clonedChild = std::make_shared<Node>(child->name + "_inst");

            // Copy Transform Data
            clonedChild->translation = child->translation;
            clonedChild->rotation = child->rotation;
            clonedChild->scale = child->scale;
            clonedChild->tag = child->tag;

            // Clone Mesh and Animation (if applicable)
            if (child->mesh != nullptr)
            {
                clonedChild->mesh = child->mesh;

                if (child->mesh->GetSkeleton() != nullptr)
                {
                    clonedChild->mesh->SetSkeleton(child->mesh->GetSkeleton());
                    clonedChild->animController = std::make_shared<AnimationController>();
                }
            }

            // Recursively clone child nodes
            CloneChildNodes(child, clonedChild);

            // Attach the cloned child to the cloned parent
            clonedNode->AddChild(clonedChild);
        }
    }

    void JLEngineCore::FinalizeLoading()
    {
        auto glbLoader = m_resourceLoader->GetGLBLoader();
        if (glbLoader == nullptr) return;

        auto& staticVAOs = glbLoader->GetStaticVAOs();
        auto& transparentVAOs = glbLoader->GetTransparentVAOs();
        auto& skinnedMeshVAOs = glbLoader->GetDynamicVAOs();

        m_renderer->EarlyInitialize();
        m_renderer->AddVAOs(JLEngine::VAOType::STATIC, staticVAOs);
        m_renderer->AddVAOs(JLEngine::VAOType::JL_TRANSPARENT, transparentVAOs);

        if (skinnedMeshVAOs.size() > 0)
            m_renderer->AddVAO(JLEngine::VAOType::DYNAMIC, skinnedMeshVAOs.begin()->first, skinnedMeshVAOs.begin()->second);
        m_renderer->GenerateGPUBuffers();
        m_renderer->LateInitialize();
    }

    GraphicsAPI* JLEngineCore::GetGraphicsAPI() const
    {
        return Graphics::API();
    }
    Input* JLEngineCore::GetInput() const
    {
        return m_input.get();
    }
    ResourceLoader* JLEngineCore::GetResourceLoader() const
    {
        return m_resourceLoader;
    }
    DeferredRenderer* JLEngineCore::GetRenderer() const
    {
        return m_renderer;
    }

    FlyCamera* JLEngineCore::GetFlyCamera()
    {
        if (m_flyCamera == nullptr)
        {
            m_flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
        }
        return m_flyCamera;
    }

    void JLEngineCore::InitIMGUI()
    {
        m_imguiManager.Initialize(m_window->GetGLFWwindow());
        m_im3dManager.Initialise(m_window.get(), m_resourceLoader, m_assetFolder);
        m_renderer->SetDebugDrawer(&m_im3dManager);
    }
}