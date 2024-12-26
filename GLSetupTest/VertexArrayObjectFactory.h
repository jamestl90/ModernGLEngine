#ifndef VERTEX_ARRAY_OBJECT_FACTORY_H
#define VERTEX_ARRAY_OBJECT_FACTORY_H

#include "VertexArrayObject.h"
#include "ResourceManager.h"

#include <memory>
#include <string>

namespace JLEngine
{
	class VertexArrayObjectFactory
	{
	public:
        VertexArrayObjectFactory(ResourceManager<VertexArrayObject>* vaoManager)
            : m_vaoManager(vaoManager) {}

        std::shared_ptr<VertexArrayObject> CreateVertexArray(const std::string& name)
        {
            return m_vaoManager->Load(name, [&]()
                {
                    auto vao = std::make_shared<VertexArrayObject>(name, Graphics::API());

                    return vao;
                });
        }

    protected:
        ResourceManager<VertexArrayObject>* m_vaoManager;
	};
}

#endif