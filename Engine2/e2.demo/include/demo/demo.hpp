
#pragma once 

#include <e2/export.hpp>
#include <e2/application.hpp>

#include <set>

namespace e2
{
	
	class Demo : public e2::Application
	{
	public:

		Demo(e2::Context* ctx);
		virtual ~Demo();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;


		virtual ApplicationType type() override;


	protected:

	};
}