#include "Node.h"

void JLEngine::PrintNodeHierarchy(const Node* node, int depth)
{
    if (!node) return;
    std::cout << std::string(depth, '-') << " " << node->name << std::endl;

    for (const auto& mesh : node->meshes) {
        std::cout << std::string(depth + 2, ' ') << "Mesh: " << mesh->GetName() << std::endl;
    }

    for (const auto& child : node->children) {
        PrintNodeHierarchy(child.get(), depth + 2);
    }
}
