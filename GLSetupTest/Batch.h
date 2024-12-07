#ifndef BATCH_H
#define BATCH_H

#include <memory>
#include "VertexBuffers.h"
#include "Material.h"
#include "InstanceBuffer.h"

namespace JLEngine
{
    class Node;

    class Batch
    {
    public:
        Batch(
            std::shared_ptr<VertexBuffer> vb,
            std::shared_ptr<IndexBuffer> ib,
            Material* mat,
            bool instanced = false)
            : vertexBuffer(vb), indexBuffer(ib), material(mat), isInstanced(instanced) {}

        Batch(
            std::shared_ptr<VertexBuffer> vb,
            std::shared_ptr<IndexBuffer> ib,
            Material* mat)
            : vertexBuffer(vb), indexBuffer(ib), material(mat), isInstanced(false) {}

        void SetInstanceBuffer(std::shared_ptr<InstanceBuffer> ib);

        bool HasInstanceBuffer() const { return instanceBuffer.get() != nullptr; }
        bool IsValid() const
        {
            return vertexBuffer != nullptr && indexBuffer != nullptr && material != nullptr;
        }

        // Accessors
        const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const { return vertexBuffer; }
        const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return indexBuffer; }
        Material* GetMaterial() const { return material; }
        const std::shared_ptr<InstanceBuffer>& GetInstanceBuffer() const { return instanceBuffer; }
        bool IsInstanced() const { return isInstanced; }

        std::string attributesKey;
        JLEngine::Node* owner;

    private:
        // Core buffers
        std::shared_ptr<VertexBuffer> vertexBuffer;
        std::shared_ptr<IndexBuffer> indexBuffer;

        // Material for the batch
        Material* material;

        // Instance data (optional)
        std::shared_ptr<InstanceBuffer> instanceBuffer;

        // Flag to indicate if this batch supports instancing
        bool isInstanced = false;
    };
}

#endif