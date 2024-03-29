
#pragma once

#include "game/gameunit.hpp"
#include <e2/renderer/meshproxy.hpp>


namespace e2
{

	enum class EngineerBuildType : uint8_t
	{
		HarvestWood,
		SpawnEntity,
		SpawnWaterEntity
	};

	/** @tags(arena, arenaSize=4096)*/
	class Engineer : public e2::GameUnit
	{
		ObjectDeclaration();
	public:
		Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~Engineer();
		virtual void updateAnimation(double seconds) override;
		virtual void drawUI(e2::UIContext* ctx) override;

		virtual void initialize() override;

		virtual void updateEntityAction(double seconds) override;

		virtual void onBeginMove() override;
		virtual void onEndMove() override;

		virtual void onTurnEnd() override;
		virtual void onTurnStart() override;

		virtual void onHit(e2::GameEntity* instigator, float dmg) override;
		virtual void kill() override;


	protected:
		e2::AnimationPose* m_idlePose{};
		e2::AnimationPose* m_runPose{};

		e2::AnimationPose* m_hitPose{};
		e2::AnimationPose* m_diePose{};

		e2::AnimationPose* m_buildPose{};

		e2::EngineerBuildType m_buildType{};
		e2::EntityType m_spawnEntityType{};
		e2::Hex m_waterHex{};

		uint32_t m_buildPointsLeft = 1;
		uint32_t m_buildPoints = 1;
	};
}

#include "builderunit.generated.hpp"