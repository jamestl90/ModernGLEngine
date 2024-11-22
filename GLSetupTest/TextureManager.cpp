#include "TextureManager.h"
#include "Texture.h"
#include "Platform.h"

namespace JLEngine
{
	TextureManager::TextureManager(Graphics* graphics)
		: GraphicsResourceManager<Texture>(graphics)
	{
	}

	TextureManager::~TextureManager()
	{
	}

	uint32 TextureManager::Add(string& name, string& path, bool fromFile, bool clampToEdge, float repeat)
	{
		Texture* texture = nullptr;

		try
		{
			texture = ResourceManager<Texture>::Add(name, path);
			texture->FromFile(fromFile);
			texture->SetClampToEdge(clampToEdge);
			texture->SetTextureRepeat(repeat);
			texture->Init(m_graphics);
		}
		catch (std::exception& e)
		{
				if (!JLEngine::Platform::AlertBox(e.what(), "Texture Error"))
				{
					std::cout << e.what() << std::endl;
				}			

			ResourceManager<Texture>::Remove(texture->GetHandle());
			return -1;
		}

		return texture->GetHandle();
	}

	uint32 TextureManager::Add( string& fullpath, bool fromFile /*= true*/, bool clampToEdge /*= false*/, float repeat /*= false*/ )
	{
		Texture* texture = nullptr;

		try
		{
			std::string placeholderPath = "";
			texture = GraphicsResourceManager<Texture>::Add(fullpath, placeholderPath);
			texture->FromFile(fromFile);
			texture->SetClampToEdge(clampToEdge);
			texture->SetTextureRepeat(repeat);
			texture->Init(m_graphics);
		}
		catch (std::exception& e)
		{
			if (!Platform::AlertBox(e.what(), "Texture Error"))
			{
				std::cout << e.what() << std::endl;
			}

			ResourceManager<Texture>::Remove(texture->GetHandle());
			return -1;
		}

		return texture->GetHandle();
	}

}