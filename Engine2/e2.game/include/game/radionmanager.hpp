
#pragma once 

#include "game/gamecontext.hpp"
#include "game/entities/radionentity.hpp"
#include <e2/utils.hpp>

#include <unordered_set>

namespace e2
{
	class RadionEntity;

	struct RadionWorldPin
	{
		e2::RadionEntity* entity{};
		e2::Name name;
		uint32_t index;
		e2::RadionPinType type;
		glm::vec3 worldOffset;

	};

	class RadionManager : public e2::GameContext
	{
	public:
		RadionManager(e2::Game* g);
		~RadionManager();

		void clearDiscovered();

		void populate(e2::ALJDescription &alj);
		void finalize();

		void update(double seconds);
		virtual e2::Game* game() override;

		void registerEntity(e2::RadionEntity* ent);
		void unregisterEntity(e2::RadionEntity* ent);

		void discoverEntity(e2::Name name);
		uint32_t numDiscoveredEntities();
		e2::Name discoveredEntity(uint32_t index);

		void populatePins(e2::Hex const& coords, e2::RadionPinType type, std::vector<e2::RadionWorldPin> &outPins);

		e2::MaterialProxy* glowProxy();
		e2::MaterialProxy* signalProxy();
		e2::MaterialProxy* unglowProxy();

		void writeForSave(e2::IStream& toBuffer);
		void readForSave(e2::IStream& fromBuffer);

	protected:
		
		void tickWithParents(e2::RadionEntity* node);

		e2::Game* m_game{};

		e2::MaterialPtr m_glowMaterial;
		e2::MaterialPtr m_signalMaterial;
		e2::MaterialPtr m_unglowMaterial;
		e2::MaterialProxy* m_glowProxy{};
		e2::MaterialProxy* m_signalProxy{};
		e2::MaterialProxy* m_unglowProxy{};

		std::unordered_set<e2::RadionEntity*> m_endNodes;
		std::unordered_set<e2::RadionEntity*> m_tickedNodes;
		std::unordered_set<e2::RadionEntity*> m_untickedNodes;

		double m_timeAccumulator{};

		std::unordered_set<e2::RadionEntity*> m_entities;
		std::vector<e2::Name> m_discoveredEntities;

	};

}