#ifndef RENDER_TARGET_POOL_H
#define RENDER_TARGET_POOL_H

#include "RenderTarget.h"
#include "RenderTargetFactory.h"
#include "Graphics.h"

#include <unordered_map>

namespace JLEngine
{
	class RenderTargetPool
	{
	public:

		~RenderTargetPool()
		{
			for (auto& entry : m_pool)
			{
				for (auto target : entry.second)
				{
					Graphics::DeleteRenderTarget(target);
					delete target;
				}
			}
		}

		RenderTarget* RequestRenderTarget(uint32_t width, uint32_t height, GLenum format)
		{
			RenderTargetKey key{ width, height, format };

			auto it = m_pool.find(key);
			if (it != m_pool.end() && !it->second.empty())
			{
				auto target = it->second.back();
				it->second.pop_back();
				return target;
			}

			RenderTarget* newTarget = RenderTargetFactory::Create(width, height, format);
			return newTarget;
		}

		void ReleaseRenderTarget(RenderTarget* target)
		{
			RenderTargetKey key{ target->GetWidth(), target->GetHeight(), target->GetTextureAttribute(0).internalFormat };

			m_pool[key].push_back(target);
		}

	private:

		struct RenderTargetKey
		{
			uint32_t width;
			uint32_t height;
			uint32_t format;
			bool operator==(const RenderTargetKey& other) const
			{
				return width == other.width && height == other.height && format == other.format;
			}
		};

		struct KeyHash
		{
			size_t operator()(const RenderTargetKey& key) const
			{
				return std::hash<int>()(key.width) ^ std::hash<int>()(key.height) ^ std::hash<int>()(key.format);
			}
		};

		std::unordered_map<RenderTargetKey, std::vector<RenderTarget*>, KeyHash> m_pool;
	};
}

#endif