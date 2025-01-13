#include "JLEngine.h"
#include "GraphicsAPI.h"
#include "ShaderStorageBuffer.h"
#include "DeferredRenderer.h"

#include <iostream> 
#include <thread>
#include <chrono>

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

    void JLEngineCore::FinalizeLoading()
    {
        m_renderer->Initialize();

        m_renderer->AddVAOs(JLEngine::VAOType::STATIC, m_resourceLoader->GetGLBLoader()->GetStaticVAOs());
        m_renderer->AddVAOs(JLEngine::VAOType::JL_TRANSPARENT, m_resourceLoader->GetGLBLoader()->GetTransparentVAOs());
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