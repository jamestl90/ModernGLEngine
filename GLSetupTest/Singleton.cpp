#ifndef SINGLETON_CPP
#define SINGLETON_CPP
#include "Singleton.h"

namespace JLEngine
{
	template <class Type>
	Singleton<Type>::Singleton()
	{
	}

	template <class Type>
	Singleton<Type>::~Singleton()
	{
	}

	template <class Type>
	Type* Singleton<Type>::getInstance()
	{
		if(m_instance==nullptr)
		{
			m_instance = new Type;
		}
		return m_instance;
	}
}

#endif