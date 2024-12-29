#ifndef MAIN_APP_H
#define MAIN_APP_H

#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>     
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "JLEngine.h"
#include "FlyCamera.h"
#include "Mesh.h"
#include "Node.h"
#include "Geometry.h"
#include "DeferredRenderer.h"
#include "Material.h"
#include "TextureReader.h"
#include "TextureWriter.h"
#include "CubemapBaker.h"

extern JLEngine::FlyCamera* flyCamera;
extern JLEngine::Input* input;
extern JLEngine::Mesh* cubeMesh;
extern JLEngine::Mesh* planeMesh;
extern JLEngine::Mesh* sphereMesh;
extern std::shared_ptr<JLEngine::Node> sceneRoot;
extern std::shared_ptr<JLEngine::Node> cardinalDirections;
extern JLEngine::ShaderProgram* meshShader;
extern JLEngine::ShaderProgram* basicLit;
extern JLEngine::DeferredRenderer* m_defRenderer;
extern GLFWwindow* window;

void gameRender(JLEngine::GraphicsAPI& graphics, double interpolationFactor);
void gameLogicUpdate(double deltaTime);
void fixedUpdate(double fixedTimeDelta);
void KeyboardCallback(int key, int scancode, int action, int mods);
void MouseCallback(int button, int action, int mods);
void MouseMoveCallback(double x, double y);
void WindowResizeCallback(int width, int height);
int MainApp(std::string assetFolder);

#endif