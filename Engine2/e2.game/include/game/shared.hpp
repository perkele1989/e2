#pragma once 

#include <cstdint>
#include <e2/utils.hpp>
#include <e2/timer.hpp>

namespace e2 
{
	class GameEntity;

    constexpr uint64_t maxNumEmpires = 256;
    using EmpireId = uint8_t;

	enum class PassableFlags : uint8_t
	{
		None  = 0b0000'0000,

		Land = 0b0000'0001,
		WaterShallow = 0b0000'0010,
		WaterDeep = 0b0000'0100,
		Mountain = 0b0000'1000,

		Air = 0b0000'1111,
	};

	EnumFlagsDeclaration(PassableFlags);


	enum class MainMenuState : uint8_t
	{
		Main,
		Load,
		Options,
		Credits
	};

	enum class InGameMenuState : uint8_t
	{
		Main,
		Save,
		Load,
		Options
	};

	enum class GlobalState : uint8_t
	{
		Boot,
		Menu,
		Game,
		InGameMenu
	};

	enum class CursorMode : uint8_t
	{
		Select,
		UnitMove,
		UnitAttack
	};

	enum class GameState : uint8_t
	{
		TurnPreparing,
		Turn,
		TurnEnding,
	};

	enum class TurnState : uint8_t
	{
		Unlocked,
		UnitAction_Move,
		EntityAction_Generic,
		EntityAction_Target
	};

	enum class EntityLayerIndex : uint8_t
	{
		Unit = 0,
		Structure,
		Air,
		Count
	};

	EnumFlagsDeclaration(EntityLayerIndex);

	struct EntityLayer
	{
		std::unordered_map<glm::ivec2, e2::GameEntity*> entityIndex;
	};


	struct SaveMeta
	{
		bool exists{};

		uint8_t slot{};
		std::time_t timestamp;

		std::string cachedDisplayName;
		std::string cachedFileName;

		std::string displayName();
		std::string fileName();
	};

	constexpr uint64_t numSaveSlots = 8;

	constexpr uint64_t maxNumHitLabels = 16;
	struct HitLabel
	{
		HitLabel() = default;
		HitLabel(glm::vec3 const& worldOffset, std::string const& inText);

		bool active{};
		e2::Moment timeCreated;
		std::string text;
		glm::vec3 offset;
		glm::vec3 velocity;
	};

}