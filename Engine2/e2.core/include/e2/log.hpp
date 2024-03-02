
#pragma once

#include <e2/export.hpp>

#include <string>
#include <format>

//#if defined(E2_DEVELOPMENT)
#define LogNotice(x, ...) e2::Log::write( e2::Severity::Notice, __func__, __FILE__, __LINE__, std::format(x, __VA_ARGS__))
#define LogWarning(x, ...) e2::Log::write( e2::Severity::Warning, __func__, __FILE__, __LINE__, std::format(x, __VA_ARGS__))
#define LogError(x, ...) e2::Log::write( e2::Severity::Error, __func__, __FILE__, __LINE__, std::format(x, __VA_ARGS__))
//#else 
//#define LogNotice(...)
//#define LogWarning(...)
//#define LogError(...)
//#endif

namespace e2
{

	enum class Severity : uint8_t
	{
		Notice = 0,
		Warning,
		Error
	};

	struct E2_API LogEntry
	{
		e2::Severity severity{ e2::Severity::Notice };
		std::string function;
		std::string file;
		uint32_t line{};

		std::string message;
	};


	class E2_API Log
	{
	public:
		static std::string formatFilepath(std::string const& filePath);

		static void write(e2::Severity severity, std::string const& function, std::string const& file, uint32_t line, std::string const& message);
	
		static uint32_t numEntries();
		static e2::LogEntry const& getEntry(uint32_t id);

		static void clear();

		static std::recursive_mutex& getMutex();

	};

}


