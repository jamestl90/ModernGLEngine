#ifndef WINDOW_H
#define WINDOW_H

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <functional>

struct GLFWwindow;

namespace JLEngine
{
    class Window
    {
    public:
        // Constructor to create the window
        Window(int width, int height, const char* title, int targetFPS = 60, int maxFrameRate = 0);
        ~Window();

        bool init(const char* name);
        int GetWidth() const;
        int GetHeight() const;
        bool ShouldClose() const;
        void SwapBuffers();
        void PollEvents();
        void WaitEventsTimeout(double timeout);

        GLFWwindow* GetGLFWwindow() { return m_window; }

        void SetResizeCallback(std::function<void(int, int)> callback);

        bool SRGBCapable() const;
        int GetVersionMajor() { return m_major; }
        int GetVersionMinor() { return m_minor; }
        int GetRevision() { return m_revision; }

    private:

        static void PrintStackTrace();
        static void GLDebugCallback(unsigned int source, unsigned int type, unsigned int id,
            unsigned int severity, int length,
            const char* message, const void* userParam);

        GLFWwindow* m_window;
        int m_width, m_height;

        std::function<void(int, int)> m_resizeCallback;
        static void FramebufferResizeCallback(GLFWwindow* window, int width, int height);

        int m_major;
        int m_minor;
        int m_revision;
        int m_defaultFramebufferEncoding;
    };
}
#endif