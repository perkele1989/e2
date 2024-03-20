
#include "game/militaryunit.hpp"
#include "game/game.hpp"


#include <e2/game/gamesession.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/renderer.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

e2::MilitaryUnit::MilitaryUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	displayName = "Ranger";
	sightRange = 5;

	entityType = e2::EntityType::Unit_Ranger;
}

e2::MilitaryUnit::~MilitaryUnit()
{

}

void e2::MilitaryUnit::calculateStats()
{

}

e2::Grunt::Grunt(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::MilitaryUnit(ctx, tile, empire)
{
	displayName = "Grunt";
	sightRange = 5;
	moveRange = 5;
	movePointsLeft = 5;

	entityType = EntityType::Unit_Grunt;




}

e2::Grunt::~Grunt()
{
	if(m_idlePose)
		e2::destroy(m_idlePose);

	if(m_runPose)
		e2::destroy(m_runPose);
}

void e2::Grunt::initialize()
{
	e2::MilitaryUnit::initialize();
	m_idlePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierIdle), true);
	m_runPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierRun), true);

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;

}

void e2::Grunt::updateAnimation(double seconds)
{
	m_idlePose->updateAnimation(seconds);
	m_runPose->updateAnimation(seconds);

	e2::MilitaryUnit::updateAnimation(seconds);
}

void e2::Grunt::drawUI(e2::UIContext* ctx)
{
	e2::MilitaryUnit::drawUI(ctx);
}

void e2::Grunt::onBeginMove()
{
	setCurrentPose(m_runPose, 0.2f);
}

void e2::Grunt::onEndMove()
{
	setCurrentPose(m_idlePose, 0.2f);
}
