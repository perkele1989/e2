
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
	m_attackPoints = 1;
	m_attackPointsLeft = m_attackPoints;



}

e2::Grunt::~Grunt()
{
	if(m_idlePose)
		e2::destroy(m_idlePose);

	if(m_runPose)
		e2::destroy(m_runPose);

	if (m_hitPose)
		e2::destroy(m_hitPose);

	if (m_firePose)
		e2::destroy(m_firePose);
}

void e2::Grunt::initialize()
{
	e2::MilitaryUnit::initialize();
	m_idlePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierIdle), true);
	m_runPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierRun), true);
	m_firePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierFire), false);
	m_hitPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierHit), false);
	m_diePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierDie), false);

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;

}

void e2::Grunt::updateAnimation(double seconds)
{
	if (dying && (!m_diePose->playing() || m_diePose->time() > m_diePose->animation()->timeSeconds() - 0.05f))
	{
		game()->queueDestroyUnit(this);
		return;
	}

	m_idlePose->updateAnimation(seconds);
	m_runPose->updateAnimation(seconds);
	m_firePose->updateAnimation(seconds);
	m_hitPose->updateAnimation(seconds);
	m_diePose->updateAnimation(seconds);


	e2::MilitaryUnit::updateAnimation(seconds);
}

void e2::Grunt::drawUI(e2::UIContext* ui)
{
	e2::MilitaryUnit::drawUI(ui);

	e2::TileData* tileData = game()->hexGrid()->getTileData(tileIndex);
	if (!tileData)
		return;

	e2::TileFlags resource = tileData->getResource();

	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->gameLabel(std::format("Movement points left: {}", movePointsLeft), 11);

	//ui->beginStackV("actions");


	if (ui->button("att", "Attack"))
	{
		game()->beginEntityTargeting();
	}


	//ui->endStackV();

	ui->endStackV();
}

void e2::Grunt::onBeginMove()
{
	setCurrentPose(m_runPose, 0.2f);
}

void e2::Grunt::onEndMove()
{
	setCurrentPose(m_idlePose, 0.2f);
}
void e2::Grunt::kill()
{
	e2::MilitaryUnit::kill();

	playAction(m_diePose, 0.2f, 0.0f);
}


void e2::Grunt::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::MilitaryUnit::onHit(instigator, dmg);

	if (health <= 0.0f)
	{
		kill();
	}
	else
	{
		playAction(m_hitPose, 0.2f, 0.2f);
	}

	
}

void e2::Grunt::updateEntityAction(double seconds)
{
	if (m_fireBegin.durationSince().seconds() > 0.5f && !m_didFire)
	{
		playAction(m_firePose);
		m_didFire = true;
	}

	if (m_didFire && !m_firePose->playing())
	{
		game()->applyDamage(m_targetUnit, this, 50.0f);

		{
			HitLabel& newLabel = labels[labelIndex++];
			newLabel.active = true;
			newLabel.timeCreated = e2::timeNow();
			newLabel.offset = game()->worldToPixels(e2::Hex(m_targetUnit->tileIndex).localCoords()+ glm::vec3(e2::worldUp() * 0.5));
			newLabel.velocity = { 50.0f, -200.0f };
			newLabel.text = std::format("{:.0f}", 50.0f);
			if (labelIndex >= 3)
				labelIndex = 0;
		}


		game()->endCustomEntityAction();
		m_didFire = false;
	}
}

void e2::Grunt::onEntityTargetChanged(e2::Hex const& location)
{
	m_targetUnit = game()->unitAtHex(location.offsetCoords());
}

void e2::Grunt::onEntityTargetClicked()
{
	if (!m_targetUnit || m_targetUnit == this || m_targetUnit->dying)
	{
		game()->endEntityTargeting();
		return;
	}

	game()->endEntityTargeting();
	game()->beginCustomEntityAction();
	m_fireBegin = e2::timeNow();

	float angle = e2::radiansBetween(e2::Hex(m_targetUnit->tileIndex).localCoords(), e2::Hex(tileIndex).localCoords());
	m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
}

void e2::Grunt::onTurnStart()
{
	e2::MilitaryUnit::onTurnStart();
}

void e2::Grunt::onTurnEnd()
{
	e2::MilitaryUnit::onTurnEnd();
}
