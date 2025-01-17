#ifndef ANIM_DATA_H
#define ANIM_DATA_H

#include "Resource.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>

// stl
#include <vector>
#include <string>

namespace JLEngine
{
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
        // Constructor
        AnimationSampler() = default;

        // Accessors
        const std::vector<float>& GetInputTimes() const { return m_inputTimes; }
        const std::vector<glm::vec4>& GetOutputValues() const { return m_outputValues; }
        const std::string& GetInterpolation() const { return m_interpolation; }

        void SetInputTimes(const std::vector<float>& inputTimes) { m_inputTimes = inputTimes; }
        void SetOutputValues(const std::vector<glm::vec4>& outputValues) { m_outputValues = outputValues; }
        void SetInterpolation(const std::string& interpolation) { m_interpolation = interpolation; }

    private:
        std::vector<float> m_inputTimes;         
        std::vector<glm::vec4> m_outputValues;   
        std::string m_interpolation = "LINEAR";  
    };

    class AnimationChannel
    {
    public:
        // Constructor
        AnimationChannel(int samplerIndex, int targetNode, const std::string& targetPath)
            : m_samplerIndex(samplerIndex), m_targetNode(targetNode), m_targetPath(targetPath) {
        }

        // Accessors
        int GetSamplerIndex() const { return m_samplerIndex; }
        int GetTargetNode() const { return m_targetNode; }
        const std::string& GetTargetPath() const { return m_targetPath; }

    private:
        int m_samplerIndex;          
        int m_targetNode;            
        std::string m_targetPath;    
    };

    class Animation : public Resource
    {
    public:
        // Constructor
        Animation(const std::string& name) : Resource(name), m_name(name) {}

        // Accessors
        const std::string& GetName() const { return m_name; }
        const std::vector<AnimationSampler>& GetSamplers() const { return m_samplers; }
        const std::vector<AnimationChannel>& GetChannels() const { return m_channels; }

        // Modifiers
        void AddSampler(const AnimationSampler& sampler) { m_samplers.push_back(sampler); }
        void AddChannel(const AnimationChannel& channel) { m_channels.push_back(channel); }

    private:
        std::string m_name;                           // Animation name
        std::vector<AnimationSampler> m_samplers;     // List of samplers
        std::vector<AnimationChannel> m_channels;     // List of channels
    };
}

#endif