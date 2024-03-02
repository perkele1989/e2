
#pragma once 

#include <e2/context.hpp>
#include <e2/game/worldcontext.hpp>
#include <e2/game/entity.hpp>
#include <e2/utils.hpp>
#include <string>
#include <unordered_set>
#include <mutex>
#include <thread>

namespace e2
{
	class Session;
	class Entity;
	class Component;

	/** @tags(arena, arenaSize=64)*/
	class World : public e2::Context, public e2::Object
	{
		ObjectDeclaration()
	public:
		World(e2::Session *session);
		virtual ~World();

		virtual e2::Engine* engine() override;

		void tick(double seconds);

		template<typename EntityType>
		EntityType* spawnEntity(e2::Name name)
		{
			EntityType* newEntity = e2::create<EntityType>(this, name);
			m_newEntities.insert(newEntity);
			return newEntity;
		}

		e2::Entity* spawnEntity(e2::Name name, e2::Name typeName);

		e2::Session* worldSession();

	protected:
		friend e2::Entity;


		e2::Session* m_session{};

		std::unordered_set<e2::Entity*> m_entities;
		std::unordered_set<e2::Entity*> m_newEntities;
		std::unordered_set<e2::Entity*> m_destroyedEntities;
	};

}

#include "world.generated.hpp"