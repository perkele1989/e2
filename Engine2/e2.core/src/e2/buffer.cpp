
#include "e2/buffer.hpp"
#include "e2/log.hpp"

#include <fstream>
#include <filesystem>


e2::Buffer::Buffer(bool encode, uint64_t slack /*= 0*/)
	: m_encode(encode)
{
	if(slack > 0)
		reserve(slack);

}

e2::Buffer::~Buffer()
{

}

void e2::Buffer::reserve(uint64_t extraSlack)
{
	m_data.reserve(extraSlack);
}

bool e2::Buffer::consume(uint64_t size, uint64_t &outCursor)
{
	outCursor = m_cursor;
	if (remaining() >= size)
	{
		m_cursor += size;

		return true;
	}

	return false;
}

void e2::Buffer::seek(uint64_t newCursor)
{
	if (size() == 0)
	{
		m_cursor = 0;
		return;
	}

	m_cursor = glm::clamp(newCursor, 0ULL, size() - 1ULL);
}

uint64_t e2::Buffer::cursor() const
{
	return m_cursor;
}

uint64_t e2::Buffer::remaining() const
{
	return size() - cursor();
}

uint64_t e2::Buffer::size() const
{
	return m_data.size();
}

glm::uint8_t* e2::Buffer::begin() const
{
	return m_data.data();
}

glm::uint8_t* e2::Buffer::current() const
{
	return m_data.data() + m_cursor;
}

glm::uint8_t * e2::Buffer::at(uint64_t cur) const
{
	return m_data.data() + cur;
}

uint64_t e2::Buffer::readFromFile(std::string const& fromFile, uint64_t offset /*= 0*/, uint64_t readSize /*= 0*/)
{
	std::ifstream handle(fromFile, std::ios::binary);
	if (!handle)
	{
		return 0;
	}

	uint64_t fileSize = std::filesystem::file_size(fromFile);
	
	if (fileSize == 0)
		return 0;

	offset = glm::clamp(offset, 0ULL, fileSize - 1ULL);

	handle.seekg(offset);

	if (readSize == 0 || readSize > fileSize - offset)
		readSize = fileSize - offset;

	std::vector<uint8_t> buffer(readSize, 0);
	handle.read(reinterpret_cast<char*>(buffer.data()), readSize);

	if (handle.fail())
	{
		LogError("Failed to read {} bytes from {} at offset {}", readSize, fromFile, offset);
		return 0;
	}

	m_data.insert(m_data.end(), buffer.begin(), buffer.end());
	return buffer.size();
}

void e2::Buffer::write(uint8_t const* data, uint64_t size)
{
	m_data.insert(m_data.end(), data, data + size);
}

bool e2::Buffer::writeToFile(std::string const& toFile, uint64_t from) const
{
	if (m_data.size() == 0)
	{
		return false;
	}


	std::ofstream handle(toFile, std::ios::binary|std::ios::trunc| std::ios::out);
	if (!handle)
	{
		return false;
	}

	from = glm::clamp(from, 0ULL, m_data.size() - 1ULL);

	uint64_t numBytes = m_data.size() - from;

	handle.write(reinterpret_cast<char const*>(m_data.data()) + from, numBytes);

	if (!handle.good())
	{
		LogError("Failed to write stream to {} at offset {}", toFile, from);
		return false;
	}

	return true;
}




uint8_t const* e2::Buffer::read(uint64_t numBytes)
{
	uint64_t oldCursor{};
	if (!consume(numBytes, oldCursor))
	{
		LogError("Failed to fetch {} bytes from stream", numBytes);
		return nullptr;
	}
	
	return m_data.data() + oldCursor;
}

e2::Buffer& e2::Buffer::operator<<(e2::Data const& value)
{
	value.write(*this);

	return *this;
}

e2::Buffer& e2::Buffer::operator>>(glm::mat4& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this >> value[i];
	}

	return *this;
}

e2::Buffer& e2::Buffer::operator>>(glm::mat3& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this >> value[i];
	}

	return *this;
}

e2::Buffer& e2::Buffer::operator<<(glm::mat4 const& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this << value[i];
	}

	return *this;
}

e2::Buffer& e2::Buffer::operator<<(glm::mat3 const& value)
{
	for (glm::length_t i = 0; i < value.length(); i++)
	{
		*this << value[i];
	}

	return *this;
}

e2::Buffer& e2::Buffer::operator>>(glm::quat& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::quat const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::vec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::vec3& value)
{
	*this << value.x >> value.y >> value.z;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::vec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::ivec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}



e2::Buffer& e2::Buffer::operator>>(glm::ivec3& value)
{
	*this << value.x >> value.y >> value.z;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::ivec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::uvec4& value)
{
	*this >> value.x >> value.y >> value.z >> value.w;
	return*this;
}


e2::Buffer& e2::Buffer::operator>>(glm::uvec3& value)
{
	*this << value.x >> value.y >> value.z;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(glm::uvec2& value)
{
	*this >> value.x >> value.y;
	return*this;
}


e2::Buffer& e2::Buffer::operator<<(glm::vec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::vec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::vec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::ivec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::ivec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::ivec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::uvec4 const& value)
{
	*this << value.x << value.y << value.z << value.w;
	return*this;
}

e2::Buffer& e2::Buffer::operator<<(glm::uvec3 const& value)
{
	*this << value.x << value.y << value.z;
	return*this;
}


e2::Buffer& e2::Buffer::operator<<(glm::uvec2 const& value)
{
	*this << value.x << value.y;
	return*this;
}

e2::Buffer& e2::Buffer::operator>>(e2::Data& value)
{
	value.read(*this);
	return *this;
}

e2::Buffer& e2::Buffer::operator<<(std::string const& value)
{
	*this << uint64_t(value.size());
	write(reinterpret_cast<uint8_t const*>(value.data()), value.size());
	return *this;
}

e2::Buffer& e2::Buffer::operator<<(Buffer const& value)
{
	write(value.begin(), value.size());
	return *this;
}

e2::Buffer& e2::Buffer::operator>>(std::string& value)
{
	uint64_t num{};
	*this >> num;
	uint8_t const* readData = read(num);

	value.assign(reinterpret_cast<char const*>(readData), num);

	return *this;
}

