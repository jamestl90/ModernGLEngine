#ifndef CUBEMAP_FACTORY_H
#define CUBEMAP_FACTORY_H

#include "ResourceManager.h"
#include "Texture.h"
#include "GraphicsAPI.h"
#include "TextureReader.h"
#include "ImageData.h"
#include "Graphics.h"
#include "Cubemap.h"

#include <string>
#include <memory>

namespace JLEngine
{
	class CubemapFactory
	{
	public:
		CubemapFactory(ResourceManager<Cubemap>* cubemapManager, GraphicsAPI* graphics)
			: m_cubemapManager(cubemapManager), m_graphics(graphics) {}

        std::shared_ptr<Cubemap> CreateCubemapFromFiles(const std::string& name, std::array<std::string, 6> fileNames, std::string folderPath)
        {
                return m_cubemapManager->Load(name, [&]()
                {
                    auto imgData = TextureReader::LoadCubeMapHDR(folderPath, fileNames);
                    auto cubemap = std::make_shared<Cubemap>(name, m_graphics);

                    TexParams params = Cubemap::HDRCubemapParams();
                    cubemap->SetParams(params);

                    Graphics::CreateCubemap(cubemap.get());

                    return cubemap;
                });
        }

	protected:

		ResourceManager<Cubemap>* m_cubemapManager;
		GraphicsAPI* m_graphics;
	};
}

#endif