#ifndef SINGLETON_H
#define SINGLETON_H
/***********************************************************
 *
 * Filename: Singleton.h
 * Author: Chris Scott
 * Purpose: Creates a basic singleton class that is used to inheritent from
 *
 ***********************************************************/
/**
* @class Singleton
* @brief A templated singleton design pattern made so that it can be inherited, for other classes.
*	 
* @author Chris Scott
* @version 1.0 
* @date 29/02/12
*/
namespace JLEngine
{
	template <class Type>
	class Singleton
	{
	public:
		/**
		* @brief Default destructor it also made virtual so that the class has to be a base class and cannot
		* be used to make objects of type Singleton
		*
		* @param  none
		* @return none
		*/
		virtual ~Singleton();
		/**
		* @brief getInstance method is used to return the static object of this class
		*
		* @param  none
		* @return returns the single object of this type
		*/
		static Type* getInstance();
	protected:
		/**
		* @brief Default constructor made protected so that it cannot be called
		*
		* @param  none
		* @return none
		*/
		Singleton();
		static Type* m_instance;
	};
	template <class Type>
	Type* Singleton<Type>::m_instance = 0;
}

#include "Singleton.cpp"
#endif