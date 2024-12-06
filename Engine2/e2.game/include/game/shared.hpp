#pragma once 

#include <cstdint>
#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/ui/uitypes.hpp>

namespace e2 
{
	class Entity;
	//class TurnbasedEntity;

    //constexpr uint64_t maxNumEmpires = 256;
    //using EmpireId = uint64_t;

	enum class EntityMoveType : uint8_t
	{
		Static,
		Linear,
		Smooth
	};

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

	//enum class CursorMode : uint8_t
	//{
	//	Select,
	//	UnitMove,
	//	UnitAttack
	//};

	//enum class GameState : uint8_t
	//{
	//	TurnPreparing,
	//	Turn,
	//	TurnEnding
	//};

	enum class TurnState : uint8_t
	{
		//Auto,
		Unlocked,
		//UnitAction_Move,
		//EntityAction_Generic,
		//EntityAction_Target,
		//WavePreparing,
		//Wave,
		//WaveEnding,
		Realtime
	};

	//enum class EntityLayerIndex : uint8_t
	//{
	//	Unit = 0,
	//	Structure,
	//	Air,
	//	Count
	//};

	//EnumFlagsDeclaration(EntityLayerIndex);

	//struct EntityLayer
	//{
	//	std::unordered_map<glm::ivec2, e2::TurnbasedEntity*> entityIndex;
	//};


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

	constexpr uint64_t maxNumHitLabels = 128; 
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

	struct SweepResult
	{
		bool hit{};
		float moveDistance{};
		glm::vec2 hitNormal{};
		glm::vec2 hitLocation{};
		glm::vec2 stopLocation{};

		glm::vec2 obstacleLocation{};
		float obstacleRadius{};
	};

	struct OverlapResult
	{
		bool overlaps{};
		glm::vec2 overlapDirection{};
		float overlapDepth{};
	};

	SweepResult circleSweepTest(glm::vec2 const& startLocation, glm::vec2 const& endLocation, float sweepRadius, glm::vec2 const& obstacleLocation, float obstacleRadius);

	OverlapResult circleOverlapTest(glm::vec2 const& location1, float radius1, glm::vec2 const& location2, float radius2);

	static const e2::UIColor gameAccent{ 0xf59b14FF };
	static const e2::UIColor gameAccentHalf{ 0xf59b147F };
	static const e2::UIColor gamePanelDark{ 0x1d2531FF };
	static const e2::UIColor gamePanelDarkHalf{ 0x1d25317F };
	static const e2::UIColor gamePanelLight{ 0x425168FF };
	static const e2::UIColor gamePanelLightHalf{ 0x4251687F };


	enum class CollisionType : uint8_t
	{
		All = 0b0000'1111,

		Water = 0b0000'0001,
		Mountain = 0b0000'0010,
		Tree = 0b0000'0100,
		Component = 0b0000'1000,
		Land = 0b0001'0000
	};


	EnumFlagsDeclaration(CollisionType);


	class CollisionComponent;

	struct Collision
	{
		CollisionType type{ CollisionType::Water };

		glm::vec2 position;
		glm::ivec2 hex;
		float radius{ 0.0f };

		e2::CollisionComponent* component{};
		int32_t treeIndex{ -1 };
	};
}