#ifndef BATCH_H
#define BATCH_H

#include <memory>
#include "VertexBuffers.h"
#include "Material.h"

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

        bool IsValid() const
        {
            return vertexBuffer != nullptr && indexBuffer != nullptr && material != nullptr;
        }

        // Accessors
        const std::shared_ptr<VertexBuffer>& GetVertexBuffer() const { return vertexBuffer; }
        const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const { return indexBuffer; }
        Material* GetMaterial() const { return material; }

        bool IsInstanced() const { return isInstanced; }
        void SetMaterial(Material* mat) { material = mat; }

        VertexAttribKey attributesKey;

    private:
        // Core buffers
        std::shared_ptr<VertexBuffer> vertexBuffer;
        std::shared_ptr<IndexBuffer> indexBuffer;

        // Material for the batch
        Material* material;

        // Flag to indicate if this batch supports instancing
        bool isInstanced = false;
    };
}

#endif