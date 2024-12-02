#include "MaterialManager.h"

namespace JLEngine
{
    MaterialManager::MaterialManager(Graphics* graphics) : m_graphics(graphics) 
    {
        m_defaultMat = CreateMaterial("DefaultMaterial");
    }
}