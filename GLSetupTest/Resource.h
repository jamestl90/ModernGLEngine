#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>

#include "Types.h"

using std::string;

namespace JLEngine
{
    class Resource
    {
    public:
        virtual ~Resource() = default;

        const std::string& GetName() const
        {
            return m_name;
        }

        uint32_t GetHandle() const
        {
            return m_handle;
        }

    protected:
        Resource(uint32_t handle, const std::string& name)
            : m_handle(handle), m_name(name)
        {}

        uint32_t m_handle;
        std::string m_name;
    };
}

#endif