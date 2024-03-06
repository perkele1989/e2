
#pragma once

#include "game/gameunit.hpp"

namespace e2
{

	enum class MilitaryUnitUpgrade : uint16_t
	{
		None = 0,
		Offensive1 = 1 << 0,
		Offensive2 = 1 << 1,
		Offensive3 = 1 << 2,
		OffensiveSpecialist1 = 1 << 3,
		OffensiveSpecialist2 = 1 << 4,

		Defensive1 = 1 << 5,
		Defensive2 = 1 << 6,
		Defensive3 = 1 << 7,
		DefensiveSpecialist1 = 1 << 8,
		DefensiveSpecialist2 = 1 << 9,

		BalancedOffensive = 1 << 10,
		BalancedDefensive = 1 << 11,
		Balanced = 1 << 12,

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

	/** @tags(arena, arenaSize=256)*/
	class MilitaryUnit : public e2::GameUnit
	{
		ObjectDeclaration();
	public:
		MilitaryUnit(e2::GameContext* ctx, glm::ivec2 const& tile);
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

		uint32_t experience{ 0 }; // max 14,000, one level per 1000 maybe
		uint8_t upgradesAvailable{ 0 };
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
}

#include "militaryunit.generated.hpp"