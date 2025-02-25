#include "imgui.h"
#include "Node.h"

namespace JLEngine
{
    std::string GetSubmeshFlagsString(uint32_t flags)
    {
        std::string flagStr;

        if (flags == 0)
        {
            flagStr = "NONE";
        }
        else
        {
            if (flags & SubmeshFlags::STATIC) flagStr += "STATIC, ";
            if (flags & SubmeshFlags::SKINNED) flagStr += "SKINNED, ";
            if (flags & SubmeshFlags::INSTANCED) flagStr += "INSTANCED, ";
            if (flags & SubmeshFlags::ANIMATED) flagStr += "ANIMATED, ";
            if (flags & SubmeshFlags::USES_TRANSPARENCY) flagStr += "USES_TRANSPARENCY, ";
            if (flags & SubmeshFlags::USE_SKELETON) flagStr += "USE_SKELETON, ";

            // Remove trailing comma and space
            if (!flagStr.empty())
            {
                flagStr.pop_back();
                flagStr.pop_back();
            }
        }

        return flagStr;
    }

    void ShowNodeHierarchy(Node* node)
    {
        if (!node) return;

        ImGui::Begin("Node Hierarchy");

        std::function<void(Node*)> DisplayNode = [&](Node* currentNode)
            {
                if (!currentNode) return;

                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;

                if (currentNode->children.empty())
                {
                    flags |= ImGuiTreeNodeFlags_Leaf;
                }

                std::string nodeLabel = currentNode->name;
                if (currentNode->IsAnimated)
                    nodeLabel += " (Animated) ";
                if (currentNode->tag == NodeTag::Mesh && currentNode->mesh)
                {
                    auto submeshes = currentNode->mesh->GetSubmeshes();
                    if (!submeshes.empty())
                    {
                        std::string flagsText = GetSubmeshFlagsString(submeshes[0].flags);
                        nodeLabel += " [SubmeshFlags: " + flagsText + "]";
                    }
                }

                bool nodeOpen = ImGui::TreeNodeEx(nodeLabel.c_str(), flags);

                if (nodeOpen)
                {
                    for (auto& child : currentNode->children)
                    {
                        DisplayNode(child.get());
                    }
                    ImGui::TreePop();
                }
            };

        DisplayNode(node);

        ImGui::End();
    }
}
