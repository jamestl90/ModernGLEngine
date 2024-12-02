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

		void SetAt(int pos, T val);

		bool InsertAt(int pos, std::vector<T>& vals);

		bool InsertAt(int pos, T* vals, int count);

		void Add(T& val);

		T* Array();

		const std::vector<T>& GetBuffer() const;

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
	T* Buffer<T>::Array()
	{
		return &m_buffer[0];
	}

	template <class T>
	class SmallArray
	{
	public:
		SmallArray(T* data, uint32 size)
			: m_data(data), m_size(size)
		{
			if (data == nullptr)
			{
				throw std::invalid_argument("SmallArray: data pointer cannot be null.");
			}
		}
		
		SmallArray(uint32 size)
			: m_size(size), m_data(new T[size])
		{
		}

		SmallArray() 
			: m_size(0), m_data(nullptr)
		{
		}

		~SmallArray()
		{
			delete[] m_data;
		}

		void Create(T* data, uint32 size)
		{
			if (m_data != nullptr)
				delete[] m_data;

			m_size = size;
			m_data = data;
		}

		void Create(uint32 size)
		{
			if (m_data != nullptr)
				delete[] m_data;

			m_size = size;
			m_data = new T[m_size];
		}

		void Insert(T value, uint32 idx)
		{
			if (idx >= m_size)
			{
				throw std::out_of_range("Index out of bounds");
			}
			m_data[idx] = value;
		}

		// Getter for size
		uint32 Size() const { return m_size; }

		// Access operator for convenience
		T& operator[](uint32_t idx)
		{
			if (idx >= m_size)
			{
				throw std::out_of_range("Index out of bounds");
			}
			return m_data[idx];
		}

		const T& operator[](uint32_t idx) const
		{
			if (idx >= m_size)
			{
				throw std::out_of_range("Index out of bounds");
			}
			return m_data[idx];
		}

		T* data() { return m_data; }

		const T* data() const { return m_data; }

	protected:
		T* m_data = nullptr;
		uint32 m_size = 0;
	};
}

#endif