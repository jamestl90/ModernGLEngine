#include "JLEngine.h"
#include "Graphics.h"
#include <iostream> 
#include <thread>
#include <chrono>

namespace JLEngine
{
    JLEngineCore::JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate) :
        m_maxFrameRate(maxFrameRate), m_maxFrameRateInterval(0),
        m_lastFrameTime(0.0), m_deltaTime(0.0), m_accumulatedTime(0.0), m_fixedUpdateRate(fixedUpdates) 
    {
        // Set up fixed update interval based on target FPS
        m_fixedUpdateInterval = 1.0 / static_cast<double>(m_fixedUpdateRate);
        m_maxFrameRateInterval = 1.0 / static_cast<double>(m_maxFrameRate);

        m_window = std::make_unique<Window>(windowWidth, windowHeight, windowTitle, fixedUpdates);
        m_input = std::make_unique<Input>(m_window.get());
        m_graphics = std::make_unique<Graphics>(m_window.get());
        m_textureManager = std::make_unique<TextureManager>(m_graphics.get());
        m_shaderManager = std::make_unique<ShaderManager>(m_graphics.get());
        m_materialManager = std::make_unique<MaterialManager>(m_graphics.get());
        m_renderTargetManager = std::make_unique<RenderTargetManager>(m_graphics.get());
        m_meshManager = std::make_unique<MeshManager>(m_graphics.get());

        m_assetLoader = std::make_unique<AssetLoader>(
            m_shaderManager.get(),
            m_meshManager.get(),
            m_materialManager.get(),
            m_textureManager.get()
        );

        m_input->SetRawMouseMotion(true);
        setVsync(true);
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
        std::function<void(Graphics& graphics, double interpolationFactor)> render,
        std::function<void(double fixedDeltaTime)> fixedUpdate)
    {
        auto lastTime = std::chrono::high_resolution_clock::now();

        // The main game loop
        while (!m_window->ShouldClose()) 
        {
            auto frameStart = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> frameDuration = frameStart - lastTime;
            lastTime = frameStart;
            m_deltaTime = frameDuration.count(); // Delta time in seconds
            m_accumulatedTime += m_deltaTime;

            logicUpdate(m_deltaTime);

            // Fixed Update (runs at the fixed time step)
            while (m_accumulatedTime >= m_fixedUpdateInterval)
            {
                fixedUpdate(m_fixedUpdateInterval); 
                m_accumulatedTime -= m_fixedUpdateInterval; 
                m_fixedUpdateCount++;
            }

            double interpolationFactor = m_accumulatedTime / m_fixedUpdateInterval;
            render(*m_graphics, interpolationFactor); 
            m_window->SwapBuffers();
            m_frameCount++;
            m_window->PollEvents();

            logPerformanceMetrics();
        }
    }

    Graphics* JLEngineCore::GetGraphics() const
    {
        return m_graphics.get();
    }

    ShaderManager* JLEngineCore::GetShaderManager() const
    {
        return m_shaderManager.get();
    }

    TextureManager* JLEngineCore::GetTextureManager() const
    {
        return m_textureManager.get();
    }

    MeshManager* JLEngineCore::GetMeshManager() const
    {
        return m_meshManager.get();
    }

    MaterialManager* JLEngineCore::GetMaterialManager() const
    {
        return m_materialManager.get();
    }

    Input* JLEngineCore::GetInput() const
    {
        return m_input.get();
    }

    RenderTargetManager* JLEngineCore::GetRenderTargetManager() const
    {
        return m_renderTargetManager.get();
    }

    AssetLoader* JLEngineCore::GetAssetLoader() const
    {
        return m_assetLoader.get();
    }
}