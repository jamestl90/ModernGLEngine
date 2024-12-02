#ifndef JLEngine_H
#define JLEngine_H

#include <memory>
#include <chrono>  
#include "Window.h"
#include "Graphics.h"
#include "TextureManager.h"
#include "ShaderManager.h"
#include "Input.h"
#include "MeshManager.h"
#include "MaterialManager.h"
#include "RenderTargetManager.h"
#include "AssetLoader.h"

namespace JLEngine
{
    class JLEngineCore
    {
    public:
        JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate);
        void run(std::function<void(double deltaTime)> logicUpdate,
            std::function<void(Graphics& graphics, double interpolationFactor)> render,
            std::function<void(double fixedDeltaTime)> fixedUpdate);

        Graphics*            GetGraphics()            const;
        ShaderManager*       GetShaderManager()       const;
        TextureManager*      GetTextureManager()      const;
        MeshManager*         GetMeshManager()         const;
        MaterialManager*     GetMaterialManager()     const;
        Input*               GetInput()               const;
        RenderTargetManager* GetRenderTargetManager() const;
        AssetLoader*         GetAssetLoader()         const;

    private:
        void setFixedUpdateRate(int fps);
        void setMaxFrameRate(int fps);
        void setVsync(bool toggle);

        void logPerformanceMetrics();

        std::unique_ptr<Input> m_input;
        std::unique_ptr<Window> m_window;
        std::unique_ptr<Graphics> m_graphics;
        std::unique_ptr<TextureManager> m_textureManager;
        std::unique_ptr<ShaderManager> m_shaderManager;
        std::unique_ptr<MeshManager> m_meshManager;
        std::unique_ptr<MaterialManager> m_materialManager;
        std::unique_ptr<RenderTargetManager> m_renderTargetManager;
        std::unique_ptr<AssetLoader> m_assetLoader;

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