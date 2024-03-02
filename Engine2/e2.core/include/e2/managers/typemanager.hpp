
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <e2/utils.hpp>

#include <e2/game/meshcomponent.hpp>
 
namespace e2
{ 
	class Engine;
	class Session; 

	/** Handles reflection, global arenas for special types etc. */
	class E2_API TypeManager : public Manager
	{
	public:
		TypeManager(Engine* owner);
		virtual ~TypeManager();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double deltaTime) override;




	protected:

	};


}
