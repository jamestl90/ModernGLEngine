#include "Window.h"
#include <iostream>
#include <glad/glad.h> // Include GLAD for OpenGL function loading

namespace JLEngine
{
    Window::Window(int width, int height, const char* title, int targetFPS, int maxFrameRate)
        : m_width(width), m_height(height), m_window(nullptr)
    {
        // Initialize GLFW and create the window in the constructor
        if (!init(title)) {
            std::cerr << "Failed to initialize window!" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    Window::~Window()
    {
        // Clean up GLFW resources
        if (m_window) {
            glfwDestroyWindow(m_window);
        }
        glfwTerminate();
    }

    bool Window::init(const char* name)
    {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "GLFW initialization failed!" << std::endl;
            return false;
        }

        int major, minor, revision;
        glfwGetVersion(&major, &minor, &revision);
        std::cout << "GLFW Version: " << major << "." << minor << "." << revision << std::endl;

        glfwWindowHint(GLFW_SAMPLES, 4); // msaa samples
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_DEPTH_BITS, 24);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_DOUBLEBUFFER, GLFW_TRUE);
        //glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

        // Create a GLFW windowed mode window and its OpenGL context
        m_window = glfwCreateWindow(m_width, m_height, name, nullptr, nullptr);
        if (!m_window) 
        {
            std::cerr << "Failed to create GLFW window!" << std::endl;
            glfwTerminate();
            return false;
        }

        // Make the OpenGL context current
        glfwMakeContextCurrent(m_window);

        // Initialize GLAD (OpenGL function loader)
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) 
        {
            std::cerr << "Failed to initialize OpenGL loader!" << std::endl;
            return false;
        }

        std::cout << "OpenGL " << glGetString(GL_VERSION) << " initialized!" << std::endl;

        // Set the frame buffer size callback for resizing the window
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) 
        {
            glViewport(0, 0, width, height);
        });

        // Set up V-Sync (we can turn it off to allow uncapped rendering if needed)
        glfwSwapInterval(1);  // Enable V-Sync by default, set to 0 for uncapped

        return true;
    }

    int Window::GetWidth() const
    {
        return m_width;
    }

    int Window::GetHeight() const
    {
        return m_height;
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_window);
    }

    void Window::SwapBuffers()
    {
        glfwSwapBuffers(m_window);
    }

    void Window::PollEvents()
    {
        glfwPollEvents();
    }

    void Window::WaitEventsTimeout(double timeout)
    {
        glfwWaitEventsTimeout(timeout);
    }
}