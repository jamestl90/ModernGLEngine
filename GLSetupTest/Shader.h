#ifndef SHADER_H
#define SHADER_H
#include <iostream>
#include <string>

#include "Types.h"

namespace JLEngine
{
	class Shader
	{
	public:

		Shader(int type, std::string name);
		~Shader() {}

		int GetType() { return m_type; }

		void SetShaderId(uint32 id) { m_shaderId = id; }
		uint32 GetShaderId() { return m_shaderId; }

		void SetName(std::string name) { m_name = name; }
		std::string GetName() const { return m_name; }

	protected: 

		uint32 m_shaderId;
		int m_type;

		std::string m_name;
	};
}

#endif