#ifndef NODE_H
#define NODE_H

#include <string>
#include <vector>
#include <memory> // For smart pointers
#include <glm/glm.hpp> // For transformation matrices and vectors
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Mesh.h"
#include "Light.h"

namespace JLEngine
{
    enum class NodeTag 
    {
        Default,    // General-purpose node
        Mesh,       // Node with a mesh
        Camera,     // Node with a camera
        Light,      // Node with a light
        SceneRoot
    };

    // Node class representing a single entity in a scene graph
    class Node : public std::enable_shared_from_this<Node>
    {
    public:
        // Constructor
        Node(const std::string& name = "", NodeTag nodeTag = NodeTag::Mesh)
            : name(name), tag(nodeTag), useMatrix(false),
            localMatrix(glm::mat4(1.0)), translation(0.0f),
            rotation(1.0f, 0.0f, 0.0f, 0.0f), scale(1.0f), mesh(0) 
        {
            UpdateTransforms();
        }

        std::string name;
        NodeTag tag;

        void SetTranslationRotation(const glm::vec3& newTranslation, const glm::quat& newRotation)
        {
            translation = newTranslation;
            rotation = newRotation;
            UpdateHierarchy();
        }

        void SetTRS(const glm::vec3& newTranslation, const glm::quat& newRotation, const glm::vec3& newScale)
        {
            translation = newTranslation;
            rotation = newRotation;
            scale = newScale;
            UpdateHierarchy();
        }

        void SetTranslation(const glm::vec3& newTranslation)
        {
            translation = newTranslation;
            UpdateHierarchy();
        }

        const glm::vec3& GetTranslation() const
        {
            return translation;
        }

        // Rotation
        void SetRotation(const glm::quat& newRotation)
        {
            rotation = newRotation;
            UpdateHierarchy();
        }

        const glm::quat& GetRotation() const
        {
            return rotation;
        }

        // Scale
        void SetScale(const glm::vec3& newScale)
        {
            scale = newScale;
            UpdateHierarchy();
        }

        const glm::vec3& GetScale() const
        {
            return scale;
        }

        glm::mat4 localMatrix;
        bool useMatrix;

        Light* m_lightData;
        Mesh* mesh;
        std::vector<std::shared_ptr<Node>> children;

        std::weak_ptr<Node> parent;

        glm::mat4 globalTransform; // Cached global transform

        glm::mat4 GetLocalTransform() const
        {
            if (useMatrix)
            {
                return localMatrix;
            }
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translation);
            glm::mat4 rotationMatrix = glm::toMat4(rotation);
            glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
            return translationMatrix * rotationMatrix * scaleMatrix;
        }

        glm::mat4 GetGlobalTransform() const
        {
            return globalTransform; // Use cached value
        }

        void UpdateTransforms(const glm::mat4& parentTransform = glm::mat4(1.0f))
        {
            // Compute this node's global transform
            globalTransform = parentTransform * GetLocalTransform();

            // Update children
            for (auto& child : children)
            {
                child->UpdateTransforms(globalTransform);
            }
        }

        // You do not want to call this before a node has been attached to the scene root 
        // So when setting up a new node just set its scale/translation/rotation directly without 
        // using the SetX methods which call UpdateHierarchy
        void UpdateHierarchy()
        {
            if (auto parentPtr = parent.lock())
            {
                UpdateTransforms(parentPtr->GetGlobalTransform());
            }
            else
            {
                UpdateTransforms(glm::mat4(1.0f));
            }
        }

        void AddChild(std::shared_ptr<Node>& child)
        {
            child->parent = shared_from_this();
            child->UpdateTransforms(globalTransform);
            children.push_back(child);
        }

        void SetTag(NodeTag newTag)
        {
            tag = newTag;
        }

        NodeTag GetTag() const
        {
            return tag;
        }

        glm::vec3 translation;
        glm::quat rotation;
        glm::vec3 scale;
    private:
    };

    void PrintNodeHierarchy(const Node* node, int depth = 0);
}

#endif