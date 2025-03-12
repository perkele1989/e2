
#pragma once

#include <e2/export.hpp>

#include <e2/log.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <bit>
#include <fstream>

namespace e2
{


	class Data;

	template <typename T>
	concept PlainOldData = std::is_integral_v<T> || std::is_floating_point_v<T> || std::is_enum_v<T>;

	template <typename T>
	concept SpecifiedData = std::is_base_of_v<e2::Data, T>;

	
	class E2_API IStream
	{
	public:
		IStream(bool transcode = true);
		virtual ~IStream() = default;

		virtual bool growable() = 0;
		virtual uint64_t size() const = 0;
		
		virtual uint8_t const* read(uint64_t numBytes) = 0;
		virtual bool write(uint8_t const* data, uint64_t size) = 0;


		bool consume(uint64_t size, uint64_t& oldCursor);
		void seek(uint64_t newCursor);
		uint64_t cursor() const;
		uint64_t remaining() const;

		IStream& operator<<(std::string const& value);
		IStream& operator>>(std::string& value);

		IStream& operator<<(glm::uvec2 const& value);
		IStream& operator>>(glm::uvec2& value);

		IStream& operator<<(glm::uvec3 const& value);
		IStream& operator>>(glm::uvec3& value);

		IStream& operator<<(glm::uvec4 const& value);
		IStream& operator>>(glm::uvec4& value);

		IStream& operator<<(glm::ivec2 const& value);
		IStream& operator>>(glm::ivec2& value);

		IStream& operator<<(glm::ivec3 const& value);
		IStream& operator>>(glm::ivec3& value);

		IStream& operator<<(glm::ivec4 const& value);
		IStream& operator>>(glm::ivec4& value);

		IStream& operator<<(glm::vec2 const& value);
		IStream& operator>>(glm::vec2& value);

		IStream& operator<<(glm::vec3 const& value);
		IStream& operator>>(glm::vec3& value);

		IStream& operator<<(glm::vec4 const& value);
		IStream& operator>>(glm::vec4& value);

		IStream& operator<<(glm::quat const& value);
		IStream& operator>>(glm::quat& value);

		IStream& operator<<(glm::mat3 const& value);
		IStream& operator>>(glm::mat3& value);

		IStream& operator<<(glm::mat4 const& value);
		IStream& operator>>(glm::mat4& value);



		template <e2::PlainOldData PODType>
		IStream& operator<<(PODType const& value)
		{
			static_assert (sizeof(value) == 1 || sizeof(value) == 2 || sizeof(value) == 4 || sizeof(value) == 8);

			if (!m_transcode)
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
		IStream& operator>>(PODType& value)
		{
			static_assert(sizeof(value) == 1 || sizeof(value) == 2 || sizeof(value) == 4 || sizeof(value) == 8);

			uint8_t const* readData = read(sizeof(PODType));

			if (!readData)
			{
				LogError("Failed to read {} bytes from stream, not enough data remaining", sizeof(PODType));
				return *this;
			}

			if (!m_transcode)
			{
				value = *reinterpret_cast<PODType const*>(readData);
			}
			else if constexpr (std::endian::native == std::endian::little)
			{
				// This crap is needed when working on non integer types (we want this for floats and doubles too
				if constexpr (sizeof(PODType) == 1)
				{
					value = *reinterpret_cast<PODType const*>(readData);
				}
				else if constexpr (sizeof(PODType) == 2)
				{
					uint16_t tmp = *reinterpret_cast<uint16_t const*>(readData);
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
				else if constexpr (sizeof(PODType) == 4)
				{
					uint32_t tmp = *reinterpret_cast<uint32_t const*>(readData);
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
				else if constexpr (sizeof(PODType) == 8)
				{
					uint64_t tmp = *reinterpret_cast<uint64_t const*>(readData);
					tmp = std::byteswap(tmp);
					value = *reinterpret_cast<PODType*>(&tmp);
				}
			}
			else // big endian, just read raw
			{

				value = *reinterpret_cast<PODType const*>(readData);
			}

			return *this;
		}

		IStream& operator<<(Data const& value);
		IStream& operator>>(Data& value);

		/** Writes any remaining data of value to this stream */
		IStream& operator<<(IStream& value);

		/** Writes any remianing data of this stream to value*/
		IStream& operator>>(IStream& value);

	protected:
		bool m_transcode{}; // true if this stream transcodes data on read/write
		uint64_t m_cursor{}; // the cursor of this stream in bytes
	};


	class E2_API Data
	{
	public:
		virtual ~Data();
		virtual void write(IStream& destination) const = 0;
		virtual bool read(IStream& source) = 0;

	protected:
	};


	/**
	 * Raw memory pointing to provided data (backed by user)
	 */
	class E2_API RawMemoryStream : public e2::IStream
	{
	public:
		RawMemoryStream(uint8_t* data, uint64_t size, bool transcode = true);
		~RawMemoryStream() = default;
		virtual bool growable() override;
		virtual uint64_t size() const override;
		virtual uint8_t const* read(uint64_t numBytes) override;
		virtual bool write(uint8_t const* data, uint64_t size) override;

		inline uint8_t* data() const
		{
			return m_data;
		}

	protected:
	
		mutable uint8_t* m_data;
		uint64_t m_size;
	};



	/** 
	 * Allocates memory on the heap using an std::vector, and backs this stream from that
	 * 
	 */
	class E2_API HeapStream : public e2::IStream
	{
	public:
		HeapStream(bool transcode = true, uint64_t slack = 0);
		~HeapStream() = default;

		void reserve(uint64_t extraSlack);

		virtual bool growable() override;
		virtual uint64_t size() const override;
		virtual uint8_t const* read(uint64_t numBytes) override;
		virtual bool write(uint8_t const* data, uint64_t size) override;

	protected:
		mutable std::vector<uint8_t> m_data;
	};

	template<uint64_t byteSize>
	class E2_API StackStream : public e2::IStream
	{
	public:
		StackStream(bool transcode = true)
		{

		}

		~StackStream() = default;

		virtual bool growable() override
		{
			return false;
		}

		virtual uint64_t size() const override
		{
			return byteSize;
		}

		virtual uint8_t const* read(uint64_t numBytes) override
		{
			uint64_t oldCursor;
			if (consume(numBytes, oldCursor))
			{
				return &m_data[oldCursor];
			}

			LogError("failed to read {} bytes from stream", numBytes);

			return nullptr;
		}

		virtual bool write(uint8_t const* data, uint64_t size) override
		{
			uint64_t oldCursor;
			if (consume(size, oldCursor))
			{
				memcpy(&m_data[oldCursor], data, size);
				return true;
			}

			return false;
		}

	protected:
		mutable std::array<uint8_t, byteSize> m_data;
	};


	/**
	 * Filestream
	 */

	enum class FileMode : uint8_t
	{
		WriteMask = 0b0000'0001,
		ReadOnly = 0b0000'0000, //read only
		ReadWrite = 0b0000'0001, // read and write (will create if missing)

		OpenMask = 0b0000'0010,
		Append = 0b0000'0000, // Append to file
		Truncate = 0b0000'0010, // Truncate file 
	};

	class E2_API FileStream : public e2::IStream
	{
	public:
		FileStream(std::string const& path, e2::FileMode mode, bool transcode = true);
		~FileStream();
		virtual bool growable() override;
		virtual uint64_t size() const override;
		virtual uint8_t const* read(uint64_t numBytes) override;
		virtual bool write(uint8_t const* data, uint64_t size) override;

		inline bool valid() const
		{
			return m_valid;
		}

	protected:

		void updateSize();

		std::vector<uint8_t> m_readBuffer;
		mutable std::fstream m_handle;
		uint64_t m_size;
		bool m_valid;
	};


	constexpr e2::FileMode operator|(e2::FileMode lhs, e2::FileMode rhs)
	{
		return static_cast<e2::FileMode>(static_cast<std::underlying_type<e2::FileMode>::type>(lhs) | static_cast<std::underlying_type<e2::FileMode>::type>(rhs));
	}

	constexpr e2::FileMode& operator|=(e2::FileMode& lhs, e2::FileMode const& rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

	constexpr e2::FileMode operator&(e2::FileMode lhs, e2::FileMode rhs)
	{
		return static_cast<e2::FileMode>(static_cast<std::underlying_type<e2::FileMode>::type>(lhs) & static_cast<std::underlying_type<e2::FileMode>::type>(rhs));
	}

	constexpr e2::FileMode operator^(e2::FileMode lhs, e2::FileMode rhs)
	{
		return static_cast<e2::FileMode>(static_cast<std::underlying_type<e2::FileMode>::type>(lhs) ^ static_cast<std::underlying_type<e2::FileMode>::type>(rhs));
	}

}
