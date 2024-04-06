
#include "game/militaryunit.hpp"
#include "game/game.hpp"


#include <e2/e2.hpp>
#include <e2/game/gamesession.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/renderer.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

void e2::MilitaryUnit::setupConfig()
{
	displayName = "Ranger";
	sightRange = 5;

	entityType = e2::EntityType::Unit_Ranger;
}

e2::MilitaryUnit::MilitaryUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	setupConfig();
}

e2::MilitaryUnit::MilitaryUnit()
	: e2::GameUnit()
{
	setupConfig();
}

e2::MilitaryUnit::~MilitaryUnit()
{

}

void e2::MilitaryUnit::calculateStats()
{

}

void e2::Grunt::setupConfig()
{
	displayName = "Grunt";
	sightRange = 5;
	movePoints = 5;
	movePointsLeft = 5;

	entityType = EntityType::Unit_Grunt;
	m_attackPoints = 1;
	m_attackPointsLeft = m_attackPoints;
	m_modelScale = glm::vec3(1.0f, -1.0f, -1.0f) / 200.0f;
}

e2::Grunt::Grunt(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::MilitaryUnit(ctx, tile, empire)
{

	setupConfig();

}

e2::Grunt::Grunt()
	: e2::MilitaryUnit()
{
	setupConfig();
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
	E2_PROFILE_SCOPE_CTX(game());

	e2::Hex hex = e2::Hex(tileIndex);
	e2::Viewpoints2D viewpoints = game()->viewPoints();
	bool inView = viewpoints.isWithin(hex.planarCoords(), 1.0f);

	if (dying && (!m_diePose->playing() || m_diePose->time() > m_diePose->animation()->timeSeconds() - 0.05f))
	{
		game()->queueDestroyUnit(this);
		return;
	}

	if (inView && !m_proxy->enabled())
		m_proxy->enable();

	if (!inView && m_proxy->enabled())
		m_proxy->disable();


	// Only update main poses if we are in view, and if so do it fully, these are noop's if they arent playing so that is already optimized
	if (inView)
	{
		m_idlePose->updateAnimation(seconds, false);
		m_runPose->updateAnimation(seconds, false);
	}

	// Update action poses always, but only tick time if we aren't in view.
	m_firePose->updateAnimation(seconds, !inView);
	m_hitPose->updateAnimation(seconds, !inView);
	m_diePose->updateAnimation(seconds, !inView);

	// dont update base entity animation stuff unless we are in view
	if (inView)
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

void e2::CombatBoat::setupConfig()
{
	displayName = "CB-90";
	sightRange = 10;
	movePoints = 10;
	movePointsLeft = 10;

	entityType = EntityType::Unit_AssaultCraft;
	m_attackPoints = 1;
	m_attackPointsLeft = m_attackPoints;

	passableFlags = PassableFlags::WaterDeep | PassableFlags::WaterShallow;

	m_modelScale = (glm::vec3(1.0f, -1.0f, -1.0f) / 4.0f) * 2.5f;
	m_heightOffset = -0.25f;
	moveSpeed = 2.8f;
	moveType = UnitMoveType::Smooth;
}

e2::CombatBoat::CombatBoat(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::MilitaryUnit(ctx, tile, empire)
{
	setupConfig();
}

e2::CombatBoat::CombatBoat()
{
	setupConfig();
}

e2::CombatBoat::~CombatBoat()
{
	if (m_idlePose)
		e2::destroy(m_idlePose);

	if (m_drivePose)
		e2::destroy(m_drivePose);

}

void e2::CombatBoat::initialize()
{
	e2::MilitaryUnit::initialize();
	m_idlePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::CombatBoatIdle), true);
	m_drivePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::CombatBoatDrive), true);

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;
}

void e2::CombatBoat::updateAnimation(double seconds)
{
	E2_PROFILE_SCOPE_CTX(game());

	e2::Hex hex = e2::Hex(tileIndex);
	e2::Viewpoints2D viewpoints = game()->viewPoints();
	bool inView = viewpoints.isWithin(hex.planarCoords(), 1.0f);

	/*if (dying && (!m_diePose->playing() || m_diePose->time() > m_diePose->animation()->timeSeconds() - 0.05f))
	{
		game()->queueDestroyUnit(this);
		E2_END_SCOPE_CTX(game());
		return;
	}*/

	if (inView && !m_proxy->enabled())
		m_proxy->enable();

	if (!inView && m_proxy->enabled())
		m_proxy->disable();


	// Only update main poses if we are in view, and if so do it fully, these are noop's if they arent playing so that is already optimized
	if (inView)
	{
		m_idlePose->updateAnimation(seconds, false);
		m_drivePose->updateAnimation(seconds, false);
	}

	// Update action poses always, but only tick time if we aren't in view.
	//m_firePose->updateAnimation(seconds, !inView);
	//m_hitPose->updateAnimation(seconds, !inView);
	//m_diePose->updateAnimation(seconds, !inView);

	// dont update base entity animation stuff unless we are in view
	if (inView)
		e2::MilitaryUnit::updateAnimation(seconds);

}

void e2::CombatBoat::drawUI(e2::UIContext* ui)
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

void e2::CombatBoat::onBeginMove()
{
	setCurrentPose(m_drivePose, 0.2f);
}

void e2::CombatBoat::onEndMove()
{
	setCurrentPose(m_idlePose, 0.2f);
}

void e2::CombatBoat::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::MilitaryUnit::onHit(instigator, dmg);

	if (health <= 0.0f)
	{
		kill();
	}
	else
	{
		//playAction(m_hitPose, 0.2f, 0.2f);
	}
}

void e2::CombatBoat::kill()
{
	e2::MilitaryUnit::kill();

	//playAction(m_diePose, 0.2f, 0.0f);
}

void e2::CombatBoat::updateEntityAction(double seconds)
{
	/*if (m_fireBegin.durationSince().seconds() > 0.5f && !m_didFire)
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
			newLabel.offset = game()->worldToPixels(e2::Hex(m_targetUnit->tileIndex).localCoords() + glm::vec3(e2::worldUp() * 0.5));
			newLabel.velocity = { 50.0f, -200.0f };
			newLabel.text = std::format("{:.0f}", 50.0f);
			if (labelIndex >= 3)
				labelIndex = 0;
		}


		game()->endCustomEntityAction();
		m_didFire = false;
	}*/
}

void e2::CombatBoat::onEntityTargetChanged(e2::Hex const& location)
{
	m_targetUnit = game()->unitAtHex(location.offsetCoords());
}

void e2::CombatBoat::onEntityTargetClicked()
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

void e2::CombatBoat::onTurnStart()
{
	e2::MilitaryUnit::onTurnStart();
}

void e2::CombatBoat::onTurnEnd()
{
	e2::MilitaryUnit::onTurnEnd();
}

void e2::Tank::setupConfig()
{
	displayName = "Stridsvagn 122";
	sightRange = 10;
	movePoints = 10;
	movePointsLeft = 10;

	entityType = EntityType::Unit_Tank;
	m_attackPoints = 1;
	m_attackPointsLeft = m_attackPoints;

	m_modelScale = glm::vec3(1.0f, -1.0f, -1.0f) / 8.0f;
	moveSpeed = 2.8f;
	moveType = UnitMoveType::Smooth;
}

e2::Tank::Tank(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::MilitaryUnit(ctx, tile, empire)
{
	setupConfig();
}

e2::Tank::Tank()
	: e2::MilitaryUnit()
{
	setupConfig();
}

e2::Tank::~Tank()
{
	if (m_idlePose)
		e2::destroy(m_idlePose);

	if (m_drivePose)
		e2::destroy(m_drivePose);

	if (m_firePose)
		e2::destroy(m_firePose);
}

void e2::Tank::initialize()
{
	e2::MilitaryUnit::initialize();
	m_idlePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::TankIdle), true);
	m_drivePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::TankDrive), true);
	m_firePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::TankFire), false);

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;
}

void e2::Tank::updateAnimation(double seconds)
{
	E2_PROFILE_SCOPE_CTX(game());

	e2::Hex hex = e2::Hex(tileIndex);
	e2::Viewpoints2D viewpoints = game()->viewPoints();
	bool inView = viewpoints.isWithin(hex.planarCoords(), 1.0f);

	/*if (dying && (!m_diePose->playing() || m_diePose->time() > m_diePose->animation()->timeSeconds() - 0.05f))
	{
		game()->queueDestroyUnit(this);
		E2_END_SCOPE_CTX(game());
		return;
	}*/

	if (inView && !m_proxy->enabled())
		m_proxy->enable();

	if (!inView && m_proxy->enabled())
		m_proxy->disable();


	// Only update main poses if we are in view, and if so do it fully, these are noop's if they arent playing so that is already optimized
	if (inView)
	{
		m_idlePose->updateAnimation(seconds, false);
		m_drivePose->updateAnimation(seconds, false);
	}

	// Update action poses always, but only tick time if we aren't in view.
	m_firePose->updateAnimation(seconds, !inView);
	//m_hitPose->updateAnimation(seconds, !inView);
	//m_diePose->updateAnimation(seconds, !inView);

	// dont update base entity animation stuff unless we are in view
	if (inView)
		e2::MilitaryUnit::updateAnimation(seconds);


}

void e2::Tank::drawUI(e2::UIContext* ui)
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

void e2::Tank::onBeginMove()
{
	setCurrentPose(m_drivePose, 0.2f);
}

void e2::Tank::onEndMove()
{
	setCurrentPose(m_idlePose, 0.2f);
}

void e2::Tank::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::MilitaryUnit::onHit(instigator, dmg);

	if (health <= 0.0f)
	{
		kill();
	}
	else
	{
		//playAction(m_hitPose, 0.2f, 0.2f);
	}
}

void e2::Tank::kill()
{
	e2::MilitaryUnit::kill();

	//playAction(m_diePose, 0.2f, 0.0f);
}

void e2::Tank::updateEntityAction(double seconds)
{
	/*if (m_fireBegin.durationSince().seconds() > 0.5f && !m_didFire)
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
			newLabel.offset = game()->worldToPixels(e2::Hex(m_targetUnit->tileIndex).localCoords() + glm::vec3(e2::worldUp() * 0.5));
			newLabel.velocity = { 50.0f, -200.0f };
			newLabel.text = std::format("{:.0f}", 50.0f);
			if (labelIndex >= 3)
				labelIndex = 0;
		}


		game()->endCustomEntityAction();
		m_didFire = false;
	}*/
}

void e2::Tank::onEntityTargetChanged(e2::Hex const& location)
{
	m_targetUnit = game()->unitAtHex(location.offsetCoords());
}

void e2::Tank::onEntityTargetClicked()
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

void e2::Tank::onTurnStart()
{
	e2::MilitaryUnit::onTurnStart();
}

void e2::Tank::onTurnEnd()
{
	e2::MilitaryUnit::onTurnEnd();
}
