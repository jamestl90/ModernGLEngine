#include "FileSystem.h"

namespace JLEngine
{
	FileSystem::FileSystem()
	{
		m_filesIter = m_files.begin();
	}

	size_t FileSystem::loadFile(string fileName, bool binary)
	{
		ifstream* file = new ifstream;
		file->open(fileName.c_str(), binary ? std::ios::in | std::ios::binary : std::ios::in);

		if (file->is_open())
		{
			int id = 0;

			if (m_ids.empty())
			{
				id = (signed)m_files.size() + 1;
			}
			else
			{
				id = m_ids.top();
				m_ids.pop(); 
			}
			m_files[(signed)m_files.size() + 1] = file;

			return m_files.size();
		}
		else
		{
			return -1; 
		}
	}

	bool FileSystem::isGood(int id)
	{
		if (!isValidID(id))
		{
			return false;
		}
		if (m_files[id]->good())
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	vector<string>* FileSystem::getNextLine(int id, char delimiter, char EOLchar, char escapeChar)
	{
		return getData(id, delimiter, EOLchar, escapeChar); 
	}

	vector<string>* FileSystem::getData(int id, char delimiter, char EOLchar, char escapeChar)
	{
		if (!isGood(id))
		{
			return NULL;
		}
		else
		{
			vector<string>* tempVector = new vector<string>;
			string line, segment;

			do 
			{
				if (m_files[id]->eof())
					return NULL;

				getline(*m_files[id], line, EOLchar);
			}
			while (line == "" || line.at(0) == escapeChar);

			// dont read lines with escape character or empty

			stringstream ss(line);

			if (delimiter == NULL)
			{
				while (getline(ss, segment))	// no delim
				{
					tempVector->push_back(segment);
				}
			}
			else
			{
				while (getline(ss, segment, delimiter))	// using delim
				{
					tempVector->push_back(segment);
				}
			}

			return tempVector;
		}
	}

	void FileSystem::removeEmptyStrings(vector<string>& vec)
	{
		vec.erase(remove(vec.begin(), vec.end(), ""), vec.end());
	}

	bool FileSystem::closeFile(int id)
	{
		if (!isValidID(id))
		{
			return false; 
		}
		if (m_files[id] != NULL)	// make sure it hasn't been erased
		{
			if (m_files[id]->is_open())
			{
				m_files[id]->close();				// close the file
				m_ids.push(id);						// push valid id on stack
				delete m_files[id];					// delete ifstream data
				m_filesIter = m_files.find(id);		// set iterator
				m_files.erase(m_filesIter);			// erase element at this iterater position
				return true;						// return from function
			}
		}
		return false;
	}

	void FileSystem::clearFiles()
	{
		for (m_filesIter = m_files.begin(); m_filesIter != m_files.end(); m_filesIter++)
		{
			if ((*m_filesIter).second->is_open())	// if file is open
			{
				(*m_filesIter).second->close();	// close the file
			}
			delete (*m_filesIter).second;			// delete data
			m_filesIter = m_files.erase(m_filesIter);		// erase element, set new iterator 
		}
		m_filesIter = m_files.begin();		// set iterator to beginning
	}

	bool FileSystem::isValidID(int id)
	{
		if (id < 0)
		{
			return false;
		}
		return true; 
	}

	void FileSystem::getWholeFileString(int id, string& str)
	{
		if (!isGood(id))
		{
			return;
		}
		else
		{
			string tempLine;

			m_files[id]->seekg(0, ios::beg);	// set to start of file

			std::stringstream buffer;
			buffer << m_files[id]->rdbuf();

			str = buffer.str();
		}
	}

	void FileSystem::getWholeFileAsArray(int id, char* str)
	{
		if (!isGood(id))
		{
			str = nullptr;
			return;
		}
		else
		{
			int size;

			m_files[id]->seekg(0, ios::end);
			size = (int)m_files[id]->tellg();
			m_files[id]->seekg(0, ios::beg);

			str = new char[size];

			m_files[id]->read(str, size);

		    str[size] = '\0';
		}
	}
}