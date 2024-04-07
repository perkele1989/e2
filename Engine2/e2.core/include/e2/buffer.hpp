
#pragma once

#include <e2/export.hpp>

#include <e2/log.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <bit>

namespace e2
{


	class Buffer;

	class E2_API Data
	{
	public:
		virtual ~Data();
		virtual void write(Buffer& destination) const = 0;
		virtual bool read(Buffer& source) = 0;

	protected:
	};

	template <typename T>
	concept PlainOldData = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>;

	template <typename T>
	concept SpecifiedData = std::is_base_of_v<e2::Data, T>;

	/** 
	 * @todo rewrite into several classes
	 * e2::AlignedReader
	 */
	class E2_API Buffer
	{
	public:
		Buffer(bool encode = true, uint64_t slack = 0);
		~Buffer();

		void reserve(uint64_t extraSlack);

		bool consume(uint64_t size, uint64_t &oldCursor); 

		void seek(uint64_t newCursor);
		uint64_t cursor() const;
		uint64_t remaining() const;
		uint64_t size() const;

		uint8_t* begin() const;
		uint8_t* current() const;
		uint8_t* at(uint64_t cur) const;

		/** read up to `size` bytes from file, starting at `offset`. returns number of bytes read. if size is 0, it will read until eof */
		uint64_t readFromFile(std::string const& fromFile, uint64_t offset = 0, uint64_t size = 0);

		/** writes entire buffer to file by default, can be specified by setting from, for example, to cursor() */
		bool writeToFile(std::string const& toFile, uint64_t from = 0) const;


		uint8_t const * read(uint64_t numBytes);
		void write(uint8_t const* data, uint64_t size);

		Buffer& operator<<(std::string const& value);
		Buffer& operator>>(std::string& value);

		Buffer& operator<<(glm::uvec2 const& value);
		Buffer& operator>>(glm::uvec2& value);

		Buffer& operator<<(glm::uvec3 const& value);
		Buffer& operator>>(glm::uvec3& value);

		Buffer& operator<<(glm::uvec4 const& value);
		Buffer& operator>>(glm::uvec4& value);

		Buffer& operator<<(glm::ivec2 const& value);
		Buffer& operator>>(glm::ivec2& value);

		Buffer& operator<<(glm::ivec3 const& value);
		Buffer& operator>>(glm::ivec3& value);

		Buffer& operator<<(glm::ivec4 const& value);
		Buffer& operator>>(glm::ivec4& value);

		Buffer& operator<<(glm::vec2 const& value);
		Buffer& operator>>(glm::vec2& value);

		Buffer& operator<<(glm::vec3 const& value);
		Buffer& operator>>(glm::vec3& value);

		Buffer& operator<<(glm::vec4 const& value);
		Buffer& operator>>(glm::vec4& value);

		Buffer& operator<<(glm::quat const& value);
		Buffer& operator>>(glm::quat& value);

		Buffer& operator<<(glm::mat3 const& value);
		Buffer& operator>>(glm::mat3& value);

		Buffer& operator<<(glm::mat4 const& value);
		Buffer& operator>>(glm::mat4& value);



		template <e2::PlainOldData PODType>
		Buffer& operator<<(PODType const& value)
		{
			static_assert (sizeof(value) == 1 || sizeof(value) == 2 || sizeof(value) == 4 || sizeof(value) == 8);

			if (!m_encode)
			{
				write(reinterpret_cast<uint8_t const*>(&value), sizeof(PODType));
			}
			else if constexpr (std::endian::native == std::endian::little)
			{
				// This crap is needed when working on non integer types (we want this for floats and doubles too
				if constexpr (sizeof(PODType) == 1)
				{
					write(reinterpret_cast<uint8_t const*>(&value), sizeof(PODType));
				}
				else if constexpr (sizeof(PODType) == 2)
				{
					uint16_t tmp = *reinterpret_cast<uint16_t const*>(&value);
					tmp = std::byteswap(tmp);
					write(reinterpret_cast<uint8_t const*>(&tmp), sizeof(PODType));
				}
				else if constexpr (sizeof(PODType) == 4)
				{
					uint32_t tmp = *reinterpret_cast<uint32_t const*>(&value);
					tmp = std::byteswap(tmp);
					write(reinterpret_cast<uint8_t const*>(&tmp), sizeof(PODType));
				}
				else if constexpr (sizeof(PODType) == 8)
				{
					uint64_t tmp = *reinterpret_cast<uint64_t const*>(&value);
					tmp = std::byteswap(tmp);
					write(reinterpret_cast<uint8_t const*>(&tmp), sizeof(PODType));
				}
			}
			else // big endian, just write raw
			{
				write(reinterpret_cast<uint8_t*>(&value), sizeof(PODType));
			}

			return *this;
		}

		template <e2::PlainOldData PODType>
		Buffer& operator>>(PODType& value)
		{
			static_assert(sizeof(value) == 1 || sizeof(value) == 2 || sizeof(value) == 4 || sizeof(value) == 8);

			uint64_t oldCursor{};
			if (!consume(sizeof(PODType), oldCursor))
			{
				LogError("Failed to read {} bytes from stream, not enough data remaining", remaining());
				return *this;
			}

			if (!m_encode)
			{
				value = *reinterpret_cast<PODType const*>(at(oldCursor));
			}
			else if constexpr (std::endian::native == std::endian::little)
			{
				// This crap is needed when working on non integer types (we want this for floats and doubles too
				if constexpr (sizeof(PODType) == 1)
				{
					value = *reinterpret_cast<PODType const*>(at(oldCursor));
				}
				else if constexpr (sizeof(PODType) == 2)
				{
					uint16_t tmp = *reinterpret_cast<uint16_t const*>(at(oldCursor));
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
				else if constexpr (sizeof(PODType) == 4)
				{
					uint32_t tmp = *reinterpret_cast<uint32_t const*>(at(oldCursor));
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
				else if constexpr (sizeof(PODType) == 8)
				{
					uint64_t tmp = *reinterpret_cast<uint64_t const*>(at(oldCursor));
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
			}
			else // big endian, just read raw
			{

				value = *reinterpret_cast<PODType const*>(at(oldCursor));
			}

			return *this;
		}

		Buffer& operator<<(Data const& value);
		Buffer& operator>>(Data& value);

		Buffer& operator<<(Buffer const& value);

	protected:

		bool m_encode{};
		mutable std::vector<uint8_t> m_data; 
		uint64_t m_cursor{};
	};

}