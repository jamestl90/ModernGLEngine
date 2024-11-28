#include <string>
#include <vector>
#include <memory> // For smart pointers
#include <glm/glm.hpp> // For transformation matrices and vectors
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"

namespace JLEngine
{
    // Node class representing a single entity in a scene graph
    class Node
    {
    public:
        // Constructor
        Node(const std::string& name = "")
            : name(name), parent(nullptr), translation(0.0f), rotation(1.0f, 1.0f, 1.0f, 1.0f), scale(1.0f) {}

        std::string name;

        glm::vec3 translation;       
        glm::quat rotation;          
        glm::vec3 scale;             


        std::vector<std::shared_ptr<Node>> children;

        Node* parent;

        glm::mat4 GetGlobalTransform() const
        {
            glm::mat4 localTransform = GetLocalTransform();
            if (parent)
            {
                return parent->GetGlobalTransform() * localTransform;
            }
            return localTransform;
        }

        void AddChild(const std::shared_ptr<Node>& child)
        {
            child->parent = this; 
            children.push_back(child);
        }

    private:

        glm::mat4 GetLocalTransform() const
        {
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 rotationMatrix = glm::toMat4(rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
            return translationMatrix * rotationMatrix * scaleMatrix;
        }
    };
}