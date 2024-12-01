
#pragma once 

#include "game/gamecontext.hpp"

#include <e2/utils.hpp>

#include <unordered_set>

namespace e2
{
	class RadionEntity;

	class RadionManager : public e2::GameContext
	{
	public:
		RadionManager(e2::Game* g);
		~RadionManager();

		void update(double seconds);
		virtual e2::Game* game() override;

		void registerEntity(e2::RadionEntity* ent);
		void unregisterEntity(e2::RadionEntity* ent);

		void discoverEntity(e2::Name name);
		uint32_t numDiscoveredEntities();
		e2::Name discoveredEntity(uint32_t index);

	protected:

		e2::Game* m_game{};

		double m_timeAccumulator{};

		std::unordered_set<e2::RadionEntity*> m_entities;
		std::vector<e2::Name> m_discoveredEntities;

	};

}