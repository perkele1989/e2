
#pragma once 

#include <e2/application.hpp>
#include <e2/hex/hex.hpp>

namespace e2
{
	class Game;
	class GameContext
	{
	public:
		virtual e2::Game* game() = 0;
	};
	
	struct ResourceTable
	{
		int32_t wood{};
		int32_t stone{};
		int32_t metal{};
		int32_t gold{};
		int32_t oil{};
		int32_t uranium{};
		int32_t meteorite{};
	};


	struct GameResources
	{
		void calculateProfits();

		// funds, persistent value
		ResourceTable funds;

		// revenue streams, calculated at start of each round
		ResourceTable revenue;

		// expenses, calculated at the start of each round, can
		ResourceTable expenditures;

		// profits, calculated by (profits = revenue - expenditure)
		ResourceTable profits;
	};

	class GameUnit : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		GameUnit(e2::GameContext* ctx)
			: m_game(ctx->game())
		{

		}

		virtual e2::Game* game() override
		{
			return m_game;
		}

		virtual ~GameUnit();

		glm::ivec2 tileIndex; // offset coords
		float health{ 100.0f };

	protected:
		e2::Game* m_game;
	};

	enum class MilitaryUnitUpgrade : uint16_t
	{
		None = 0,
		Offensive1				= 1 << 0,
		Offensive2				= 1 << 1,
		Offensive3				= 1 << 2,
		OffensiveSpecialist1	= 1 << 3,
		OffensiveSpecialist2	= 1 << 4,

		Defensive1				= 1 << 5,
		Defensive2				= 1 << 6,
		Defensive3				= 1 << 7,
		DefensiveSpecialist1	= 1 << 8,
		DefensiveSpecialist2	= 1 << 9,

		BalancedOffensive		= 1 << 10,
		BalancedDefensive		= 1 << 11,
		Balanced				= 1 << 12,

	};

	EnumFlagsDeclaration(e2::MilitaryUnitUpgrade);

	enum class MilitaryUnitType : uint8_t
	{
		Ranger = 0,
	};


	constexpr uint32_t militaryUnitMaxLevel = 14;

	struct MilitaryUnitTypeStatsTable
	{
		float maxArmor[militaryUnitMaxLevel]{};

		float baseMeleeDamage{};
		float baseRangedDamage{};
		float baseDefensiveStrength{};
		int32_t baseRange{ 1 }; 

	};

	struct MilitaryUnitTypeStatsModifier
	{
		float offsetMaxArmor{};
		float offsetMeleeDamage{};
		float offsetRangedDamage{};
		float offsetDefensiveStrength{};
		int32_t offsetRange{};
	};

	struct MilitaryUnitTypeStats
	{
		void applyTable(MilitaryUnitTypeStatsTable const& table, uint8_t level)
		{
			maxArmor = table.maxArmor[level];

			meleeDamage = table.baseMeleeDamage;
			rangedDamage = table.baseRangedDamage;
			defensiveStrength = table.baseDefensiveStrength;
			range = table.baseRange;

			validate();
		}

		void applyModifier(MilitaryUnitTypeStatsModifier const& modifier)
		{
			maxArmor += modifier.offsetMaxArmor;

			meleeDamage += modifier.offsetMeleeDamage;
			rangedDamage += modifier.offsetRangedDamage;
			defensiveStrength += modifier.offsetDefensiveStrength;
			range += modifier.offsetDefensiveStrength;
			
			validate();
		}

		void validate()
		{
			maxArmor = glm::max(maxArmor, 0.f);
			meleeDamage = glm::max(meleeDamage, 0.0f);
			rangedDamage = glm::max(rangedDamage, 0.0f);
			defensiveStrength = glm::max(defensiveStrength, 0.0f);
			range = glm::max(range, 0);
		}

		float maxArmor{};

		float meleeDamage{};
		float rangedDamage{};
		float defensiveStrength{};
		int32_t range{ };
	};

	class MilitaryUnit : public GameUnit
	{
		ObjectDeclaration();
	public:
		MilitaryUnit() = default;
		virtual ~MilitaryUnit();

		// calculates stats for the given round 
		void calculateStats();
		MilitaryUnitTypeStats calculatedStats;

		MilitaryUnitType unitType;

		float armor{ 0.0f };

		uint8_t level()
		{
			return experience / 1000;
		}

		uint32_t experience{0}; // max 14,000, one level per 1000 maybe
		uint8_t upgradesAvailable{0};
		MilitaryUnitUpgrade upgradeFlags{ e2::MilitaryUnitUpgrade::None };

		bool hasUpgrade(MilitaryUnitUpgrade upgrade)
		{
			return (upgradeFlags & upgrade) == upgrade;
		}

		bool hasAllUpgrades(MilitaryUnitUpgrade upgrades)
		{
			return (upgradeFlags & upgrades) == upgrades;
		}

		bool hasAnyUpgrades(MilitaryUnitUpgrade upgrades)
		{
			return (upgradeFlags & upgrades) != MilitaryUnitUpgrade::None;
		}

		// returns true if the given upgrade can be unlocked
		bool canUpgrade(MilitaryUnitUpgrade upgrade)
		{
			if (hasUpgrade(upgrade))
				return false;

			bool hasAnyBalanced = hasAnyUpgrades(MilitaryUnitUpgrade::BalancedDefensive | MilitaryUnitUpgrade::BalancedOffensive | MilitaryUnitUpgrade::Balanced);
			bool hasAnyOffensive = hasAnyUpgrades(MilitaryUnitUpgrade::OffensiveSpecialist1 | MilitaryUnitUpgrade::OffensiveSpecialist2);
			bool hasAnyDefensive = hasAnyUpgrades(MilitaryUnitUpgrade::DefensiveSpecialist1 | MilitaryUnitUpgrade::DefensiveSpecialist2);
			
			bool offensiveLocked = hasAnyDefensive || hasAnyBalanced;
			bool defensiveLocked = hasAnyOffensive || hasAnyBalanced;
			bool balancedLocked = hasAnyOffensive || hasAnyDefensive;

			switch (upgrade)
			{
			case MilitaryUnitUpgrade::Offensive1:
				return true;
			case MilitaryUnitUpgrade::Offensive2:
				return hasUpgrade(MilitaryUnitUpgrade::Offensive1);
			case MilitaryUnitUpgrade::Offensive3:
				return hasUpgrade(MilitaryUnitUpgrade::Offensive2);
			case MilitaryUnitUpgrade::OffensiveSpecialist1:
				return !offensiveLocked;
			case MilitaryUnitUpgrade::OffensiveSpecialist2:
				return hasUpgrade(MilitaryUnitUpgrade::Offensive1);
			case MilitaryUnitUpgrade::Defensive1:
				return true;
			case MilitaryUnitUpgrade::Defensive2:
				return hasUpgrade(MilitaryUnitUpgrade::Defensive1);
			case MilitaryUnitUpgrade::Defensive3:
				return hasUpgrade(MilitaryUnitUpgrade::Defensive2);
			case MilitaryUnitUpgrade::DefensiveSpecialist1:
				return !defensiveLocked;
			case MilitaryUnitUpgrade::DefensiveSpecialist2:
				return hasUpgrade(MilitaryUnitUpgrade::DefensiveSpecialist1);
			case MilitaryUnitUpgrade::BalancedDefensive:
			case MilitaryUnitUpgrade::BalancedOffensive:
				return !balancedLocked;
			case MilitaryUnitUpgrade::Balanced:
				return hasAllUpgrades(MilitaryUnitUpgrade::BalancedDefensive | MilitaryUnitUpgrade::BalancedOffensive);
			}
		}
	};

	class Game : public e2::Application, public e2::GameContext
	{
	public:

		Game(e2::Context* ctx);
		virtual ~Game();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;


		virtual ApplicationType type() override;

		virtual e2::Game* game() override;

		void drawStatusUI();
		void drawDebugUI();

		e2::RenderView calculateRenderView(glm::vec2 const& viewOrigin);

	protected:
		// main world grid
		e2::HexGrid* m_hexGrid{};

		// better make it uint64, who knows how autistic my player-base will be
		uint64_t m_turn{};

		// game economy
		void updateResources();
		GameResources m_resources;
		e2::Texture2DPtr m_uiTextureResources;

		
		glm::vec2 m_cursor; // mouse position in pixels, from topleft corner
		glm::vec2 m_cursorUnit; // mouse position scaled between 0.0 - 1.0
		glm::vec2 m_cursorNdc; // mouse position scaled between -1.0 and 1.0
		glm::vec2 m_cursorPlane; // mouse position as projected on to the world xz plane
		e2::Hex m_cursorHex; // mouse position as projected upon a hex
		e2::MeshPtr m_cursorMesh;
		e2::MeshProxy* m_cursorProxy{};

		// camera stuff 
		void updateMainCamera(double seconds);
		e2::RenderView m_view;

		// stuff to navigate camera main view by dragging
		glm::vec2 m_cursorDragOrigin;
		glm::vec2 m_viewDragOrigin;
		e2::RenderView m_dragView;

		e2::Viewpoints2D m_viewPoints;
		glm::vec2 m_viewOrigin{ 0.0f, 0.0f };
		float m_viewZoom{0.0f};
		glm::vec2 m_viewVelocity{};
		void updateAltCamera(double seconds);
		// alt view lets you fly around freely, without affecting world streaming
		bool m_altView = false;
		glm::vec3 m_altViewOrigin{ 0.0f, 0.0f, 0.0f };
		float m_altViewYaw = 0.0f;
		float m_altViewPitch = 0.0f;
		// end camera stuff
	};
}

#include "game.generated.hpp"