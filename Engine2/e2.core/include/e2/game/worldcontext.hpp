
#pragma once

#include <e2/export.hpp>
#include <e2/context.hpp>

namespace e2
{
	class World;
	class Engine;
	class Session;

	class E2_API WorldContext : public  e2::Context
	{
	public:
		virtual e2::Engine* engine() override;
		virtual e2::World* world() = 0;


		e2::Session* worldSession();
	};

}