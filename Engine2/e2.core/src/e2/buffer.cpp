
#include "e2/buffer.hpp"
#include "e2/log.hpp"

#include <fstream>
#include <filesystem>






e2::Data::~Data()
{

}






e2::IStream::IStream(bool transcode)
	: m_transcode(transcode)
	, m_cursor(0)
{

}

bool e2::IStream::consume(uint64_t size, uint64_t& oldCursor)
{
	oldCursor = m_cursor;
	if (remaining() >= size)
	{
		m_cursor += size;

		return true;
	}

	return false;
}

void e2::IStream::seek(uint64_t newCursor)
{
	m_cursor = newCursor;
}

uint64_t e2::IStream::cursor() const
{
	return m_cursor;
}

uint64_t e2::IStream::remaining() const
{
	return size() - cursor();
}


e2::IStream& e2::IStream::operator<<(e2::Data const& value)
{
	value.write(*this);

	return *this;
}

e2::IStream& e2::IStream::operator>>(e2::Data& value)
{
	if (!value.read(*this))
	{
		LogError("failed to read data");
	}

	return *this;
}

e2::IStream& e2::IStream::operator>>(glm::mat4& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this >> value[i];
	}

	return *this;
}

e2::IStream& e2::IStream::operator>>(glm::mat3& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this >> value[i];
	}

	return *this;
}

e2::IStream& e2::IStream::operator<<(glm::mat4 const& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this << value[i];
	}

	return *this;
}

e2::IStream& e2::IStream::operator<<(glm::mat3 const& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this << value[i];
	}

	return *this;
}

e2::IStream& e2::IStream::operator>>(glm::quat& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::quat const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::vec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::vec3& value)
{
	*this >> value.x >> value.y >> value.z;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::vec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::ivec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}



e2::IStream& e2::IStream::operator>>(glm::ivec3& value)
{
	*this << value.x >> value.y >> value.z;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::ivec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::uvec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}


e2::IStream& e2::IStream::operator>>(glm::uvec3& value)
{
	*this << value.x >> value.y >> value.z;
	return*this;
}

e2::IStream& e2::IStream::operator>>(glm::uvec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}


e2::IStream& e2::IStream::operator<<(glm::vec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::vec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::vec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::ivec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::ivec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::ivec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::uvec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::IStream& e2::IStream::operator<<(glm::uvec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}


e2::IStream& e2::IStream::operator<<(glm::uvec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::IStream& e2::IStream::operator<<(std::string const& value)
{
	*this << uint64_t(value.size());
	write(reinterpret_cast<uint8_t const*>(value.data()), value.size());
	return *this;
}

e2::IStream& e2::IStream::operator>>(std::string& value)
{
	uint64_t num{};
	*this >> num;
	uint8_t const* readData = read(num);

	value.assign(reinterpret_cast<char const*>(readData), num);

	return *this;
}

e2::IStream& e2::IStream::operator<<(e2::IStream &value)
{
	uint64_t remainingBytes = value.remaining();
	uint8_t const* readPtr = value.read(remainingBytes);
	write(readPtr, remainingBytes);
	return *this;
}

e2::IStream& e2::IStream::operator>>(e2::IStream& value)
{
	value << *this;
	return *this;
}







e2::RawMemoryStream::RawMemoryStream(uint8_t* data, uint64_t size, bool transcode)
	: m_data(data)
	, m_size(size)
	, e2::IStream(transcode)
{
}


bool e2::RawMemoryStream::growable()
{
	return false;
}

uint64_t e2::RawMemoryStream::size() const
{
	return m_size;
}

uint8_t const* e2::RawMemoryStream::read(uint64_t numBytes)
{
	uint64_t oldCursor;
	if (consume(numBytes, oldCursor))
	{
		return m_data + oldCursor;
	}

	return nullptr;
}

bool e2::RawMemoryStream::write(uint8_t const* data, uint64_t size)
{
	uint64_t oldCursor;
	if (consume(size, oldCursor))
	{
		memcpy(m_data + oldCursor, data, size);
		return true;
	}

	return false;
}






e2::HeapStream::HeapStream(bool transcode, uint64_t slack /*= 0*/)
	: e2::IStream(transcode)
{
	if (slack > 0)
		reserve(slack);
}


void e2::HeapStream::reserve(uint64_t extraSlack)
{
	m_data.reserve(extraSlack);
}

bool e2::HeapStream::growable()
{
	return true;
}

uint64_t e2::HeapStream::size() const
{
	return m_data.size();
}

uint8_t const* e2::HeapStream::read(uint64_t numBytes)
{
	uint64_t oldCursor;
	if (consume(numBytes, oldCursor))
	{
		return &m_data[oldCursor];
	}

	LogError("failed to read {} bytes from stream", numBytes);

	return nullptr;
}

bool e2::HeapStream::write(uint8_t const* data, uint64_t numBytes)
{
	if (m_data.size() < m_cursor + numBytes)
	{
		m_data.resize(m_cursor + numBytes);
	}

	memcpy(&m_data[m_cursor], data, numBytes);

	seek(m_cursor + numBytes);
	return true;
}



e2::FileStream::FileStream(std::string const& path, e2::FileMode mode, bool transcode)
	: e2::IStream(transcode)
{
	m_valid = false;

	std::filesystem::path p = std::filesystem::absolute(path);
	if ((mode & e2::FileMode::WriteMask) == e2::FileMode::ReadWrite)
	{
		if (!std::filesystem::exists(p))
		{
			if (p.has_parent_path())
			{
				if (!std::filesystem::exists(p.parent_path()))
				{
					try
					{
						if (!std::filesystem::create_directories(p.parent_path()))
						{
							return;
						}
					}
					catch (...)
					{
						return;
					}
				}
			}
		}
	}

	std::ios_base::openmode m = std::ios_base::binary | std::ios_base::in;
	if ((mode & e2::FileMode::WriteMask) == e2::FileMode::ReadWrite)
	{
		m = m | std::ios_base::out;

		if ((mode & e2::FileMode::OpenMask) == e2::FileMode::Append)
		{
			m = m | std::ios_base::app;
		}
		else if ((mode & e2::FileMode::OpenMask) == e2::FileMode::Truncate)
		{
			m = m | std::ios_base::trunc;
		}
	}

	m_handle = std::fstream(path, m);
	if (!m_handle.good())
	{
		return;
	}

	m_valid = true;


	updateSize();

}

e2::FileStream::~FileStream()
{
	m_handle.close();
}

bool e2::FileStream::growable()
{
	return true;
}

uint64_t e2::FileStream::size() const
{
	if (!m_valid)
	{
		return 0;
	}

	return m_size;
}

uint8_t const* e2::FileStream::read(uint64_t numBytes)
{
	if (!m_valid)
	{
		return nullptr;
	}
	
	if (m_readBuffer.size() < numBytes)
	{
		m_readBuffer.resize(numBytes);
	}

	m_handle.seekg(m_cursor);
	m_handle.read(reinterpret_cast<char*>(m_readBuffer.data()), numBytes);

	if (!m_handle.good())
	{
		m_valid = false;
		return nullptr;
	}

	m_cursor += numBytes;

	return m_readBuffer.data();

}

bool e2::FileStream::write(uint8_t const* data, uint64_t size) 
{
	if (!m_valid)
	{
		return false;
	}

	m_handle.seekp(m_cursor);
	m_handle.write(reinterpret_cast<char const*>(data), size);

	if (!m_handle.good())
	{
		m_valid = false;
		return false;
	}

	if (m_cursor + size > m_size)
	{
		updateSize();
	}

	m_cursor += size;

	return true;
}

void e2::FileStream::updateSize()
{
	if (!m_valid)
	{
		return;
	}

	m_handle.seekg(0, std::ios_base::end);
	m_size = m_handle.tellg();
	m_handle.seekg(0, std::ios_base::beg);
}
