#ifndef WINDOW_H
#define WINDOW_H

#include <glad/glad.h> 
#include <GLFW/glfw3.h>
#include <functional>

class Window
{
public:
    // Constructor to create the window
    Window(int width, int height, const char* title, int targetFPS = 60, int maxFrameRate = 0);
    ~Window();

    bool init(const char* name);
    int getWidth() const;
    int getHeight() const;
    bool shouldClose() const;
    void swapBuffers();
    void pollEvents();

private:
    GLFWwindow* m_window;
    int m_width, m_height; 
};

#endif