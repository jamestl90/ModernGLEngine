#ifndef JLEngine_H
#define JLEngine_H

#include <memory>
#include <chrono>  
#include "Window.h"
#include "GraphicsAPI.h"
#include "Graphics.h"
#include "Input.h"
#include "ResourceLoader.h"
#include "IMGuiManager.h"

namespace JLEngine
{
    class JLEngineCore
    {
    public:
        JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate);
        ~JLEngineCore();
        void run(std::function<void(double deltaTime)> logicUpdate,
            std::function<void(GraphicsAPI& graphics, double interpolationFactor)> render,
            std::function<void(double fixedDeltaTime)> fixedUpdate);

        GraphicsAPI* GetGraphicsAPI()          const;
        Input* GetInput()                   const;
        ResourceLoader* GetResourceLoader() const;

        void InitIMGUI() { m_imguiManager.Initialize(m_window->GetGLFWwindow()); }

    private:
        void setFixedUpdateRate(int fps);
        void setMaxFrameRate(int fps);
        void setVsync(bool toggle);

        void logPerformanceMetrics();

        IMGuiManager m_imguiManager;

        std::unique_ptr<Input> m_input;
        std::unique_ptr<Window> m_window;
        ResourceLoader* m_resourceLoader;

        // Frame timing variables
        int m_maxFrameRate;
        double m_maxFrameRateInterval;
        int m_fixedUpdateRate;
        double m_fixedUpdateInterval;
        double m_lastFrameTime;
        double m_deltaTime;
        double m_accumulatedTime;

        // Debugging and performance monitoring
        int m_frameCount = 0;
        int m_fixedUpdateCount = 0;
        std::chrono::time_point<std::chrono::steady_clock> m_debugTimer;
    };
}

#endif 