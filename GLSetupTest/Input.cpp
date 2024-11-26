#include "Input.h"

namespace JLEngine
{
    // Static variable initialization
    map<int, int> Input::m_mapKey = {};
    map<int, int> Input::m_mouseKey = {};
    double Input::m_mouseX = 0;
    double Input::m_mouseY = 0;
    double Input::m_deltaX = 0;
    double Input::m_deltaY = 0;
    double Input::m_lastMouseX = 0;
    double Input::m_lastMouseY = 0;
    bool Input::m_firstMouse = true;
    std::function<void(int button, int action, int mods)> Input::m_mouseCallback = nullptr;
    std::function<void(double x, double y)> Input::m_mouseMoveCallback = nullptr;
    std::function<void(int key, int scancode, int action, int mods)> Input::m_keyboardCallback = nullptr;

    Input::Input(Window* window) : m_window(window)
    {
        m_glfwWindow = window->GetGLFWwindow();
    }

    Input::~Input()
    {
        m_mapKey.clear();
    }

    void Input::SetKey(int key, int state)
    {
        m_mapKey[key] = state;
    }

    void Input::KeyboardInput(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        m_mapKey[key] = action;

        m_keyboardCallback(key, scancode, action, mods);
    }

    void Input::GetMouseDelta(double& x, double& y)
    {
        x = m_deltaX;
        y = m_deltaY;
    }

    void Input::MouseButton(GLFWwindow* window, int button, int state, int mods)
    {
        m_mouseKey[button] = state;

        if (m_mouseCallback != nullptr)
        {
            m_mouseCallback(button, state, mods);
        }
    }

    void Input::MousePos(GLFWwindow* window, double x, double y)
    {
        m_mouseX = x;
        m_mouseY = y;

        if (m_firstMouse)
        {
            m_lastMouseX = m_mouseX;
            m_lastMouseY = m_mouseY;
            m_firstMouse = false;
        }

        m_deltaX = m_mouseX - m_lastMouseX;
        m_deltaY = m_mouseY - m_lastMouseY;

        m_lastMouseX = m_mouseX;
        m_lastMouseY = m_mouseY;

        if (m_mouseMoveCallback != nullptr)
        {
            m_mouseMoveCallback(x, y);
        }
    }

    int Input::IsKeyDown(int key)
    {
        return m_mapKey[key];
    }

    void Input::SetStickyKeys(bool flag)
    {
        glfwSetInputMode(m_glfwWindow, GLFW_STICKY_KEYS, flag ? GLFW_TRUE : GLFW_FALSE);
    }

    void Input::SetMouseStickyKeys(bool flag)
    {
        glfwSetInputMode(m_glfwWindow, GLFW_STICKY_MOUSE_BUTTONS, flag ? GLFW_TRUE : GLFW_FALSE);
    }

    bool Input::SetRawMouseMotion(bool flag)
    {
        if (!glfwRawMouseMotionSupported())
            return false;
        glfwSetInputMode(m_glfwWindow, GLFW_RAW_MOUSE_MOTION, flag ? GLFW_TRUE : GLFW_FALSE);
        return true;
    }

    void Input::SetMouseCursor(int value)
    {
        glfwSetInputMode(m_glfwWindow, GLFW_CURSOR, value);
    }

    double Input::GetMouseX()
    {
        return m_mouseX;
    }

    double Input::GetMouseY()
    {
        return m_mouseY;
    }

    int Input::IsMouseDown(int button)
    {
        return m_mouseKey[button];
    }

    void Input::SetMousePos(double x, double y)
    {
        glfwSetCursorPos(m_glfwWindow, x, y);

        // Update mouse position in case callback doesn't trigger
        m_mouseX = x;
        m_mouseY = y;
    }

    void Input::SetKeyboardCallback(std::function<void(int key, int scancode, int action, int mods)> callback)
    {
        m_keyboardCallback = callback;
        glfwSetKeyCallback(m_glfwWindow, &Input::KeyboardInput);
    }

    void Input::SetMouseCallback(std::function<void(int button, int action, int mods)> callback)
    {
        m_mouseCallback = callback;
        glfwSetMouseButtonCallback(m_glfwWindow, &Input::MouseButton);
    }

    void Input::SetMouseMoveCallback(std::function<void(double x, double y)> callback)
    {
        m_mouseMoveCallback = callback;
        glfwSetCursorPosCallback(m_glfwWindow, &Input::MousePos);
    }
}
