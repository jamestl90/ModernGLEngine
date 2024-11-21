#ifndef RENDERER_H
#define RENDERER_H

#include <glad/glad.h>
#include <string>
#include <vector>
#include <memory>

class Renderer {
public:
    Renderer();
    ~Renderer();

    void initialize();
    void renderScene(); // To be called every frame

private:
    void initializeShaders();
    void initializeBuffers();

    GLuint m_shaderProgram;
    GLuint m_VAO, m_VBO;
};

#endif // RENDERER_H