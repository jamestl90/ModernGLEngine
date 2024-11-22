#ifndef SHADERMANAGER_H
#define SHADERMANAGER_H

#include <vector>

#include "GraphicsResourceManager.h"
#include "Graphics.h"
#include "ShaderProgram.h"
#include "ShaderGroup.h"

namespace JLEngine
{
	class Graphics;

	class ShaderManager : public GraphicsResourceManager<ShaderProgram>
	{
	public:
		ShaderManager(Graphics* graphics);
		~ShaderManager();

		uint32 Add(string& vert, string& frag, string& path);

		uint32 Add(ShaderGroup& group, string& name, string& path);
	};
}

#endif