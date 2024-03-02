
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>

#include <string>
#include <vector>

namespace e2
{
	class Context;
	class Engine;

	enum class ApplicationType : uint8_t
	{
		Game = 0,
		Tool
	};

	class E2_API Application : public virtual e2::Context
	{
	public:
		Application(e2::Context* ctx);
		virtual ~Application();

		virtual void initialize() =0;
		virtual void shutdown() = 0;
		virtual void update(double seconds) = 0;

		virtual ApplicationType type() = 0;

		virtual Engine* engine() override;

	protected:
		e2::Engine* m_engine{};


	};
}