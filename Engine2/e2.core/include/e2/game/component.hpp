
#pragma once 

#include <e2/context.hpp>
#include <e2/game/worldcontext.hpp>

#include <e2/utils.hpp>
#include <string>

namespace e2
{
	class Entity;

	/** @tags() */
	class E2_API Component : public e2::WorldContext, public e2::Object
	{
		ObjectDeclaration()
	public:

		Component(e2::Entity* entity, e2::Name name);
		virtual ~Component();

		virtual void postConstruct(e2::Entity* entity, e2::Name name);

		virtual void onSpawned();
		virtual void onDestroyed();

		virtual e2::World* world() override;

		virtual void tick(double seconds);
	protected:

		friend class World;
		friend class Entity;

		e2::Name m_name;
		e2::Entity* m_entity{};
	};
}

#include <component.generated.hpp>