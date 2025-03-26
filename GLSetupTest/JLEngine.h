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
#include "Im3dManager.h"
#include "FlyCamera.h"

namespace JLEngine
{
    class DeferredRenderer;

    class JLEngineCore
    {
    public:
        JLEngineCore(int windowWidth, int windowHeight, const char* windowTitle, int fixedUpdates, int maxFrameRate, const std::string& assetFolder);
        ~JLEngineCore();
        void run(std::function<void(double deltaTime)> logicUpdate,
            std::function<void(GraphicsAPI& graphics, double interpolationFactor)> render,
            std::function<void(double fixedDeltaTime)> fixedUpdate);

        std::shared_ptr<Node> Load(const std::string& fileName);
        std::shared_ptr<Node> LoadAndAttachToRoot(const std::string& fileName);
        std::shared_ptr<Node> LoadAndAttachToRoot(const std::string& fileName, const glm::vec3& pos);
        std::shared_ptr<Node> LoadAndAttachToRoot(const std::string& fileName, const glm::vec3& pos, const glm::quat& rotation, const glm::vec3& scale);
        std::shared_ptr<Node> LoadAndAttachToRoot(const std::string& fileName, const glm::mat4& transform);
        std::shared_ptr<Node> MakeInstanceOf(std::shared_ptr<Node>& existingNode, const glm::vec3& pos, bool attachToRoot);        
        void FinalizeLoading();

        GraphicsAPI* GetGraphicsAPI()           const;
        Input* GetInput()                       const;
        ResourceLoader* GetResourceLoader()     const;
        DeferredRenderer* GetRenderer()         const;

        FlyCamera* GetFlyCamera();

        void InitIMGUI();

    private:
        void setFixedUpdateRate(int fps);
        void setMaxFrameRate(int fps);
        void setVsync(bool toggle);

        void CloneChildNodes(const std::shared_ptr<Node>& originalNode, const std::shared_ptr<Node>& clonedNode);

        IMGuiManager m_imguiManager;
        IM3DManager m_im3dManager;

        std::unique_ptr<Input> m_input;
        std::unique_ptr<Window> m_window;
        ResourceLoader* m_resourceLoader;

        std::string m_assetFolder;
        DeferredRenderer* m_renderer;

        FlyCamera* m_flyCamera;

        // Frame timing variables
        int m_maxFrameRate;
        double m_maxFrameRateInterval;
        int m_fixedUpdateRate;
        double m_fixedUpdateInterval;
        double m_deltaTime;
        double m_accumulatedTime;

        // Debugging and performance monitoring
        int m_frameCount = 0;
        int m_fixedUpdateCount = 0;
        std::chrono::time_point<std::chrono::steady_clock> m_debugTimer;
    };
}

#endif 