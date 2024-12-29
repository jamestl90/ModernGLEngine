#ifndef IBUFFER_H
#define IBUFFER_H

#include <vector>
#include <iostream>
#include <stdexcept>

#include "Types.h"

namespace JLEngine
{
	/**
	 * \class	IBuffer
	 *
	 * \brief	Simple template interface wrapper for std::vector.
	 * 			To be used to derive different types of buffers 
	 * 			such as VertexBuffer. 
	 *
	 * \author	James Leupe
	 * \date	11/21/2012
	 */

	template <class T>
	class Buffer
	{
	public:
		Buffer(std::vector<T>& vals) 
			: m_buffer(vals) {}

		Buffer() {}

		virtual ~Buffer() {}

		void Set(std::vector<T>& vals);

		void Set(std::vector<T>&& vals);

		void SetAt(int pos, T val);

		bool InsertAt(int pos, std::vector<T>& vals);

		bool InsertAt(int pos, T* vals, int count);

		void Append(std::vector<T>& vals);

		void Add(T& val);

		T* Array();

		const std::vector<T>& GetBuffer() const;

		std::vector<T>& GetBufferMutable();

		void Clear();

		size_t Size();

	protected:

		std::vector<T> m_buffer;
	};

	template <class T>
	void Buffer<T>::SetAt(int pos, T val)
	{
		m_buffer.at(pos) = val;
	}

	template <class T>
	bool Buffer<T>::InsertAt(int pos, T* vals, int count)
	{
		auto it = m_buffer.begin() + pos;

		try
		{
			m_buffer.insert(it, vals, vals + count);
			return true;
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}
	}

	template<class T>
	void Buffer<T>::Append(std::vector<T>& vals)
	{
		m_buffer.insert(m_buffer.end(), vals.begin(), vals.end());
	}

	template <class T>
	bool Buffer<T>::InsertAt(int pos, std::vector<T>& vals)
	{
		auto it = m_buffer.begin() + pos;

		try
		{
			m_buffer.insert(it, vals.begin(), vals.end());
			return true;
		}
		catch (std::exception& e)
		{
			std::cout << e.what() << std::endl;
			return false;
		}
	}

	template <class T>
	void Buffer<T>::Set(std::vector<T>& vals)
	{
		Clear();
		m_buffer = vals;
	}

	template <class T>
	void Buffer<T>::Set(std::vector<T>&& vals)
	{
		m_buffer = std::move(vals);
	}

	template <class T>
	void Buffer<T>::Add(T& val)
	{
		m_buffer.push_back(val);
	}

	template <class T>
	size_t Buffer<T>::Size()
	{
		return m_buffer.size();
	}

	template <class T>
	void Buffer<T>::Clear()
	{
		m_buffer.clear();
	}

	template <class T>
	const std::vector<T>& Buffer<T>::GetBuffer() const
	{
		return m_buffer;
	}

	template <class T>
	std::vector<T>& Buffer<T>::GetBufferMutable()
	{
		return m_buffer;
	}


	template <class T>
	T* Buffer<T>::Array()
	{
		return &m_buffer[0];
	}
}

#endif