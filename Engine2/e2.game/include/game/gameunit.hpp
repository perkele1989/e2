
#pragma once 

#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/ui/uicontext.hpp>

#include <e2/renderer/meshproxy.hpp>
#include <game/gamecontext.hpp>
#include "game/shared.hpp"



namespace e2
{
	class MeshProxy;






	enum class EntityType : uint8_t
	{
		// base builder/worker unit 
		Unit_Engineer = 0,

		// tier1 infantry units, offensive 
		// all of these can upgrade the Signaleer perk (lets you call in airstrikes from nearby air and artillery units)
		Unit_Grunt, //base melee tank, short range, low unit damage, high defensive strength
		Unit_Ranger, // ranged unit, mid range, high unit damage, low defensive strength
		Unit_Operator, // stealth unit, long range, mid unit damage, low defensive strength 

		// tier2 infantry units, supportiev
		Unit_MachineGunner, // mid range, mid and wide suppressive unit damage, mid defensive strength
		Unit_Grenadier, // short range, high unit damage, mid defensive strength
		Unit_AntiTank, // mid range, high vehicle damage, low unit damage, mid defensive strength

		// tier1 vehicle units , offensive
		Unit_APC, // mid range, high unit damage, low vehicle damage, low defensive strength, can transport up to 6 infantry units
		Unit_Tank, // long range, mid unit damage, high vehicle damage, high defensive strength

		//tier2 vehicle units, supportive
		Unit_AntiAir, // mid range, high damage, low defensive strength, targets air only 
		Unit_Artillery, // very long range, high damage, low defensive strength, callable many times per turn by radio operators

		// tier 1 air units, offesnive
		Unit_Fighter, //  anti air, can do strikes on infantry units with mid damage 
		Unit_Bomber, // anti ground, can do bombing runs on all ground units and structures with high damage

		// tier2 air units, supportive
		Unit_Helicopter, // unit transport (up to 7 units), low defensive strength, long move range 
		Unit_Drone,		// passive unit, high visibility range, can call in airstrikes from nearby air and artillery units'


		// tier1 naval units, offensive
		Unit_AssaultCraft, // stridsbåt, the APC of the sea, mid range, transports up to 5 infantry units, high damage against infantry and other assault craft, low damage against vehicles
		Unit_WarShip, // korvett, high damage against anything, very long range, artillery, upgradeable to antiair, detects stealth, the big bad boi of the sea

		// tier2 naval units, supportive 
		Unit_Submarine, // stealth, short range, torpedoes
		Unit_AircraftCarrier, // transport for air units 

		// Primary buildings
		Structure_MainOperatingBase = 0,
		Structure_ForwardOperatingBase,

		// Unit builders
		Structure_Barracks,
		Structure_Factory,
		Structure_NavalBase,
		Structure_Airbase,

		// Supportive buildings
		Structure_TechnologyCenter,

		// Improvements 
		Structure_SawMill,
		Structure_GoldMine,
		Structure_Quarry,
		Structure_OreMine,
		Structure_OilWell,
		Structure_UraniumMine,

		Count
	};




	/** Base for something that lives on the hex grid, and spreads visibility, and is owned by an empire */
	class GameEntity : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameEntity(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empireId);
		virtual ~GameEntity();

		EntityType entityType;

		virtual e2::Game* game() override
		{
			return m_game;
		}


		virtual void initialize();


		void spreadVisibility();
		void rollbackVisibility();

		virtual void onTurnEnd();
		virtual void onTurnStart();

		virtual void updateAnimation(double seconds);

		virtual void onBeginMove();
		virtual void onEndMove();


		void setMeshTransform(glm::vec3 const& pos, float angle);


		std::string displayName{ "Entity" };
		uint32_t sightRange{ 5 };
		glm::ivec2 tileIndex;
		EmpireId empireId;

		glm::vec2 planarCoords();

	protected:
		e2::Game* m_game{};

		glm::quat m_rotation{};
		glm::quat m_targetRotation{};
		glm::vec3 m_position{};

		glm::vec3 m_modelScale{ glm::vec3(1.0f, -1.0f, -1.0f) / 200.0f };
		e2::MeshPtr m_mesh;
		e2::SkeletonPtr m_skeleton;

		void buildProxy();
		void destroyProxy();

		e2::MeshProxy* m_proxy{};
		e2::SkinProxy* m_skinProxy{};

		void setCurrentPose(e2::Pose* pose, float lerpTime);
		void playAction(e2::AnimationPose* anim, float blendIn = 0.2f, float blendOut = 0.2f);

		e2::Pose* m_mainPose{};

		float m_lerpTime{};
		e2::Moment m_lastChangePose;
		e2::Pose* m_currentPose;
		e2::Pose* m_oldPose;

		e2::AnimationPose* m_actionPose;
		float m_actionBlendInTime = 0.2f;
		float m_actionBlendOutTime = 0.2f;
	};


	class GameStructure : public e2::GameEntity
	{
		ObjectDeclaration();
	public:
		GameStructure(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId);
		virtual ~GameStructure();

		float health{ 100.0f };
		
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






	class GameUnit : public e2::GameEntity
	{
		ObjectDeclaration();
	public:
		GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId);
		virtual ~GameUnit();

		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

		virtual void updateUnitAction(double seconds);

		virtual void drawUI(e2::UIContext* ctx);

		float health{ 100.0f };
		int32_t moveRange{ 3 };
		int32_t movePointsLeft{ 3 };

	protected:

	};
}

#include "gameunit.generated.hpp"