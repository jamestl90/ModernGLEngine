#include "Resource.h"

namespace JLEngine
{
	Resource::Resource(uint32 handle, string& name, string& path)
		: m_referenceCount(0), m_handle(handle), m_name(name), m_filename(path + name), m_filepath(path), m_fromFile(true)
	{
	}

	Resource::Resource(uint32 handle, string& filename)
		: m_referenceCount(0), m_handle(handle), m_filename(filename), m_fromFile(true)
	{
		string name = filename.substr(0, filename.find_last_of('\\') + 1);
		//string path = filename.substr(path.find_last_of('\\') + 1);

		m_name = name;
		m_filepath = "nopath";
	}

	Resource::Resource(uint32 handle)
		: m_referenceCount(0), m_handle(handle), m_name(""), m_filename(""), m_filepath(""), m_fromFile(false)
	{
	}

	Resource::~Resource()
	{

	}
}