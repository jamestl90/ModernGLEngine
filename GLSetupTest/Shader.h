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

		void SetShaderId(uint32_t id) { m_shaderId = id; }
		uint32_t GetShaderId() { return m_shaderId; }

		void SetName(std::string name) { m_name = name; }
		std::string GetName() const { return m_name; }

		std::string& GetSource() { return m_source; }
		void SetSource(const std::string& source) { m_source = ""; m_source = source; }

	protected: 

		std::string m_source;
		uint32_t m_shaderId;
		int m_type;

		std::string m_name;
	};
}

#endif