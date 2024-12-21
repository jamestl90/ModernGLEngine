#ifndef IMGUI_MANAGER_H
#define IMGUI_MANAGER_H

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

namespace JLEngine
{
    class IMGuiManager 
    {
    public:
        void Initialize(GLFWwindow* window);

        void BeginFrame();

        void EndFrame();

        void Shutdown();

        bool Initialised() { return m_initialised; }

    private:
        bool m_initialised = false;
    };

}

#endif