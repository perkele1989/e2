
#pragma once 

#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/ui/uicontext.hpp>

#include <game/gamecontext.hpp>
#include "game/shared.hpp"



namespace e2
{
	class MeshProxy;

	/** Base for something that lives on the hex grid, and spreads visibility, and is owned by an empire */
	class GameEntity : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameEntity(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empireId);
		virtual ~GameEntity();

		virtual e2::Game* game() override
		{
			return m_game;
		}

		void spreadVisibility();
		void rollbackVisibility();

		virtual void onTurnEnd();
		virtual void onTurnStart();

		virtual void updateAnimation(double seconds);


		void setMeshTransform(glm::vec3 const& pos, float angle);


		std::string displayName{ "Entity" };
		uint32_t sightRange{ 5 };
		glm::ivec2 tileIndex;
		EmpireId empireId;

		glm::vec2 planarCoords();

	protected:
		e2::Game* m_game{};

		e2::MeshPtr m_mesh;

		void buildProxy();
		void destroyProxy();

		e2::MeshProxy* m_proxy{};


		glm::quat m_rotation{};
		glm::quat m_targetRotation{};
		glm::vec3 m_position{};



	};

	enum class GameStructureType : uint8_t
	{
		// Primary buildings
		MainOperatingBase = 0,
		ForwardOperatingBase,

		// Unit builders
		Barracks,
		Factory,
		NavalBase,
		Airbase,

		// Supportive buildings
		TechnologyCenter,

		// Improvements 
		SawMill, 
		GoldMine, 
		Quarry, 
		OreMine, 
		OilWell,
		UraniumMine,
		

		Count
	};

	class GameStructure : public e2::GameEntity
	{
		ObjectDeclaration();
	public:
		GameStructure(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId);
		virtual ~GameStructure();

		float health{ 100.0f };
		
		GameStructureType structureType{ e2::GameStructureType::MainOperatingBase };
		
		virtual void drawUI(e2::UIContext* ctx);

	};

	// we say mine but really a sawmill, mine or quarry. generic resource improevment
	class Mine : public e2::GameStructure
	{
		ObjectDeclaration();
	public:
		Mine(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId);
		virtual ~Mine();

		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;
	};




	enum class GameUnitType : uint8_t
	{
		// base builder/worker unit 
		Engineer = 0,

		// tier1 infantry units, offensive 
		// all of these can upgrade the Signaleer perk (lets you call in airstrikes from nearby air and artillery units)
		Grunt, //base melee tank, short range, low unit damage, high defensive strength
		Ranger, // ranged unit, mid range, high unit damage, low defensive strength
		Operator, // stealth unit, long range, mid unit damage, low defensive strength 

		// tier2 infantry units, supportiev
		MachineGunner, // mid range, mid and wide suppressive unit damage, mid defensive strength
		Grenadier, // short range, high unit damage, mid defensive strength
		AntiTank, // mid range, high vehicle damage, low unit damage, mid defensive strength

		// tier1 vehicle units , offensive
		APC, // mid range, high unit damage, low vehicle damage, low defensive strength, can transport up to 6 infantry units
		Tank, // long range, mid unit damage, high vehicle damage, high defensive strength

		//tier2 vehicle units, supportive
		AntiAir, // mid range, high damage, low defensive strength, targets air only 
		Artillery, // very long range, high damage, low defensive strength, callable many times per turn by radio operators
		
		// tier 1 air units, offesnive
		Fighter, //  anti air, can do strikes on infantry units with mid damage 
		Bomber, // anti ground, can do bombing runs on all ground units and structures with high damage

		// tier2 air units, supportive
		Helicopter, // unit transport (up to 7 units), low defensive strength, long move range 
		Drone,		// passive unit, high visibility range, can call in airstrikes from nearby air and artillery units'


		// tier1 naval units, offensive
		AssaultCraft, // stridsbåt, the APC of the sea, mid range, transports up to 5 infantry units, high damage against infantry and other assault craft, low damage against vehicles
		WarShip, // korvett, high damage against anything, very long range, artillery, upgradeable to antiair, detects stealth, the big bad boi of the sea

		// tier2 naval units, supportive 
		Submarine, // stealth, short range, torpedoes
		AircraftCarrier, // transport for air units 

		Count
	};

	class GameUnit : public e2::GameEntity
	{
		ObjectDeclaration();
	public:
		GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId);
		virtual ~GameUnit();

		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;
		virtual void updateAnimation(double seconds) override;

		virtual void drawUI(e2::UIContext* ctx);

		float health{ 100.0f };
		int32_t moveRange{ 3 };
		int32_t movePointsLeft{ 3 };
		
		GameUnitType unitType{ GameUnitType::Engineer };

	protected:

	};
}

#include "gameunit.generated.hpp"