
#include "e2/log.hpp"

#include <mutex>

#include <iostream>
#include <fstream>
#include <filesystem>

namespace
{
	std::recursive_mutex logMutex;
	std::ofstream outHandle;
}

std::string e2::Log::formatFilepath(std::string const& filePath)
{
	return std::filesystem::path(filePath).filename().string();
}

namespace
{
	std::vector<e2::LogEntry> logEntries;
}

void e2::Log::write(e2::Severity severity, std::string const& function, std::string const& file, uint32_t line, std::string const& message)
{
	std::scoped_lock lock(::logMutex);

	e2::LogEntry newEntry;
	newEntry.severity = severity;
	newEntry.function = function;
	newEntry.file = file;
	newEntry.line = line;
	newEntry.message = message;

	logEntries.push_back(newEntry);

	std::string severityStr;
	if (severity == Severity::Notice)
		severityStr = "Notice";
	else if (severity == Severity::Warning)
		severityStr = "Warning";
	else if (severity == Severity::Error)
		severityStr = "Error";

	std::string output = std::format("{} in {} ({}:{}): {}", severityStr, function, e2::Log::formatFilepath(file), line, message);

	std::cout << output << std::endl;

	if (!outHandle.is_open())
	{
		outHandle.open("e2.log");
	}

	outHandle << output << std::endl;
}

uint32_t e2::Log::numEntries()
{
	return ::logEntries.size();
}

e2::LogEntry const& e2::Log::getEntry(uint32_t id)
{
	return ::logEntries[id];
}

void e2::Log::clear()
{
	::logEntries.clear();
}

std::recursive_mutex& e2::Log::getMutex()
{
	return ::logMutex;
}
