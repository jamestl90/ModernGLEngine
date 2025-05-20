#ifndef FLYCAMERA_H
#define FLYCAMERA_H

#include "Input.h"
#include "ViewFrustum.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_inverse.hpp>


namespace JLEngine
{
    class FlyCamera
    {
    public:
        FlyCamera(glm::vec3 position, glm::vec3 up, float yaw, float pitch);

        glm::mat4 GetViewMatrix() const
        {
            return glm::lookAt(Position, Position + Forward, Up);
        }
        
        void ProcessKeyboard(GLFWwindow* window, float deltaTime);

        void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

        glm::vec3 ScreenToWorldRay(glm::vec2 ndc, const glm::mat4& proj) const;

        const glm::vec3 GetPosition() { return Position; }

        void ToggleFreeMouse(Input* input);

        ViewFrustum GetViewFrustum() { return m_frustum; }

        const glm::vec3& GetForward() { return Forward; }
        const glm::vec3& GetRight() { return Right; }
        const glm::vec3& GetUp() { return Up; }

    private:
        void updateCameraVectors();

        void handleKeyPress(int key, float deltaTime);

        GLFWwindow* m_window;
        Input* m_input;
        glm::vec3 Position;
        glm::vec3 Forward;
        glm::vec3 Up;
        glm::vec3 Right;
        glm::vec3 WorldUp;
        bool m_freeCursor = true;

        ViewFrustum m_frustum{};

        float Yaw;
        float Pitch;

        float MovementSpeed;
        float MouseSensitivity;
    };
}
#endif
