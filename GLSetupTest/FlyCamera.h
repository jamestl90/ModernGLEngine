#ifndef FLYCAMERA_H
#define FLYCAMERA_H

#include "Input.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

namespace JLEngine
{
    class FlyCamera
    {
    public:
        FlyCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
            : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(5.5f), MouseSensitivity(0.1f) 
        {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(Position, Position + Front, Up);
        }
        
        void ProcessKeyboard(GLFWwindow* window, float deltaTime) 
        {
            m_window = window;

            if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
                handleKeyPress(GLFW_KEY_W, (float)deltaTime);
            if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
                handleKeyPress(GLFW_KEY_S, (float)deltaTime);
            if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
                handleKeyPress(GLFW_KEY_A, (float)deltaTime);
            if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
                handleKeyPress(GLFW_KEY_D, (float)deltaTime);

            // std::cout << glm::to_string(Position) << std::endl;
        }

        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true) 
        {
            xoffset *= MouseSensitivity;
            yoffset *= MouseSensitivity * -1.0f;

            Yaw += xoffset;
            Pitch += yoffset;

            if (constrainPitch) {
                if (Pitch > 89.0f)
                    Pitch = 89.0f;
                if (Pitch < -89.0f)
                    Pitch = -89.0f;
            }

            updateCameraVectors();
        }

        const glm::vec3 GetPosition() { return Position; }

    private:
        void updateCameraVectors() 
        {
            glm::vec3 front;
            front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            front.y = sin(glm::radians(Pitch));
            front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            Front = glm::normalize(front);
            Right = glm::normalize(glm::cross(Front, WorldUp));
            Up = glm::normalize(glm::cross(Right, Front));
        }

        void handleKeyPress(int key, float deltaTime)
        {
            float multi = 1.0f;
            if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
                multi = 3.0f;

            float velocity = MovementSpeed * deltaTime * multi;
            if (key == GLFW_KEY_W)
                Position += Front * velocity;
            if (key == GLFW_KEY_S)
                Position -= Front * velocity;
            if (key == GLFW_KEY_A)
                Position -= Right * velocity;
            if (key == GLFW_KEY_D)
                Position += Right * velocity;
        }

        GLFWwindow* m_window;
        Input* m_input;
        glm::vec3 Position;
        glm::vec3 Front;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;

        float Yaw;
        float Pitch;

        float MovementSpeed;
        float MouseSensitivity;
    };
}
#endif
