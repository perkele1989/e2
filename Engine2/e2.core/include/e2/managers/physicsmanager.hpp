
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/manager.hpp>
#include <e2/utils.hpp>

#include <e2/physics/physicsworld.hpp>

namespace e2
{
	class Engine;
	class IPhysicsContext;
	class IPhysicsWorld;

	class E2_API PhysicsManager : public Manager
	{
	public:
		PhysicsManager(Engine* owner);
		virtual ~PhysicsManager();

		virtual void initialize() override;
		virtual void shutdown() override;

		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

		e2::IPhysicsWorld* createWorld(e2::PhysicsWorldCreateInfo const& info);
		void destroyWorld(e2::IPhysicsWorld* world);

	protected:
		e2::IPhysicsContext* m_physicsContext{};
		e2::StackVector<e2::IPhysicsWorld*, e2::maxNumPhysicsWorlds> m_worlds;
	};

}