
#pragma once 

#include <e2/context.hpp>
#include <e2/game/worldcontext.hpp>
#include <e2/game/component.hpp>
#include <e2/transform.hpp>
#include <e2/utils.hpp>

#include <string>
#include <mutex>
#include <unordered_set>

namespace e2
{
	class Component;
	class World;

	/** @tags(arena, arenaSize=8192) */
	class Entity : public e2::WorldContext, public e2::Object
	{
		ObjectDeclaration()
	public:
		Entity();
		Entity(e2::World* world, e2::Name name);
		virtual ~Entity();

		virtual void postConstruct(e2::World* world, e2::Name name);

		virtual void onSpawned();
		virtual void onDestroyed();

		virtual e2::World* world() override;

		virtual void tick(double seconds);

		inline e2::Transform* getTransform() const
		{
			return m_transform;
		}

		template<typename ComponentType>
		ComponentType* spawnComponent(e2::Name name)
		{
			ComponentType* newComponent = e2::create<ComponentType>(this, name);
			m_newComponents.insert(newComponent);
			return newComponent;
		}

		e2::Component* spawnComponent(e2::Name name, e2::Name typeName);


	protected:

		friend class World;

		e2::Name m_name;
		e2::World* m_world{};

		e2::Transform* m_transform{};

		// @todo optimize this bigly. We can store components in the world instead. Also do we need 3 sets?
		std::unordered_set<e2::Component*> m_components;
		std::unordered_set<e2::Component*> m_newComponents;
		std::unordered_set<e2::Component*> m_destroyedComponents;
	};

}

#include <entity.generated.hpp>