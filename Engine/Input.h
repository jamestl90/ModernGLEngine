#ifndef INPUT_H
#define INPUT_H

#include <map>
#include <iostream>
#include "Window.h"
#include <functional>

#include <glm/glm.hpp>

using std::map;

struct GLFWwindow;

namespace JLEngine
{
    class Input
    {
    public:
        Input(Window* window);
        ~Input();

        // Keyboard and mouse functions
        int IsKeyDown(int key);
        void SetStickyKeys(bool flag);
        void SetKey(int key, int state);
        void SetMouseStickyKeys(bool flag);
        bool SetRawMouseMotion(bool flag);
        void SetMouseCursor(int value); // GLFW_CURSOR_NORMAL, GLFW_CURSOR_HIDDEN, GLFW_CURSOR_DISABLED
        int GetMouseCursor();
        double GetMouseX();
        double GetMouseY();
        int IsMouseDown(int button);
        void SetMousePos(double  x, double  y); 
        void GetMouseDelta(double& x, double& y);

        glm::vec2 GetNDC()
        {
            auto screenSize = glm::vec2(m_window->GetWidth(), m_window->GetHeight());
            auto mouseX = GetMouseX();
            auto mouseY = GetMouseY();

            glm::vec2 ndc{};
            ndc.x = ((float)mouseX / screenSize.x) * 2.0f - 1.0f;
            ndc.y = ((float)mouseY / screenSize.y) * 2.0f + 1.0f;
            return ndc;
        }

        // callback functions
        void SetKeyboardCallback(std::function<void(int key, int scancode, int action, int mods)> callback);
        void SetMouseCallback(std::function<void(int button, int action, int mods)> callback);
        void SetMouseMoveCallback(std::function<void(double x, double y)> callback);

    private:
        // Static GLFW callback handlers
        static void KeyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods);
        static void MousePos(GLFWwindow* window, double x, double y);
        static void MouseButton(GLFWwindow* window, int button, int state, int mods);

        // Member variables
        Window* m_window;
        GLFWwindow* m_glfwWindow;

        static bool m_firstMouse;
        static double m_deltaX;
        static double m_deltaY;
        static double m_mouseX;
        static double m_mouseY;
        static double m_lastMouseX;
        static double m_lastMouseY;
        static map<int, int> m_mapKey;
        static map<int, int> m_mouseKey;

        // Callback pointers
        static std::function<void(double x, double y)> m_mouseMoveCallback;
        static std::function<void(int button, int action, int mods)> m_mouseCallback;
        static std::function<void(int key, int scancode, int action, int mods)> m_keyboardCallback;
    };
}

#endif
