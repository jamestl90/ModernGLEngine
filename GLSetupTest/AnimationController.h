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
					}
				}
				else
				{
					m_currTime = std::min(m_currTime, animDuration);
				}
			}
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

		float GetCurrentTime() const { return m_currTime; }
		float GetPlaybackSpeed() const { return m_playbackSpeed; }
		bool IsLooping() const { return m_looping; }

		void SetPlaybackSpeed(float speed) { m_playbackSpeed = speed; }
		void SetLooping(bool looping) { m_looping = looping; }
		void SetSkeleton(std::shared_ptr<Skeleton> skeleton) { m_skeleton = skeleton; }

	private:

		std::shared_ptr<Skeleton> m_skeleton;
		Animation* m_currAnimation;
		float m_currTime;
		float m_playbackSpeed;
		bool m_looping;
	};
}

#endif