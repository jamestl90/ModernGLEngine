#ifndef ANIM_DATA_H
#define ANIM_DATA_H

#include "Resource.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

#include <vector>
#include <string>

namespace JLEngine
{
    enum TargetPath
    {
        TRANSLATION,
        ROTATION,
        SCALE
    };

    enum class InterpolationType
    {
        LINEAR = 0,
        CUBIC = 1,
    };

    static InterpolationType InterpolationFromString(const std::string& interpolation)
    {
        if (interpolation == "LINEAR")
            return InterpolationType::LINEAR;
        if (interpolation == "CUBIC")
            return InterpolationType::CUBIC;
        else
            return InterpolationType::LINEAR;
    }

    static TargetPath TargetPathFromString(const std::string& targetPath)
    {
        if (targetPath == "translation")
            return TargetPath::TRANSLATION;
        if (targetPath == "rotation")
            return TargetPath::ROTATION;
        if (targetPath == "scale")
            return TargetPath::SCALE;
        throw std::runtime_error("Invalid target path detected");
    }

    class Skeleton
    {
    public:
        struct Joint
        {
            int parentIndex;          // Parent joint index (-1 for root)
            glm::mat4 localTransform; // Local bind pose transform
        };

        std::vector<Joint> joints;    // Joint hierarchy
        std::string name;             // Skeleton name (optional)
    };

    class AnimationSampler
    {
    public:
        AnimationSampler() = default;

        // Accessors
        const std::vector<float>& GetTimes() const { return m_times; }
        const std::vector<glm::vec4>& GetValues() const { return m_values; }
        const InterpolationType GetInterpolation() const { return m_interpolation; }

        void SetTimes(std::vector<float>&& inputTimes) { m_times = std::move(inputTimes); }
        void SetValues(std::vector<glm::vec4>&& outputValues) { m_values = std::move(outputValues); }
        void SetInterpolation(const InterpolationType interpolation) { m_interpolation = interpolation; }

    private:

        int m_currentIndex = 0;
        std::vector<float> m_times;         
        std::vector<glm::vec4> m_values;   
        InterpolationType m_interpolation = InterpolationType::LINEAR;
    };

    class AnimationChannel
    {
    public:
        AnimationChannel(int samplerIndex, int targetNode, TargetPath targetPath)
            : m_samplerIndex(samplerIndex), m_targetNode(targetNode), m_targetPath(targetPath) {
        }

        int GetSamplerIndex() const { return m_samplerIndex; }
        int GetTargetNode() const { return m_targetNode; }
        const std::string& GetNodeName() const { return m_nodeName; }
        void SetNodeName(std::string& name) { m_nodeName = name; }
        TargetPath GetTargetPath() const { return m_targetPath; }

        bool IsUpdated() { return m_updated; }
        void UpdateTargetNode(int newIndex) { m_targetNode = newIndex; m_updated = true; }

    private:
        bool m_updated = false;
        int m_samplerIndex;          
        int m_targetNode;           
        std::string m_nodeName;
        TargetPath m_targetPath;
    };

    class Animation : public Resource
    {
    public:
        Animation(const std::string& name) : Resource(name), m_name(name) {}

        const std::string& GetName() const { return m_name; }
        std::vector<AnimationSampler>& GetSamplers() { return m_samplers; }
        AnimationSampler& GetSampler(int index) { return m_samplers[index]; }
        std::vector<AnimationChannel>& GetChannels() { return m_channels; }

        void AddSampler(const AnimationSampler& sampler) { m_samplers.push_back(sampler); }
        void AddChannel(const AnimationChannel& channel) { m_channels.push_back(channel); }

        void CalcDuration()
        {
            float maxTime = 0.0f;
            for (const auto& sampler : GetSamplers())
            {
                if (!sampler.GetTimes().empty())
                {
                    maxTime = std::max(maxTime, sampler.GetTimes().back());
                }
            }
            m_duration = maxTime;
        }
        float GetDuration() { return m_duration; }

        void PrecomputeSamplers()
        {
            m_precomputedSamplers.clear();
            m_precomputedSamplers.reserve(m_channels.size());
            for (const auto& channel : m_channels)
            {
                m_precomputedSamplers.push_back(&m_samplers[channel.GetSamplerIndex()]);
            }
        }

        const std::vector<const AnimationSampler*>& GetPrecomputedSamplers() const
        {
            return m_precomputedSamplers;
        }

        friend std::ostream& operator <<(std::ostream& os, const Animation& obj)
        {
            os << "Animation\n" << "NumChannels: " << obj.m_channels.size() <<
                "\nNum Samplers: " << obj.m_samplers.size() << "\n";
            for (auto& item : obj.m_samplers)
            {
                os << "SamplerInfo: \n" << "Times: " 
                    << item.GetTimes().size() << "\nValues: " << item.GetValues().size() << std::endl;

                for (int i = 0; i < item.GetTimes().size(); i++)
                {
                    os << "Time: " << item.GetTimes()[i] << " Value: " 
                        << glm::to_string(item.GetValues()[i]) << std::endl;
                }
            }
            return os;
        }

    private:
        std::vector<const AnimationSampler*> m_precomputedSamplers;
        float m_duration = 0.0f;
        std::string m_name;                           // Animation name
        std::vector<AnimationSampler> m_samplers;     // List of samplers
        std::vector<AnimationChannel> m_channels;     // List of channels
    };
}

#endif