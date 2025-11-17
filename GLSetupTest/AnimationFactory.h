#ifndef ANIMATION_FACTORY_H
#define ANIMATION_FACTORY_H

#include "AnimData.h"
#include "ResourceManager.h"

namespace JLEngine
{
    class AnimationFactory
    {
    public:
        AnimationFactory(ResourceManager<Animation>* materialManager)
            : m_animManager(materialManager) {
        }

        std::shared_ptr<Animation> CreateAnimation(const std::string& name)
        {
            return m_animManager->Load(name, [&]()
                {
                    auto animation = std::make_shared<Animation>(name);
                    return animation;
                });
        }

    protected:
        ResourceManager<Animation>* m_animManager;
    };
}

#endif