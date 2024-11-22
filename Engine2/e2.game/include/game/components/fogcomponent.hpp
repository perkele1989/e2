#pragma once 

#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/assets/mesh.hpp>
#include <nlohmann/json.hpp>



namespace e2
{

	class GameContext;

	class Entity;

	class FogComponent
	{
	public:
		FogComponent(e2::Entity* entity, uint32_t sightRange, uint32_t extraRange);
		~FogComponent();

		void refresh();

		inline void setRange(uint32_t sightRange, uint32_t extraRange)
		{
			m_sightRange = sightRange;
			m_extraRange = extraRange;
		}

	protected:

		e2::Entity* m_entity{};

		e2::Hex m_lastHex{-999999, -999999};
		std::vector<e2::Hex> m_visibilityHexes;
		uint32_t m_sightRange{ 2 };
		uint32_t m_extraRange{ 1 };
	};


}
