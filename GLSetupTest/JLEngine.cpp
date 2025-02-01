#include "JLEngine.h"
#include "GraphicsAPI.h"
#include "ShaderStorageBuffer.h"
#include "DeferredRenderer.h"

#include <iostream> 
#include <thread>
#include <chrono>

#include <glm/gtx/matrix_decompose.hpp>
namespace JLEngine
{
    JLEngineCore::JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate, const std::string& assetFolder) :
        m_maxFrameRate(maxFrameRate), m_maxFrameRateInterval(0),
        m_lastFrameTime(0.0), m_deltaTime(0.0), m_accumulatedTime(0.0), m_fixedUpdateRate(fixedUpdates) 
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
        setVsync(true);

        Graphics::API()->DumpInfo();

        m_renderer = new DeferredRenderer(Graphics::API(), m_resourceLoader, windowWidth, windowHeight, assetFolder);
    }

    JLEngineCore::~JLEngineCore()
    {

    }

    void JLEngineCore::logPerformanceMetrics()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_debugTimer);

        if (elapsed.count() >= 1) {
            std::cout << "FPS: " << m_frameCount
                << ", Fixed Updates: " << m_fixedUpdateCount 
                << ", Delta Time: " << m_deltaTime << "s\n";

            // Reset counters and timer
            m_frameCount = 0;
            m_fixedUpdateCount = 0;
            m_debugTimer = now;
        }
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

        // The main game loop
        while (!m_window->ShouldClose()) 
        {
            auto frameStart = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> frameDuration = frameStart - lastTime;
            lastTime = frameStart;
            m_deltaTime = frameDuration.count(); // Delta time in seconds
            m_accumulatedTime += m_deltaTime;
            frameTimeAccumulator += m_deltaTime;

            m_imguiManager.BeginFrame();

            auto& animControllers = m_renderer->GetSceneManager().GetAnimationControllers();
            for (auto& [controller, node] : animControllers) { controller->Update((float)m_deltaTime); }
            logicUpdate(m_deltaTime);

            // Fixed Update (runs at the fixed time step)
            while (m_accumulatedTime >= m_fixedUpdateInterval)
            {                
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

            double interpolationFactor = m_accumulatedTime / m_fixedUpdateInterval;
            render(*Graphics::API(), interpolationFactor);
            m_frameCount++;

            ImGui::Begin("Info");

            ImGui::End();

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

    std::shared_ptr<Node> JLEngineCore::MakeInstanceOf(std::shared_ptr<Node>& existingNode, glm::vec3& pos, bool attachToRoot)
    {
        auto& submesh = existingNode->mesh->GetSubmesh(0);
        auto instanceCount = submesh.instanceTransforms == nullptr ? 2 : submesh.instanceTransforms->size() + 1;
        auto newNode = std::make_shared<Node>(existingNode->name + std::to_string(instanceCount));

        // Step 1: Clone Mesh and Animation (if applicable)
        auto mesh = existingNode->mesh;
        if (mesh != nullptr)
        {
            newNode->mesh = mesh;
            newNode->tag = existingNode->tag;

            if (mesh->GetSkeleton() != nullptr)
            {
                newNode->mesh->SetSkeleton(mesh->GetSkeleton());
                newNode->animController = std::make_shared<AnimationController>();
                newNode->animController->SetSkeleton(mesh->GetSkeleton());
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


    void JLEngineCore::FinalizeLoading()
    {
        m_renderer->Initialize();
        m_renderer->AddVAOs(JLEngine::VAOType::STATIC, m_resourceLoader->GetGLBLoader()->GetStaticVAOs());
        m_renderer->AddVAOs(JLEngine::VAOType::JL_TRANSPARENT, m_resourceLoader->GetGLBLoader()->GetTransparentVAOs());
        auto& skinnedMeshVAOs = m_resourceLoader->GetGLBLoader()->GetDynamicVAOs();
        if (skinnedMeshVAOs.size() > 0)
            m_renderer->AddVAO(JLEngine::VAOType::DYNAMIC, skinnedMeshVAOs.begin()->first, skinnedMeshVAOs.begin()->second);
        m_renderer->GenerateGPUBuffers();
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
}