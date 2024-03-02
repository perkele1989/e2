
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <glm/glm.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <map>

namespace e2
{

	class E2_API IInputContext : public Context
	{
	public:
		IInputContext(e2::Context* engineContext);
		virtual ~IInputContext();

		virtual Engine* engine() override;

	protected:
		e2::Engine* m_engine{};


	};

}