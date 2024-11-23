#include "JLEngine.h"
#include "Graphics.h"
#include <iostream> 
#include <thread>

namespace JLEngine
{
    JLEngineCore::JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate) :
        m_maxFrameRate(maxFrameRate),
        m_lastFrameTime(0.0), m_deltaTime(0.0), m_accumulatedTime(0.0), m_fixedUpdateRate(fixedUpdates)
    {
        // Set up fixed update interval based on target FPS
        m_fixedUpdateInterval = 1.0 / static_cast<double>(m_fixedUpdateRate);

        setVsync(false);

        m_window = std::make_unique<Window>(windowWidth, windowHeight, windowTitle, fixedUpdates);
        m_graphics = std::make_unique<Graphics>(m_window.get());
        m_textureManager = std::make_unique<TextureManager>(m_graphics.get());
        m_shaderManager = std::make_unique<ShaderManager>(m_graphics.get());
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
    }

    void JLEngineCore::setVsync(bool toggle)
    {
        if (m_window)
            glfwSwapInterval(toggle ? 1 : 0);
    }

    void JLEngineCore::run(std::function<void(double deltaTime)> logicUpdate,
        std::function<void()> render,
        std::function<void(double fixedDeltaTime)> fixedUpdate)
    {
        // The main game loop
        while (!m_window->shouldClose()) {
            auto frameStart = std::chrono::high_resolution_clock::now();
            double currentFrameTime = glfwGetTime();
            m_deltaTime = currentFrameTime - m_lastFrameTime;
            m_lastFrameTime = currentFrameTime;
            m_accumulatedTime += m_deltaTime;

            // Fixed Update (runs at the fixed time step)
            while (m_accumulatedTime >= m_fixedUpdateInterval) {
                fixedUpdate(m_fixedUpdateInterval); // Call the fixed update callback
                m_accumulatedTime -= m_fixedUpdateInterval; // Subtract the fixed time step
                m_fixedUpdateCount++;
            }

            // Logic Update (uncapped)
            logicUpdate(m_deltaTime);
            render();
            m_frameCount++;

            auto frameEnd = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> frameDuration = frameEnd - frameStart;

            // Swap buffers and poll events
            m_window->swapBuffers();
            m_window->pollEvents();

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
}