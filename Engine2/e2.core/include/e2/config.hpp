
#pragma once 

#include <map>
#include <string>

#include <e2/utils.hpp>

namespace e2
{

	struct ConfigValue
	{

	};

	struct ConfigVar
	{
		e2::Name name;
		e2::StackVector<uint8_t, 64> value;
		e2::StackVector<uint8_t, 64> defaultValue;
	};

	class Config
	{
	public:
		Config();
		virtual ~Config();

		bool load(std::string const& filename);
		bool save(std::string const& filename);

		void registerDefault(std::string const& name, std::string const& value);

		std::string get(std::string const& name, std::string const& fallback);
		void set(std::string const& name, std::string const& value);

	protected:
		// @todo do we need maps here?
		std::map<std::string, std::string> m_defaultVariables;

		std::map<std::string, std::string> m_variables;
	};

}