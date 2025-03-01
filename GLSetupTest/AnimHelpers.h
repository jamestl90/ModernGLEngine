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

        static void EvaluateAnimation(Animation& animation, 
            float currTime,
            std::vector<glm::mat4>& nodeTransforms,
            const std::vector<size_t>& keyframeIndices)
        {
            const auto& channels = animation.GetChannels();
            const auto& samplers = animation.GetSamplers();

            std::vector<glm::vec3> translations(nodeTransforms.size(), glm::vec3(0.0f));
            std::vector<glm::quat> rotations(nodeTransforms.size(), glm::quat(1.0f, 0.0f, 0.0f, 0.0f));
            std::vector<glm::vec3> scales(nodeTransforms.size(), glm::vec3(1.0f));

            for (size_t channelIndex = 0; channelIndex < channels.size(); ++channelIndex)
            {
                const auto& channel = channels[channelIndex];
                const auto& sampler = samplers[channel.GetSamplerIndex()];
                const auto& outputValues = sampler.GetValues();
                const auto& inputTimes = sampler.GetTimes();

                size_t currentIndex = keyframeIndices[channelIndex];
                size_t nextIndex = (currentIndex + 1 < inputTimes.size()) ? currentIndex + 1 : 0;

                float timeStart = inputTimes[currentIndex];
                float timeEnd = inputTimes[nextIndex];

                if (nextIndex == 0)
                {
                    timeEnd = inputTimes[nextIndex] + animation.GetDuration();
                }

                float factor = (timeEnd > timeStart) ? (currTime - timeStart) / (timeEnd - timeStart) : 0.0f;

                if (nextIndex == currentIndex) 
                {
                    ApplyKeyframe(channel, outputValues[currentIndex], translations, rotations, scales);
                }
                else
                {
                    InterpolateKeyframes(channel, outputValues[currentIndex], outputValues[nextIndex], factor, translations, rotations, scales);
                }
            }

            for (size_t i = 0; i < nodeTransforms.size(); ++i)
            {
                nodeTransforms[i] = glm::translate(glm::mat4(1.0f), translations[i]) *
                    glm::mat4_cast(rotations[i]) *
                    glm::scale(glm::mat4(1.0f), scales[i]);
            }
        }

        static void EvaluateRigidAnimation(Animation& anim, Node& node, float currTime,
            const std::vector<size_t>& keyframeIndices)
        {
            const auto& channels = anim.GetChannels();
            const auto& samplers = anim.GetSamplers();

            glm::vec3 translation = glm::vec3(0.0f);
            glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            glm::vec3 scale = glm::vec3(1.0f);

            for (size_t channelIndex = 0; channelIndex < channels.size(); ++channelIndex) // Hardcoded for testing
            {
                const auto& channel = channels[channelIndex];
                const auto& sampler = samplers[channel.GetSamplerIndex()];
                const auto& inputTimes = sampler.GetTimes();
                const auto& outputValues = sampler.GetValues();

                if (inputTimes.empty())
                    continue;

                size_t currentIndex = keyframeIndices[channelIndex];
                size_t nextIndex = (currentIndex + 1 < inputTimes.size()) ? currentIndex + 1 : 0;

                float timeStart = inputTimes[currentIndex];
                float timeEnd = inputTimes[nextIndex];

                if (nextIndex == 0)
                {
                    timeEnd = inputTimes[nextIndex] + anim.GetDuration(); 
                }

                float factor = (timeEnd > timeStart) ? (currTime - timeStart) / (timeEnd - timeStart) : 0.0f;

                //std::cout << "curr: " << currentIndex << " next: "
                //    << nextIndex << " time: " << currTime << " fac: " << factor << std::endl;

                if (nextIndex == currentIndex) // No interpolation needed
                {
                    ApplyKeyframe(channel, outputValues[currentIndex], translation, rotation, scale);
                }
                else
                {
                    InterpolateKeyframes(channel, outputValues[currentIndex], outputValues[nextIndex], factor, translation, rotation, scale);
                }
            }

            node.SetTRS(translation, rotation, scale);
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

                rotations[targetNode] = glm::normalize(glm::slerp(q1, q2, factor));
            }
            else if (targetPath == TargetPath::SCALE)
            {
                scales[targetNode] = glm::mix(glm::vec3(prevValue), glm::vec3(nextValue), factor);
            }
        }

        static void InterpolateKeyframes(const AnimationChannel& channel,
            const glm::vec4& prevValue,
            const glm::vec4& nextValue,
            float factor,
            glm::vec3& translation,
            glm::quat& rotation,
            glm::vec3& scale)
        {
            TargetPath targetPath = channel.GetTargetPath();

            if (targetPath == TargetPath::TRANSLATION)
            {
                translation = glm::mix(glm::vec3(prevValue), glm::vec3(nextValue), factor);
            }
            else if (targetPath == TargetPath::ROTATION)
            {
                glm::quat q1(prevValue.w, prevValue.x, prevValue.y, prevValue.z);
                glm::quat q2(nextValue.w, nextValue.x, nextValue.y, nextValue.z);

                rotation = glm::normalize(glm::slerp(q1, q2, factor));
            }
            else if (targetPath == TargetPath::SCALE)
            {
                scale = glm::mix(glm::vec3(prevValue), glm::vec3(nextValue), factor);
            }
        }

        static void ApplyKeyframe(const AnimationChannel& channel,
            const glm::vec4& keyframeValue,
            glm::vec3& translation,
            glm::quat& rotation,
            glm::vec3& scale)
        {
            TargetPath targetPath = channel.GetTargetPath();

            if (targetPath == TargetPath::TRANSLATION)
            {
                translation = glm::vec3(keyframeValue);
            }
            else if (targetPath == TargetPath::ROTATION)
            {
                rotation = glm::quat(keyframeValue.w, keyframeValue.x, keyframeValue.y, keyframeValue.z);
            }
            else if (targetPath == TargetPath::SCALE)
            {
                scale = glm::vec3(keyframeValue);
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