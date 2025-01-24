#ifndef ANIM_HELPERS_H
#define ANIM_HELPERS_H

#include "AnimData.h"

namespace JLEngine
{
	class AnimHelpers
	{
    public:
		static void ComputeGlobalTransforms(const Skeleton& skeleton, const std::vector<glm::mat4>& nodeTransforms, std::vector<glm::mat4>& globalTransforms)
        {
            globalTransforms.resize(skeleton.joints.size());

            for (size_t i = 0; i < skeleton.joints.size(); ++i)
            {
                const auto& joint = skeleton.joints[i];
                const auto& localTransform = nodeTransforms[i];

                if (joint.parentIndex >= 0)
                {
                    const glm::mat4& parentTransform = globalTransforms[joint.parentIndex];
                    globalTransforms[i] = parentTransform * localTransform;
                }
                else
                {
                    globalTransforms[i] = localTransform;
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

        static void EvaluateAnimation(Animation& animation, float currTime, std::vector<glm::mat4>& nodeTransforms)
        {
            const auto& channels = animation.GetChannels();
            const auto& samplers = animation.GetSamplers();

            std::vector<glm::vec3> translations(nodeTransforms.size(), glm::vec3(0.0f));
            std::vector<glm::quat> rotations(nodeTransforms.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            std::vector<glm::vec3> scales(nodeTransforms.size(), glm::vec3(1.0f));

            for (const auto& channel : channels)
            {
                const auto& sampler = samplers[channel.GetSamplerIndex()];
                const auto& outputValues = sampler.GetValues();
                const auto& times = sampler.GetTimes();

                auto it = std::lower_bound(times.begin(), times.end(), currTime);

                if (currTime >= times.back())
                {
                    ApplyKeyframe(channel, outputValues.back(), translations, rotations, scales);
                }
                else if (it == times.begin())
                {
                    ApplyKeyframe(channel, outputValues.front(), translations, rotations, scales);
                }
                else
                {
                    size_t i = std::distance(times.begin(), it);
                    float factor = (currTime - times[i - 1]) / (times[i] - times[i - 1]);
                    InterpolateKeyframes(channel, outputValues[i], outputValues[i], factor, translations, rotations, scales);
                }
            }

            for (size_t i = 0; i < nodeTransforms.size(); ++i)
            {
                nodeTransforms[i] = glm::translate(glm::mat4(1.0f), translations[i]) *
                    glm::mat4_cast(rotations[i]) *
                    glm::scale(glm::mat4(1.0f), scales[i]);
            }
        }

        static void InterpolateKeyframes(const AnimationChannel& channel, const glm::vec4& prevValue, const glm::vec4& nextValue, float factor,
            std::vector<glm::vec3>& translations,
            std::vector<glm::quat>& rotations,
            std::vector<glm::vec3>& scales)
        {
            int targetNode = channel.GetTargetNode();
            TargetPath targetPath = channel.GetTargetPath();

            if (targetPath == TargetPath::TRANSLATION)
            {
                translations[targetNode] = glm::mix(glm::vec3(prevValue), glm::vec3(nextValue), factor);
            }
            else if (targetPath == TargetPath::ROTATION)
            {
                glm::quat q1(prevValue.w, prevValue.x, prevValue.y, prevValue.z);
                glm::quat q2(nextValue.w, nextValue.x, nextValue.y, nextValue.z);

                if (glm::dot(q1, q2) < 0.0f)
                {
                    q2 = -q2; 
                }

                rotations[targetNode] = glm::normalize(glm::slerp(q1, q2, factor));
            }
            else if (targetPath == TargetPath::SCALE)
            {
                scales[targetNode] = glm::mix(glm::vec3(prevValue), glm::vec3(nextValue), factor);
            }
        }

        static void ApplyKeyframe(const AnimationChannel& channel, const glm::vec4& keyframeValue,
            std::vector<glm::vec3>& translations,
            std::vector<glm::quat>& rotations,
            std::vector<glm::vec3>& scales)
        {
            int targetNode = channel.GetTargetNode();
            TargetPath targetPath = channel.GetTargetPath();

            if (targetPath == TargetPath::TRANSLATION)
            {
                translations[targetNode] = glm::vec3(keyframeValue);
            }
            else if (targetPath == TargetPath::ROTATION)
            {
                rotations[targetNode] = glm::quat(keyframeValue.w, keyframeValue.x, keyframeValue.y, keyframeValue.z);
            }
            else if (targetPath == TargetPath::SCALE)
            {
                scales[targetNode] = glm::vec3(keyframeValue);
            }
        }
	};
}

#endif