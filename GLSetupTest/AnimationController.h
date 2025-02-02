#ifndef ANIMATION_CONTROLLER_H
#define ANIMATION_CONTROLLER_H

#include <memory>
#include "AnimData.h"

namespace JLEngine
{
	class AnimationController
	{
	public: 
		AnimationController() 
			: m_currAnimation(nullptr), m_currTime(0.0f), m_playbackSpeed(1.0f), m_looping(true) {}

		void SetAnimation(Animation* anim)
		{
			m_currAnimation = anim;
			m_currTime = 0.0f;

			if (m_currAnimation)
			{
				m_channelKeyframeIndices.resize(m_currAnimation->GetChannels().size(), 0);
			}
		}

		Animation* CurrAnim() { return m_currAnimation; }

		void Update(float deltaTime)
		{
			if (m_currAnimation)
			{
				m_currTime += deltaTime * m_playbackSpeed;

				float animDuration = m_currAnimation->GetDuration();
				if (m_looping)
				{
					if (m_currTime > animDuration)
					{
						m_currTime = fmod(m_currTime, animDuration);
						std::fill(m_channelKeyframeIndices.begin(), m_channelKeyframeIndices.end(), 0);
					}
				}
				else
				{
					m_currTime = std::min(m_currTime, animDuration);
				}
			}

			UpdateKeyframeIndices();
		}

		float GetTime() const
		{
			if (!m_currAnimation)
			{
				return 0.0f;
			}

			float animDuration = m_currAnimation->GetDuration();
			if (m_looping)
			{
				return fmod(m_currTime, animDuration);
			}
			else
			{
				return std::min(m_currTime, animDuration);
			}
		}

		const std::vector<size_t>& GetKeyframeIndices() const { return m_channelKeyframeIndices; }

		float GetCurrentTime() const { return m_currTime; }
		float GetPlaybackSpeed() const { return m_playbackSpeed; }
		bool IsLooping() const { return m_looping; }

		void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
		void SetLooping(bool looping) { m_looping = looping; }
		void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { m_skeleton = skeleton; }

	private:

		void UpdateKeyframeIndices()
		{
			const auto& precomputedSamplers = m_currAnimation->GetPrecomputedSamplers();

			for (size_t i = 0; i < precomputedSamplers.size(); ++i)
			{
				const auto& inputTimes = precomputedSamplers[i]->GetTimes();
				size_t& currentIndex = m_channelKeyframeIndices[i];

				if (currentIndex >= inputTimes.size() - 1)
					continue;

				while (currentIndex < inputTimes.size() - 2 && m_currTime > inputTimes[currentIndex + 2])
				{
					currentIndex += 2;  // Jump ahead two keyframes
				}

				if (currentIndex < inputTimes.size() - 1 && m_currTime > inputTimes[currentIndex + 1])
				{
					currentIndex++;
				}
			}
		}

		std::vector<size_t> m_channelKeyframeIndices;
		std::shared_ptr<Skeleton> m_skeleton;
		Animation* m_currAnimation;
		float m_currTime;
		float m_playbackSpeed;
		bool m_looping;
	};
}

#endif