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
            : Forward(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(5.5f), MouseSensitivity(0.1f) 
        {
            Position = position;
            WorldUp = up;
            Yaw = yaw;
            Pitch = pitch;
            updateCameraVectors();
        }

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(Position, Position + Forward, Up);
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
            if (!m_freeCursor) return;

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

        void ToggleFreeMouse(Input* input)
        {
            m_freeCursor = !m_freeCursor;
            input->SetMouseCursor(m_freeCursor ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        }

        const glm::vec3& GetForward() { return Forward; }
        const glm::vec3& GetRight() { return Right; }
        const glm::vec3& GetUp() { return Up; }

    private:
        void updateCameraVectors() 
        {
            glm::vec3 front;
            front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            front.y = sin(glm::radians(Pitch));
            front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
            Forward = glm::normalize(front);
            Right = glm::normalize(glm::cross(Forward, WorldUp));
            Up = glm::normalize(glm::cross(Right, Forward));
        }

        void handleKeyPress(int key, float deltaTime)
        {
            float multi = 1.0f;
            if (glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT))
                multi = 3.0f;

            float velocity = MovementSpeed * deltaTime * multi;
            if (key == GLFW_KEY_W)
                Position += Forward * velocity;
            if (key == GLFW_KEY_S)
                Position -= Forward * velocity;
            if (key == GLFW_KEY_A)
                Position -= Right * velocity;
            if (key == GLFW_KEY_D)
                Position += Right * velocity;
        }

        GLFWwindow* m_window;
        Input* m_input;
        glm::vec3 Position;
        glm::vec3 Forward;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        bool m_freeCursor = true;

        float Yaw;
        float Pitch;

        float MovementSpeed;
        float MouseSensitivity;
    };
}
#endif
