#ifndef LIST_CYCLER_H
#define LIST_CYCLER_H

#include <string>
#include <vector>
#include <stdexcept>

namespace JLEngine
{
	template <typename T>
	class ListCycler
	{
	public:
		ListCycler() : m_currentIndex(0) {}

		explicit ListCycler(std::vector<T>&& items) 
			: m_items(std::move(items)), m_currentIndex(0) {}

		void Set(std::vector<T>&& data)
		{
			m_items = std::move(data);
			m_currentIndex = 0;
		}

		const T& Current() const
		{
			if (m_items.empty())
			{
				throw std::out_of_range("ListCycler: Attempted to get an item from an empty list.");
			}

			const T& item = m_items[m_currentIndex];
			return item;
		}

		const T& Next()
		{
			if (m_items.empty())
			{
				throw std::out_of_range("ListCycler: Attempted to get an item from an empty list.");
			}

			m_currentIndex = (m_currentIndex + 1) % m_items.size();
			const T& item = m_items[m_currentIndex];
			return item;
		}

	private:
		std::vector<T> m_items;
		size_t m_currentIndex;
	};
}

#endif