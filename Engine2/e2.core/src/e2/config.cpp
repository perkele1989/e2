
#include "e2/config.hpp"
#include "e2/log.hpp"
#include "e2/utils.hpp"

#include <fstream>
#include <sstream>

e2::Config::Config()
{

}

e2::Config::~Config()
{

}

bool e2::Config::load(std::string const& filename)
{
	std::ifstream handle(filename);
	if (!handle.is_open())
	{
		LogError("failed to open file {}", filename);
		return false;
	}

	m_variables = m_defaultVariables;

	std::string line;
	while (std::getline(handle, line))
	{
		auto parts = e2::split(line, '=');
		if (parts.size() != 2)
			continue;

		m_variables[parts[0]] = parts[1];
	}
	return true;
}

bool e2::Config::save(std::string const& filename)
{
	return true;
}

void e2::Config::registerDefault(std::string const& name, std::string const& value)
{
	m_defaultVariables[name] = value;
}

std::string e2::Config::get(std::string const& name, std::string const& fallback)
{
	auto finder = m_variables.find(name);
	if (finder == m_variables.end())
	{
		return fallback;
	}

	return finder->second;
}

void e2::Config::set(std::string const& name, std::string const& value)
{
	m_variables[name] = value;
}
