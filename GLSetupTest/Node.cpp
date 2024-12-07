#include "Node.h"

void JLEngine::PrintNodeHierarchy(const Node* node, int depth)
{
    if (!node) return;
    std::cout << std::string(depth, '-') << " " << node->name << std::endl;

    std::cout << std::string(depth + 2, ' ') << "Mesh: " << node->mesh->GetName() << std::endl;

    for (const auto& child : node->children) {
        PrintNodeHierarchy(child.get(), depth + 2);
    }
}
