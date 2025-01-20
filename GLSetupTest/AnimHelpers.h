#ifndef ANIM_HELPERS_H
#define ANIM_HELPERS_H

#include "AnimData.h"

namespace JLEngine
{
	class AnimHelpers
	{
    public:
		static void ComputeGlobalTransforms(const Skeleton& skeleton, std::vector<glm::mat4>& globalTransforms)
        {
            globalTransforms.resize(skeleton.joints.size());

            for (size_t i = 0; i < skeleton.joints.size(); ++i)
            {
                const auto& joint = skeleton.joints[i];

                if (joint.parentIndex >= 0)
                {
                    const glm::mat4& parentTransform = globalTransforms[joint.parentIndex];
                    globalTransforms[i] = parentTransform * joint.localTransform;
                }
                else
                {
                    globalTransforms[i] = joint.localTransform;
                }
            }
        }

        static void ComputeJointMatrices(const std::vector<glm::mat4>&  globalTransforms,
                                         const std::vector<glm::mat4>&  inverseBindMatrices,
                                         std::vector<glm::mat4>&        jointMatrices)
        {
            jointMatrices.resize(globalTransforms.size());

            for (size_t i = 0; i < globalTransforms.size(); ++i)
            {
                jointMatrices[i] = globalTransforms[i] * inverseBindMatrices[i];
            }
        }
	};
}

#endif