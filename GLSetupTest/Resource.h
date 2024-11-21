#ifndef RESOURCE_H
#define RESOURCE_H

#include <string>

#include "Types.h"

using std::string;

namespace JLEngine
{
	/**
	 * \class	Resource
	 *
	 * \brief	Resource.
	 *
	 * \author	James
	 * \date	5/29/2012
	 */

	class Resource
	{
	public:

		/**
		 * \fn	Resource::Resource();
		 *
		 * \brief	Default constructor.
		 *
		 * \author	James
		 * \date	5/29/2012
		 */

		 Resource(uint32 handle, string& name, string& path);

		/**
		 * \fn	Resource::Resource(uint32 handle, string& filename);
		 *
		 * \brief	Constructor.
		 *
		 * \author	James
		 * \date	6/15/2012
		 *
		 * \param	handle				The handle.
		 * \param [in,out]	filename	Filename of the file.
		 */

		 Resource(uint32 handle, string& filename);

		/**
		 * \fn	Resource::~Resource();
		 *
		 * \brief	Destructor.
		 *
		 * \author	James
		 * \date	5/29/2012
		 */

		 virtual ~Resource();

		/**
		 * \fn	string Resource::getFileName()
		 *
		 * \brief	Gets the file name.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \return	The file name.
		 */

		 string GetFileName() { return m_filename; }

		/**
		 * \fn	void Resource::setFileName(string filename)
		 *
		 * \brief	Sets a file name.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \param	filename	Filename of the file.
		 */

		 void SetFileName(string filename) { m_filename = filename; }

		/**
		 * \fn	string Resource::getFilePath()
		 *
		 * \brief	Gets the file path.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \return	The file path.
		 */

		 string GetFilePath() { return m_filepath; }

		/**
		 * \fn	void Resource::getFilePath(string filePath)
		 *
		 * \brief	Gets a file path.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \param	filePath	Full pathname of the file.
		 */

		 void SetFilePath(string filePath) { m_filepath = filePath; }

		/**
		 * \fn	string Resource::getName()
		 *
		 * \brief	Gets the name.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \return	The name.
		 */

		 string GetName() { return m_name; }

		/**
		 * \fn	void Resource::getName(string name)
		 *
		 * \brief	Gets a name.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \param	name	The name.
		 */

		 void SetName(string name) { m_name = name; }

		/**
		 * \fn	uint32 Resource::getId()
		 *
		 * \brief	Gets the identifier.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \return	The identifier.
		 */

		 uint32 GetHandle() { return m_handle; }

		/**
		 * \fn	void Resource::setId(uint32 id)
		 *
		 * \brief	Sets an identifier.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \param	id	The identifier.
		 */

		 void SetHandle(uint32 handle) { m_handle = handle; }

		/**
		 * \fn	void Resource::incrementCount()
		 *
		 * \brief	Increment count.
		 *
		 * \author	James
		 * \date	5/29/2012
		 */

		 void IncrementCount() { m_referenceCount++; }

		/**
		 * \fn	void Resource::decrementCount()
		 *
		 * \brief	Decrement count.
		 *
		 * \author	James
		 * \date	5/29/2012
		 */

		 void DecrementCount() { m_referenceCount--; }

		/**
		 * \fn	int Resource::getReferenceCount()
		 *
		 * \brief	Gets the reference count.
		 *
		 * \author	James
		 * \date	5/29/2012
		 *
		 * \return	The reference count.
		 */

		 int GetReferenceCount() { return m_referenceCount; }

		 void FromFile(bool fromFile) { m_fromFile = fromFile; }
		 bool IsFromFile() { return m_fromFile; }

	protected:

		/**
		 * \summary	Filename of the file.
		 */

		string m_filename;

		/**
		 * \summary	The filepath.
		 */

		string m_filepath;

		/**
		 * \summary	The name.
		 */

		string m_name;

		/**
		 * \summary	The identifier.
		 */

		uint32 m_handle;

		/**
		 * \summary	the amount of times this resource exists
		 */

		int m_referenceCount;

		bool m_fromFile;
	};
}

#endif