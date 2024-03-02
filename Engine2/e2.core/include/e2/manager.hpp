
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>

namespace e2
{

	class E2_API Manager : public Context
	{
	public:
		Manager(Engine* owner);
		virtual ~Manager();

		virtual void initialize() = 0;
		virtual void shutdown() = 0;

		virtual void update(double deltaTime) = 0;

		virtual Engine* engine() override;

	protected:
		Engine* m_owner = nullptr;
	};

}