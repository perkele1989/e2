
#include "game/gameentity.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"
#include "game/wave.hpp"

#include "e2/managers/audiomanager.hpp"

#include <e2/e2.hpp>
#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

#define INVOKE_SCRIPT_HANDLE(x, ...) if(x) \
{\
try\
{\
	x(__VA_ARGS__);\
}\
catch (chaiscript::exception::eval_error& e)\
{\
	LogError("chai: evaluation failed: {}", e.pretty_print());\
}\
catch (chaiscript::exception::bad_boxed_cast& e)\
{\
	LogError("chai: casting return-type from script to native failed: {}", e.what());\
}\
catch (chaiscript::exception::arity_error& e)\
{\
	LogError("chai: arity error: {}", e.what());\
}\
catch (chaiscript::exception::guard_error& e)\
{\
	LogError("chai: guard error: {}", e.what());\
}\
catch (std::exception& e)\
{\
	LogError("{}", e.what());\
}\
}

#define INVOKE_SCRIPT_HANDLE_WITH_RETURN(t, d, x, ...) \
t returnValue = d;\
if(x) \
{\
try\
{\
	returnValue = x(__VA_ARGS__);\
}\
catch (chaiscript::exception::eval_error& e)\
{\
	LogError("chai: evaluation failed: {}", e.pretty_print());\
}\
catch (chaiscript::exception::bad_boxed_cast& e)\
{\
	LogError("chai: casting return-type from script to native failed: {}", e.what());\
}\
catch (chaiscript::exception::arity_error& e)\
{\
	LogError("chai: arity error: {}", e.what());\
}\
catch (chaiscript::exception::guard_error& e)\
{\
	LogError("chai: guard error: {}", e.what());\
}\
catch (std::exception& e)\
{\
	LogError("{}", e.what());\
}\
}\
return returnValue;

using json = nlohmann::json;


void e2::GameEntity::updateAnimation(double seconds)
{
	if (inView && !meshProxy->enabled())
		meshProxy->enable();

	if (!inView && meshProxy->enabled())
		meshProxy->disable();

	// never need to do these basic things if not in view
	if (inView)
		meshRotation = glm::slerp(meshRotation, meshTargetRotation, glm::min(1.0f, float(10.0f * seconds)));

	if (skinProxy)
	{
		if(inView)
			m_mainPose->applyBindPose();

		double blendTime = m_lastChangePose.durationSince().seconds();
		double poseBlend = 0.0;
		if (m_lerpTime >= 0.0001)
		{
			poseBlend = glm::clamp(blendTime / m_lerpTime, 0.0, 1.0);
			if (poseBlend >= 1.0)
				m_oldPose = nullptr;
		}
		else
			m_oldPose = nullptr;

		if (m_oldPose)
			m_oldPose->updateAnimation(seconds, !inView);

		if (m_currentPose)
			m_currentPose->updateAnimation(seconds * m_poseSpeed, !inView);

		if (inView)
		{
			if (m_oldPose && m_currentPose)
			{
				m_mainPose->applyBlend(m_oldPose, m_currentPose, poseBlend);
			}
			else if (m_currentPose)
			{
				m_mainPose->applyPose(m_currentPose);
			}
		}

		if (m_actionPose)
		{
			m_actionPose->updateAnimation(seconds * m_actionSpeed, !inView);
			if (m_actionPose->playing())
			{
				double actionCurrentTime = m_actionPose->time();

				if (m_currentAction)
				{
					for (e2::EntityAnimationActionTrigger& trigger : m_currentAction->triggers)
					{
						bool triggerInRange = trigger.time <= actionCurrentTime && trigger.time >= m_lastActionTime;
						if (triggerInRange && !trigger.triggered)
						{
							onActionTrigger(m_currentAction->id, trigger.id);
							trigger.triggered = true;
						}
					}
				}


				// 
				m_lastActionTime = actionCurrentTime;

				if (inView)
				{
					double actionTotalTime = m_actionPose->animation()->timeSeconds();
					double blendInCoeff = glm::smoothstep(0.0, m_actionBlendInTime, actionCurrentTime);
					double blendOutCoeff = 1.0 - glm::smoothstep(actionTotalTime - m_actionBlendOutTime, actionTotalTime, actionCurrentTime);
					double actionBlendCoeff = blendInCoeff * blendOutCoeff;


					m_mainPose->blendWith(m_actionPose, actionBlendCoeff);
				}
			}
			else
			{
				m_actionPose = nullptr;

				if (m_currentAction)
				{
					// trigger any untriggered events (may be last frame or smth)
					// and then set triggered to false for all of them
					for (e2::EntityAnimationActionTrigger& trigger : m_currentAction->triggers)
					{
						if (!trigger.triggered)
						{
							onActionTrigger(m_currentAction->id, trigger.id);
						}

						trigger.triggered = false;
					}
				}
				m_currentAction = nullptr;
			}

		}

		if (inView)
		{
			m_mainPose->updateSkin();
			skinProxy->applyPose(m_mainPose);
		}
	}

	if (meshProxy)
	{
		meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
		meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale * e2::globalMeshScale);
		meshProxy->modelMatrixDirty = true;
	}

	if (specification->scriptInterface.hasUpdateAnimation())
	{
		specification->scriptInterface.invokeUpdateAnimation(this, seconds);
	}

}



void e2::GameEntity::update(double seconds)
{
	if (specification->scriptInterface.hasUpdate())
	{
		specification->scriptInterface.invokeUpdate(this, seconds);
		return;
	}
}

void e2::GameEntity::onActionTrigger(e2::Name action, e2::Name trigger)
{
	if (specification->scriptInterface.hasOnActionTrigger())
	{
		specification->scriptInterface.invokeOnActionTrigger(this, action, trigger);
		return;
	}
}

void e2::GameEntity::updateCustomAction(double seconds)
{
	if (specification->scriptInterface.hasUpdateCustomAction())
	{
		specification->scriptInterface.invokeUpdateCustomAction(this, seconds);
		return;
	}
}

void e2::GameEntity::onWavePreparing()
{
	if (specification->scriptInterface.hasOnWavePreparing())
	{
		specification->scriptInterface.invokeOnWavePreparing(this);
		return;
	}
}

void e2::GameEntity::onWaveStart()
{
	if (specification->scriptInterface.hasOnWaveStart())
	{
		specification->scriptInterface.invokeOnWaveStart(this);
		return;
	}
}

void e2::GameEntity::onWaveUpdate(double seconds)
{
	if (specification->scriptInterface.hasOnWaveUpdate())
	{
		specification->scriptInterface.invokeOnWaveUpdate(this, seconds);
		return;
	}
}

void e2::GameEntity::onWaveEnding()
{
	if (specification->scriptInterface.hasOnWaveEnding())
	{
		specification->scriptInterface.invokeOnWaveEnding(this);
		return;
	}
}

void e2::GameEntity::onWaveEnd()
{
	if (specification->scriptInterface.hasOnWaveEnd())
	{
		specification->scriptInterface.invokeOnWaveEnd(this);
		return;
	}
}

void e2::GameEntity::onMobSpawned(e2::Mob* mob)
{
	if (specification->scriptInterface.hasOnMobSpawned())
	{
		specification->scriptInterface.invokeOnMobSpawned(this, mob);
		return;
	}
}

void e2::GameEntity::onMobDestroyed(e2::Mob* mob)
{
	if (specification->scriptInterface.hasOnMobDestroyed())
	{
		specification->scriptInterface.invokeOnMobDestroyed(this, mob);
		return;
	}
}

void e2::GameEntity::collectRevenue(ResourceTable& outRevenueTable)
{
	// if script specifies this function, run that instead and early return 
	if (specification->scriptInterface.hasCollectRevenue())
	{
		specification->scriptInterface.invokeCollectRevenue(this, outRevenueTable);
		return;
	}

	// if entity json specifies that revenue is scaled by abundance, then do so
	float multiplier = 1.0f;
	if (specification->revenueByAbundance)
	{
		multiplier = game()->hexGrid()->getExistingTileData(tileIndex)->getAbundanceAsFloat();
	}

	// apply revenue scaled by multiplier
	outRevenueTable += specification->revenue * multiplier;
}

void e2::GameEntity::collectExpenditure(ResourceTable& outExpenditureTable)
{
	if (specification->scriptInterface.hasCollectExpenditure())
	{
		return specification->scriptInterface.invokeCollectExpenditure(this, outExpenditureTable);
	}

	outExpenditureTable += specification->upkeep;
}

void e2::GameEntity::initialize()
{
	if(isLocal())
		spreadVisibility();

	if (specification->meshAsset && !meshProxy)
	{
		e2::MeshProxyConfiguration proxyConf{};
		e2::MeshLodConfiguration lod;

		lod.mesh = specification->meshAsset;
		proxyConf.lods.push(lod);

		meshProxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
		meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale * e2::globalMeshScale);

		meshProxy->modelMatrixDirty = true;
	}

	if (specification->meshAsset && specification->skeletonAsset && !skinProxy)
	{
		e2::SkinProxyConfiguration conf;
		conf.mesh = specification->meshAsset;
		conf.skeleton = specification->skeletonAsset;
		skinProxy = e2::create<e2::SkinProxy>(game()->gameSession(), conf);
		meshProxy->skinProxy = skinProxy;
		meshProxy->invalidatePipeline();

		m_mainPose = e2::create<e2::Pose>(specification->skeletonAsset);
	}

	if(specification->defaultPose.index() > 0)
		setPose(specification->defaultPose);

	movePointsLeft = specification->movePoints;
	attackPointsLeft = specification->attackPoints;
	buildPointsLeft = specification->buildPoints;
}

void e2::GameEntity::onHit(e2::GameEntity* instigator, float damage)
{
	if (specification->scriptInterface.hasOnHit())
	{
		specification->scriptInterface.invokeOnHit(this, instigator, damage);
		return;
	}

	health -= damage;
	if (health <= 0.0f)
		game()->killEntity(this);
}

void e2::GameEntity::onTargetChanged(glm::ivec2 const& location)
{
	if (specification->scriptInterface.hasOnTargetChanged())
	{
		specification->scriptInterface.invokeOnTargetChanged(this, location);
		return;
	}
}

void e2::GameEntity::onTargetClicked()
{
	if (specification->scriptInterface.hasOnTargetClicked())
	{
		specification->scriptInterface.invokeOnTargetClicked(this);
		return;
	}
}

e2::GameEntity::GameEntity(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::ivec2 const& tile, EmpireId empire)
	: e2::GameEntity()
{
	postConstruct(ctx, spec, tile, empire);
}

e2::GameEntity::GameEntity()
{

	meshTargetRotation = glm::angleAxis(0.0f, glm::vec3(e2::worldUp()));
	meshRotation = meshTargetRotation;

}

e2::GameEntity::~GameEntity()
{
	destroyProxy();

	for (e2::MeshProxy* proxy : m_meshes)
		destroyMesh(proxy);


	for (e2::EntityAnimationAction &action : m_animationActions)
		e2::destroy(action.pose);


	for (e2::EntityAnimationPose& pose: m_animationPoses)
		e2::destroy(pose.pose);

}

bool e2::GameEntity::scriptEqualityPtr(GameEntity* lhs, GameEntity* rhs)
{
	return lhs == rhs;
}

e2::GameEntity* e2::GameEntity::scriptAssignPtr(GameEntity*& lhs, GameEntity* rhs)
{
	lhs = rhs;
	return lhs;
}

void e2::GameEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::ivec2 const& tile, EmpireId empire)
{
	m_game = ctx->game();

	specification = spec;
	tileIndex = tile;
	empireId = empire;
	meshPosition = e2::Hex(tileIndex).localCoords();

	for (auto& [id, action] : specification->actions)
	{
		e2::EntityAnimationAction newAction;

		newAction.id = id;
		newAction.pose = e2::create<e2::AnimationPose>(specification->skeletonAsset, action.animationAsset, false);
		newAction.blendInTime = action.blendInTime;
		newAction.blendOutTime = action.blendOutTime;
		newAction.speed = action.speed;

		for (e2::EntityActionTriggerSpecification& trigger : action.triggers)
		{
			e2::EntityAnimationActionTrigger newTrigger;
			newTrigger.id = trigger.id;
			newTrigger.time = trigger.time;
			newTrigger.triggered = false;

			newAction.triggers.push(newTrigger);
		}
		

		m_animationActions.push(newAction);
	}

	for (auto& [id, pose] : specification->poses)
	{
		e2::EntityAnimationPose newPose;

		newPose.id = id;
		newPose.pose = e2::create<e2::AnimationPose>(specification->skeletonAsset, pose.animationAsset, true);
		newPose.blendTime = pose.blendTime;
		newPose.speed = pose.speed;

		m_animationPoses.push(newPose);
	}

	health = spec->maxHealth;

	if (specification->scriptInterface.hasCreateState())
	{
		scriptState = specification->scriptInterface.invokeCreateState(this);
	}
}

void e2::GameEntity::drawUI(e2::UIContext* ctx)
{
	// if this function was overridden via script, invoke that instead
	if (specification->scriptInterface.hasDrawUI())
	{
		specification->scriptInterface.invokeDrawUI(this, ctx); 
		return;
	}

	ctx->beginStackV("entityUiStackV");

	ctx->gameLabel(std::format("**{}**", specification->displayName), 12, e2::UITextAlign::Middle);

	ctx->endStackV();
}

void e2::GameEntity::writeForSave(e2::Buffer& toBuffer)
{
	toBuffer << meshRotation;
	toBuffer << meshTargetRotation;
	toBuffer << meshPosition;

}

void e2::GameEntity::readForSave(e2::Buffer& fromBuffer)
{

	fromBuffer >> meshRotation;
	fromBuffer >> meshTargetRotation;
	fromBuffer >> meshPosition;
}

bool e2::GameEntity::playerRelevant()
{
	if (sleeping)
		return false;

	if (specification->scriptInterface.hasPlayerRelevant())
	{
		updateGrugVariables();
		return specification->scriptInterface.invokePlayerRelevant(this);
	}


	return false;
}

bool e2::GameEntity::grugRelevant()
{
	if (specification->scriptInterface.hasGrugRelevant())
	{
		return specification->scriptInterface.invokeGrugRelevant(this);
	}


	return false;
}

bool e2::GameEntity::grugTick(double seconds)
{
	if (specification->scriptInterface.hasGrugTick())
	{
		return specification->scriptInterface.invokeGrugTick(this, seconds);
	}

	return false;
}

namespace
{
	// reused so we dont overdo allocs
	std::vector<e2::Hex> tmpHex;
	void cleanTmpHex()
	{
		if (tmpHex.capacity() < 256)
			tmpHex.reserve(256);

		tmpHex.clear();
	}

}
void e2::GameEntity::setMeshTransform(glm::vec3 const& pos, float angle)
{
	meshTargetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
	meshPosition = pos;
}

bool e2::GameEntity::isLocal()
{
	return empireId == game()->localEmpire()->id;
}

e2::UnitBuildAction e2::GameEntity::createBuildAction(e2::Name unitId)
{
	return e2::UnitBuildAction(this, e2::EntitySpecification::specificationById(unitId));
}

bool e2::GameEntity::isBuilding()
{
	return currentlyBuilding;
}

e2::Game* e2::GameEntity::game()
{
	return m_game;
}

void e2::GameEntity::updateGrugVariables()
{
	e2::PathFindingAS* newAS = e2::create<e2::PathFindingAS>(this);
	grugCanMove = newAS->grugCanMove;
	grugCanAttack = newAS->grugTarget != nullptr && newAS->grugTargetMovePoints == 0;
	e2::destroy(newAS);

}

e2::Mob* e2::GameEntity::closestMobWithinRange(float range)
{
	e2::Wave* wave = game()->wave();
	if (!wave)
		return nullptr;

	float closestDistance = std::numeric_limits<float>::max();
	e2::Mob* closestMob = nullptr;

	for (e2::Mob* mob : wave->mobs)
	{
		float currentDistance = glm::distance(mob->meshPlanarCoords(), meshPlanarCoords());
		if (!mob || currentDistance < closestDistance)
		{
			closestMob = mob;
			closestDistance = currentDistance;
		}
	}

	return closestMob;
}

glm::vec2 e2::GameEntity::meshPlanarCoords()
{
	return glm::vec2(meshPosition.x, meshPosition.z);
}


void e2::GameEntity::destroyProxy()
{
	if (skinProxy)
		e2::destroy(skinProxy);

	if (meshProxy)
		e2::destroy(meshProxy);

	meshProxy = nullptr;
	skinProxy = nullptr;
}


void e2::GameEntity::setPose2(e2::Pose* pose, double lerpTime)
{
	if (!skinProxy)
		return;

	if (lerpTime <= 0.00001)
		m_oldPose = nullptr;
	else 
		m_oldPose = m_currentPose;

	m_currentPose = pose;

	m_lerpTime = lerpTime;
	m_lastChangePose = e2::timeNow();
}

void e2::GameEntity::playAction(e2::Name actionName)
{
	e2::EntityAnimationAction* action{};

	for (e2::EntityAnimationAction& a : m_animationActions)
	{
		if (a.id == actionName)
		{
			action = &a;
			break;
		}
	}

	if (!action)
		return;

	m_currentAction = action;

	playAction2(action->pose, action->blendInTime, action->blendOutTime, action->speed);
}

bool e2::GameEntity::isActionPlaying(e2::Name actionName)
{
	e2::EntityAnimationAction* action{};

	for (e2::EntityAnimationAction& a : m_animationActions)
	{
		if (a.id == actionName)
		{
			action = &a;
			break;
		}
	}

	if (!action)
		return false;

	return m_actionPose && (m_actionPose == action->pose) && m_actionPose->playing();
}

bool e2::GameEntity::isAnyActionPlaying()
{
	return m_actionPose && m_actionPose->playing();
}

void e2::GameEntity::stopAction()
{
	if (m_actionPose)
		m_actionPose->stop();
}

void e2::GameEntity::playAction2(e2::AnimationPose* anim, double blendIn /*= 0.2f*/, double blendOut /*= 0.2f*/, double speed)
{
	if (!skinProxy)
		return;

	m_actionPose = anim;
	m_actionBlendInTime = blendIn;
	m_actionBlendOutTime = blendOut;
	m_actionSpeed = speed;

	m_actionPose->play(false);
}



void e2::GameEntity::playSound(e2::Name assetName, float volume,float spatiality)
{
	auto finder = specification->assets.find(assetName);
	if (finder == specification->assets.end())
		return;

	
	m_game->audioManager()->playOneShot(finder->second.asset.cast<e2::Sound>(), volume, spatiality, meshPosition);
}

e2::MeshProxy* e2::GameEntity::createMesh(e2::Name assetName)
{
	auto finder = specification->assets.find(assetName);
	if (finder == specification->assets.end())
	{
		LogError("no such asset name in entity specification: {}", assetName);
		return nullptr;
	}

	e2::AssetPtr asset = finder->second.asset;
	if (!asset)
	{
		LogError("asset was registered in entity specification but was null: {}", assetName);
		return nullptr;
	}

	MeshLodConfiguration lod;
	lod.mesh = asset.cast<e2::Mesh>();

	if (!lod.mesh)
	{
		LogError("asset found but of wrong type: {} (needs a e2::Mesh but was actually type {})", assetName, asset->type()->fqn);
		return nullptr;
	}

	e2::MeshProxyConfiguration conf;
	conf.lods.push(lod);

	e2::MeshProxy* newProxy = e2::create<e2::MeshProxy>(game()->gameSession(), conf);
	newProxy->modelMatrix = glm::translate(glm::identity<glm::mat4>(), meshPosition);
	newProxy->modelMatrixDirty = true;
	m_meshes.push(newProxy);

	return newProxy;
}

void e2::GameEntity::destroyMesh(e2::MeshProxy* mesh)
{
	if (!mesh)
		return;

	m_meshes.removeFirstByValueFast(mesh);
	e2::destroy(mesh);
}

void e2::GameEntity::setPose(e2::Name poseName)
{
	e2::EntityAnimationPose* pose{};

	for (e2::EntityAnimationPose& p : m_animationPoses)
	{
		if (p.id == poseName)
		{
			pose = &p;
			break;
		}
	}

	if (!pose)
		return;

	setPose2(pose->pose, pose->blendTime);
}

void e2::GameEntity::spreadVisibility()
{
	::cleanTmpHex();
	
	e2::Hex thisHex(tileIndex);
	e2::Hex::circle(thisHex, specification->sightRange, ::tmpHex);

	for (e2::Hex h : ::tmpHex)
		hexGrid()->flagVisible(h.offsetCoords(), e2::Hex::distance(h, thisHex) == specification->sightRange);
}

void e2::GameEntity::rollbackVisibility()
{
	::cleanTmpHex();

	e2::Hex thisHex(tileIndex);
	e2::Hex::circle(thisHex, specification->sightRange - 1, ::tmpHex);

	for (e2::Hex h : ::tmpHex)
		hexGrid()->unflagVisible(h.offsetCoords());
}

void e2::GameEntity::onTurnEnd()
{
	if (specification->scriptInterface.hasOnTurnEnd())
	{
		specification->scriptInterface.invokeOnTurnEnd(this);
	}
}

void e2::GameEntity::onTurnStart()
{
	movePointsLeft = specification->movePoints;
	attackPointsLeft = specification->attackPoints;
	buildPointsLeft = specification->buildPoints;

	if (currentlyBuilding)
	{
		e2::UnitBuildResult result = currentlyBuilding->tick();
		buildMessage = result.buildMessage;
		if (result.didSpawn)
			currentlyBuilding = nullptr;
	}
	else
	{
		if(buildMessage.size() > 0)
			buildMessage = "";
	}

	if (specification->scriptInterface.hasOnTurnStart())
	{
		specification->scriptInterface.invokeOnTurnStart(this);
	}
}


void e2::GameEntity::onBeginMove()
{
	if (specification->scriptInterface.hasOnBeginMove())
	{
		specification->scriptInterface.invokeOnBeginMove(this);
		return;
	}

	setPose("move");
}

void e2::GameEntity::onEndMove()
{
	if (specification->scriptInterface.hasOnEndMove())
	{
		specification->scriptInterface.invokeOnEndMove(this);
		return;
	}

	setPose("idle");
}

void e2::GameEntity::onSpawned()
{
	if (specification->scriptInterface.hasOnSpawned())
		specification->scriptInterface.invokeOnSpawned(this);
}

void e2::GameEntity::turnTowards(glm::ivec2 const& hex)
{	
	float angle = e2::radiansBetween(e2::Hex(hex).localCoords(), e2::Hex(tileIndex).localCoords());
	meshTargetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
}

void e2::GameEntity::turnTowardsPlanar(glm::vec3 const& planar)
{
	float angle = e2::radiansBetween(planar, meshPosition);
	meshTargetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
}

bool e2::GameEntity::canBuild(e2::UnitBuildAction& action)
{
	if (currentlyBuilding)
		return false;

	e2::GameEmpire* empire = game()->empireById(empireId);

	e2::ResourceTable const& funds = empire->resources.funds;
	e2::ResourceTable const& cost = action.specification->cost;
	bool affordCost = funds.gold >= cost.gold
		&& funds.wood >= cost.wood
		&& funds.iron >= cost.iron
		&& funds.steel >= cost.steel;

	e2::ResourceTable const& profits = empire->resources.profits;
	e2::ResourceTable const& upkeep = action.specification->upkeep;
	bool affordUpkeep = profits.gold >= upkeep.gold
		&& profits.wood >= upkeep.wood
		&& profits.iron >= upkeep.iron
		&& profits.steel >= upkeep.steel;

	return affordCost && affordUpkeep;
}



void e2::GameEntity::build(e2::UnitBuildAction& action)
{
	if (canBuild(action))
	{
		currentlyBuilding = &action;
		buildMessage = std::format("Building {}, {} turns left.", currentlyBuilding->specification->displayName, currentlyBuilding->buildTurnsLeft);
	}
}

void e2::GameEntity::cancelBuild()
{
	currentlyBuilding = nullptr;
	buildMessage = "";
}

e2::UnitBuildAction::UnitBuildAction(e2::GameEntity* inOwner, e2::EntitySpecification const* spec)
	: owner(inOwner)
	, specification(spec)
{
	buildTurnsLeft = spec->buildTurns;
}

e2::UnitBuildResult e2::UnitBuildAction::tick()
{
	if (!owner || !specification)
		return {};

	e2::UnitBuildResult result;
	e2::Game* game = owner->game();
	// subtract turn
	if (buildTurnsLeft > 0)
		buildTurnsLeft--;

	// if we are done
	if (buildTurnsLeft <= 0)
	{

		bool hasSlot = false;
		glm::ivec2 slotToUse = owner->tileIndex;
		bool mainSlotFree = game->entityAtHex(e2::EntityLayerIndex::Unit, slotToUse) == nullptr;
		if (mainSlotFree)
			hasSlot = true;
		else
		{
			for (e2::Hex& n : e2::Hex(owner->tileIndex).neighbours())
			{
				slotToUse = n.offsetCoords();
				bool extraSlotFree = game->entityAtHex(e2::EntityLayerIndex::Unit, slotToUse) == nullptr && game->hexGrid()->getCalculatedTileData(slotToUse).isPassable(specification->passableFlags);

				if (extraSlotFree)
				{
					hasSlot = true;
					break;
				}
			}
		}

		if (!hasSlot)
		{
			result.buildMessage = "Units blocking completed build action, can't spawn this turn.";
		}
		else
		{
			buildTurnsLeft = specification->buildTurns;

			totalBuilt++;
			result.didSpawn = true;
			result.spawnLocation = slotToUse;

			game->spawnEntity(specification->id, result.spawnLocation, owner->empireId);

			result.buildMessage = std::format("Just finished building {}.", specification->displayName);
			result.didSpawn = true;
		}
	}
	else
	{
		result.buildMessage = std::format("Building {}, {} turns left.", specification->displayName, buildTurnsLeft);
	}

	return result;
}

namespace
{
	// these are loaded once on startup, then never touched again, so we just give pointers to elements
	std::vector<e2::EntitySpecification> specifications;
	std::map<e2::Name, size_t> specificationIndex;


}

void e2::EntitySpecification::initializeSpecifications(e2::GameContext* ctx)
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("data/entities/"))
	{
		if (!entry.is_regular_file())
			continue;
		if (entry.path().extension() != ".json")
			continue;

		std::ifstream unitFile(entry.path());
		json data;
		try
		{
			data = json::parse(unitFile);
		}
		catch (json::parse_error& e)
		{
			LogError("failed to load specifications, json parse error: {}", e.what());
			continue;
		}

		for (json& entity : data.at("entities"))
		{
			EntitySpecification newSpec;

			if (!entity.contains("id"))
			{
				LogError("invalid entity specification: id required. skipping..");
				continue;
			}
			e2::Name id = entity.at("id").template get<std::string>();
			newSpec.id = id;

			if (!entity.contains("typeName"))
			{
				LogError("invalid entity specification: typeName required. skipping..");
				continue;
			}
			e2::Name typeName = entity.at("typeName").template get<std::string>();

			newSpec.type = e2::Type::fromName(typeName);
			if (!newSpec.type || !newSpec.type->inheritsOrIs("e2::GameEntity"))
			{
				LogError("invalid entity specification: typeName doesn't represent a proper type..");
				continue;
			}

			if (!entity.contains("displayName"))
			{
				LogError("invalid entity specification: displayName required. skipping..");
				continue;
			}

			if (!entity.contains("mesh"))
			{
				LogError("invalid entity specification: mesh required. skipping..");
				continue;
			}

			if (entity.contains("badgeId"))
			{
				newSpec.badgeId = entity.at("badgeId").template get<std::string>();
			}
			else
			{
				newSpec.badgeId = newSpec.id;
			}

			newSpec.displayName = entity.at("displayName").template get<std::string>();

			if (entity.contains("waveRelevant"))
			{
				newSpec.waveRelevant = entity.at("waveRelevant").template get<bool>();
			}

			if (entity.contains("passableFlags"))
			{
				for (json flag : entity.at("passableFlags"))
				{
					std::string flagStr = e2::toLower(flag.template get<std::string>());
					if (flagStr == "none")
						newSpec.passableFlags = e2::PassableFlags::None;
					else if (flagStr == "land")
						newSpec.passableFlags |= e2::PassableFlags::Land;
					else if (flagStr == "watershallow")
						newSpec.passableFlags |= e2::PassableFlags::WaterShallow;
					else if (flagStr == "waterdeep")
						newSpec.passableFlags |= e2::PassableFlags::WaterDeep;
					else if (flagStr == "water")
						newSpec.passableFlags |= e2::PassableFlags::WaterDeep | e2::PassableFlags::WaterShallow;
					else if (flagStr == "air")
						newSpec.passableFlags |= e2::PassableFlags::Air;
				}
			}


			if (entity.contains("moveType"))
			{
				std::string moveTypeStr = e2::toLower(entity.at("moveType").template get<std::string>());
				if (moveTypeStr == "static")
					newSpec.moveType = EntityMoveType::Static;
				else if (moveTypeStr == "linear")
					newSpec.moveType = EntityMoveType::Linear;
				else if (moveTypeStr == "smooth")
					newSpec.moveType = EntityMoveType::Smooth;
			}

			if (entity.contains("layerIndex"))
			{
				std::string layerIndexStr = e2::toLower(entity.at("layerIndex").template get<std::string>());
				if (layerIndexStr == "unit")
					newSpec.layerIndex = EntityLayerIndex::Unit;
				else if (layerIndexStr == "air")
					newSpec.layerIndex = EntityLayerIndex::Air;
				else if (layerIndexStr == "structure")
					newSpec.layerIndex = EntityLayerIndex::Structure;
			}



			if (entity.contains("maxHealth"))
				newSpec.maxHealth = entity.at("maxHealth").template get<float>();

			if (entity.contains("moveSpeed"))
				newSpec.moveSpeed = entity.at("moveSpeed").template get<float>();

			if (entity.contains("movePoints"))
				newSpec.movePoints = entity.at("movePoints").template get<int32_t>();

			if (entity.contains("buildPoints"))
				newSpec.buildPoints = entity.at("buildPoints").template get<int32_t>();

			if (entity.contains("attackPoints"))
				newSpec.attackPoints = entity.at("attackPoints").template get<int32_t>();


			if (entity.contains("showMovePoints"))
				newSpec.showMovePoints = entity.at("showMovePoints").template get<bool>();

			if (entity.contains("showBuildPoints"))
				newSpec.showBuildPoints = entity.at("showBuildPoints").template get<bool>();

			if (entity.contains("showAttackPoints"))
				newSpec.showAttackPoints = entity.at("showAttackPoints").template get<bool>();


			if (entity.contains("attackStrength"))
				newSpec.attackStrength = entity.at("attackStrength").template get<float>();

			if (entity.contains("defensiveModifier"))
				newSpec.defensiveModifier = entity.at("defensiveModifier").template get<float>();

			if (entity.contains("retaliatoryModifier"))
				newSpec.retaliatoryModifier = entity.at("retaliatoryModifier").template get<float>();

			if (entity.contains("sightRange"))
				newSpec.sightRange = entity.at("sightRange").template get<int32_t>();

			if (entity.contains("attackRange"))
				newSpec.attackRange = entity.at("attackRange").template get<int32_t>();

			if (entity.contains("meshScale"))
			{
				json& meshScale = entity.at("meshScale");
				newSpec.meshScale = glm::vec3(meshScale[0].template get<float>(), meshScale[1].template get<float>(), meshScale[2].template get<float>());
			}

			if (entity.contains("meshHeightOffset"))
				newSpec.meshHeightOffset = entity.at("meshHeightOffset").template get<float>();

			if (entity.contains("buildTurns"))
				newSpec.buildTurns = entity.at("buildTurns").template get<int32_t>();

			if (entity.contains("upkeep"))
			{
				json& upkeep = entity.at("upkeep");
				if (upkeep.contains("gold"))
					newSpec.upkeep.gold = upkeep.at("gold").template get<float>();
				if (upkeep.contains("wood"))
					newSpec.upkeep.wood = upkeep.at("wood").template get<float>();
				if (upkeep.contains("iron"))
					newSpec.upkeep.iron = upkeep.at("iron").template get<float>();
				if (upkeep.contains("steel"))
					newSpec.upkeep.steel = upkeep.at("steel").template get<float>();
			}

			if (entity.contains("cost"))
			{
				json& cost = entity.at("cost");

				if (cost.contains("gold"))
					newSpec.cost.gold = cost.at("gold").template get<float>();
				if (cost.contains("wood"))
					newSpec.cost.wood = cost.at("wood").template get<float>();
				if (cost.contains("iron"))
					newSpec.cost.iron = cost.at("iron").template get<float>();
				if (cost.contains("steel"))
					newSpec.cost.steel = cost.at("steel").template get<float>();
			}

			if (entity.contains("revenue"))
			{
				json& revenue = entity.at("revenue");
				if (revenue.contains("gold"))
					newSpec.revenue.gold = revenue.at("gold").template get<float>();
				if (revenue.contains("wood"))
					newSpec.revenue.wood = revenue.at("wood").template get<float>();
				if (revenue.contains("iron"))
					newSpec.revenue.iron = revenue.at("iron").template get<float>();
				if (revenue.contains("steel"))
					newSpec.revenue.steel = revenue.at("steel").template get<float>();
			}


			newSpec.meshAssetPath = entity.at("mesh").template get<std::string>();
			if (entity.contains("skeleton"))
				newSpec.skeletonAssetPath = entity.at("skeleton").template get<std::string>();

			if (entity.contains("defaultPose"))
			{
				newSpec.defaultPose = entity.at("defaultPose").template get<std::string>();
			}

			if (entity.contains("poses"))
			{
				json& poses = entity.at("poses");
				for (json& pose : poses)
				{
					if (!pose.contains("blendTime") || !pose.contains("asset") || !pose.contains("name"))
					{
						LogError("invalid pose; blendTime, asset and name required.. ");
						continue;
					}

					e2::EntityPoseSpecification newPose;
					newPose.blendTime = pose.at("blendTime").template get<float>();
					newPose.animationAssetPath = pose.at("asset").template get<std::string>();

					std::string poseName = pose.at("name").template get<std::string>();
					newSpec.poses[poseName] = newPose;

					if (pose.contains("speed"))
						newPose.speed = pose.at("speed").template get<float>();
				}
			}

			if (entity.contains("actions"))
			{
				json& actions = entity.at("actions");
				for (json& action : actions)
				{
					if (!action.contains("blendInTime") || !action.contains("blendOutTime") || !action.contains("asset") || !action.contains("name"))
					{
						LogError("invalid action; blendInTime, blendOutTime, asset and name required.. ");
						continue;
					}

					e2::EntityActionSpecification newAction;
					newAction.blendInTime = action.at("blendInTime").template get<float>();
					newAction.blendOutTime = action.at("blendOutTime").template get<float>();

					if(action.contains("speed"))
						newAction.speed = action.at("speed").template get<float>();

					newAction.animationAssetPath = action.at("asset").template get<std::string>();

					std::string actionName = action.at("name").template get<std::string>();

					if (action.contains("triggers"))
					{
						for (json& trigger : action.at("triggers"))
						{
							if (!trigger.contains("id") || !trigger.contains("time"))
							{
								LogError("invalid action trigger: id, time required.. ");
								continue;
							}

							EntityActionTriggerSpecification newTrigger;
							newTrigger.id = trigger.at("id").template get<std::string>();
							newTrigger.time = trigger.at("time").template get<double>();

							newAction.triggers.push(newTrigger);
						}
					}

					newSpec.actions[actionName] = newAction;
				}
			}

			if (entity.contains("script"))
			{
				chaiscript::ChaiScript* script = ctx->game()->scriptEngine();
				try
				{
					newSpec.scriptInterface = script->eval_file<EntityScriptInterface>(entity.at("script").template get<std::string>());

				}
				catch (chaiscript::exception::eval_error& e)
				{
					LogError("chai: evaluation failed: {}", e.pretty_print());
				}
				catch (chaiscript::exception::bad_boxed_cast& e)
				{
					LogError("chai: casting return-type from script to native failed: {}", e.what());
				}
				catch (std::exception& e)
				{
					LogError("{}", e.what());
				}
			}
			if (entity.contains("extraAssets"))
			{
				json& assets = entity.at("extraAssets");
				for (json& asset : assets)
				{
					if (!asset.contains("name") || !asset.contains("path"))
					{
						LogError("extra assets require both a name and a path");
						continue;
					}

					std::string assetName = asset.at("name").template get<std::string>();
					std::string assetPath = asset.at("path").template get<std::string>();
					newSpec.assets[assetName] = {assetName, assetPath, nullptr};
				}
			}

			::specifications.push_back(newSpec);
			::specificationIndex[newSpec.id] = ::specifications.size() - 1;
		}

	}

}

void e2::EntitySpecification::finalizeSpecifications(e2::Context* ctx)
{
	e2::AssetManager* am = ctx->assetManager();
	for (e2::EntitySpecification& spec : ::specifications)
	{
		spec.meshAsset = am->get(spec.meshAssetPath).cast<e2::Mesh>();

		if (spec.skeletonAssetPath.size() > 0)
			spec.skeletonAsset = am->get(spec.skeletonAssetPath).cast<e2::Skeleton>();

		for (auto& [name, pose] : spec.poses)
		{
			pose.animationAsset = am->get(pose.animationAssetPath).cast<e2::Animation>();
		}

		for (auto& [name, action] : spec.actions)
		{
			action.animationAsset = am->get(action.animationAssetPath).cast<e2::Animation>();
		}

		for (auto& [name, asset] : spec.assets)
		{
			asset.asset = am->get(asset.path);
		}
	}
}

void e2::EntitySpecification::destroySpecifications()
{
	specifications.clear();
	specificationIndex.clear();
}

e2::EntitySpecification* e2::EntitySpecification::specificationById(e2::Name id)
{
	auto finder = ::specificationIndex.find(id);
	if (finder == ::specificationIndex.end())
	{
		return nullptr;
	}

	return specification(finder->second);
}

e2::EntitySpecification* e2::EntitySpecification::specification(size_t index)
{
	return &::specifications[index];
}

size_t e2::EntitySpecification::specificationCount()
{
	return ::specifications.size();
}

chaiscript::Boxed_Value e2::EntityScriptInterface::invokeCreateState(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(chaiscript::Boxed_Value, {}, createState, entity);
}

void e2::EntityScriptInterface::invokeDrawUI(e2::GameEntity* entity, e2::UIContext* ui)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(drawUI, entity, ui);
}

void e2::EntityScriptInterface::invokeUpdate(e2::GameEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(update, entity, seconds);
}

void e2::EntityScriptInterface::invokeUpdateAnimation(e2::GameEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(updateAnimation, entity, seconds);
}

bool e2::EntityScriptInterface::invokePlayerRelevant(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, playerRelevant, entity);
}

bool e2::EntityScriptInterface::invokeGrugRelevant(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, grugRelevant, entity);
}

bool e2::EntityScriptInterface::invokeGrugTick(e2::GameEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, grugTick, entity, seconds);
}

void e2::EntityScriptInterface::invokeCollectRevenue(e2::GameEntity* entity, e2::ResourceTable &outTable)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(collectRevenue, entity, outTable);
}

void e2::EntityScriptInterface::invokeCollectExpenditure(e2::GameEntity* entity, e2::ResourceTable& outTable)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(collectExpenditure, entity, outTable);
}

void e2::EntityScriptInterface::invokeOnHit(e2::GameEntity* entity, e2::GameEntity* instigator, float dmg)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onHit, entity, instigator, dmg);
}

void e2::EntityScriptInterface::invokeOnTargetChanged(e2::GameEntity* entity, glm::ivec2 const& hex)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTargetChanged, entity, hex);
}

void e2::EntityScriptInterface::invokeOnTargetClicked(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTargetClicked, entity);
}

void e2::EntityScriptInterface::invokeUpdateCustomAction(e2::GameEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(updateCustomAction, entity, seconds);
}

void e2::EntityScriptInterface::invokeOnActionTrigger(e2::GameEntity* entity, e2::Name action, e2::Name trigger)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onActionTrigger, entity, action.string(), trigger.string());
}

void e2::EntityScriptInterface::invokeOnTurnStart(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTurnStart, entity);
}

void e2::EntityScriptInterface::invokeOnTurnEnd(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTurnEnd, entity);
}

void e2::EntityScriptInterface::invokeOnBeginMove(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onBeginMove, entity);
}

void e2::EntityScriptInterface::invokeOnEndMove(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onEndMove, entity);
}

void e2::EntityScriptInterface::invokeOnWaveUpdate(e2::GameEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveUpdate, entity, seconds);
}

void e2::EntityScriptInterface::invokeOnWavePreparing(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWavePreparing, entity);
}

void e2::EntityScriptInterface::invokeOnWaveStart(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveStart, entity);
}

void e2::EntityScriptInterface::invokeOnWaveEnding(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveEnding, entity);
}

void e2::EntityScriptInterface::invokeOnWaveEnd(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveEnd, entity);
}

void e2::EntityScriptInterface::invokeOnSpawned(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onSpawned, entity);
}

void e2::EntityScriptInterface::invokeOnMobSpawned(e2::GameEntity* entity, e2::Mob* mob)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onMobSpawned, entity, mob);
}

void e2::EntityScriptInterface::invokeOnMobDestroyed(e2::GameEntity* entity, e2::Mob* mob)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onMobSpawned, entity, mob);
}

void e2::EntityScriptInterface::invokeOnSelected(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onSelected, entity);
}

void e2::EntityScriptInterface::invokeOnDeselected(e2::GameEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onDeselected, entity);
}

void e2::EntityScriptInterface::setCreateState(scriptFunc_createState func)
{
	createState = func;
}

void e2::EntityScriptInterface::setDrawUI(scriptFunc_drawUI func)
{
	drawUI = func;
}

void e2::EntityScriptInterface::setUpdate(scriptFunc_update func)
{
	update = func;
}

void e2::EntityScriptInterface::setUpdateAnimation(scriptFunc_updateAnimation func)
{
	updateAnimation = func;
}

void e2::EntityScriptInterface::setPlayerRelevant(scriptFunc_playerRelevant func)
{
	playerRelevant = func;
}

void e2::EntityScriptInterface::setGrugRelevant(scriptFunc_grugRelevant func)
{
	grugRelevant = func;
}

void e2::EntityScriptInterface::setGrugTick(scriptFunc_grugTick func)
{
	grugTick = func;
}

void e2::EntityScriptInterface::setCollectRevenue(scriptFunc_fiscal func)
{
	collectRevenue = func;
}

void e2::EntityScriptInterface::setCollectExpenditure(scriptFunc_fiscal func)
{
	collectExpenditure = func;
}

void e2::EntityScriptInterface::setOnHit(scriptFunc_onHit func)
{
	onHit = func;
}

void e2::EntityScriptInterface::setOnTargetChanged(scriptFunc_onTargetChanged func)
{
	onTargetChanged = func;
}

void e2::EntityScriptInterface::setOnTargetClicked(scriptFunc_onTargetClicked func)
{
	onTargetClicked = func;
}

void e2::EntityScriptInterface::setUpdateCustomAction(scriptFunc_updateCustomAction func)
{
	updateCustomAction = func;
}

void e2::EntityScriptInterface::setOnActionTrigger(scriptFunc_onActionTrigger func)
{
	onActionTrigger = func;
}

void e2::EntityScriptInterface::setOnTurnStart(scriptFunc_onTurnStart func)
{
	onTurnStart = func;
}

void e2::EntityScriptInterface::setOnTurnEnd(scriptFunc_onTurnEnd func)
{
	onTurnEnd = func;
}

void e2::EntityScriptInterface::setOnBeginMove(scriptFunc_onBeginMove func)
{
	onBeginMove = func;
}

void e2::EntityScriptInterface::setOnEndMove(scriptFunc_onEndMove func)
{
	onEndMove = func;
}

void e2::EntityScriptInterface::setOnWaveUpdate(scriptFunc_onWaveUpdate func)
{
	onWaveUpdate = func;
}

void e2::EntityScriptInterface::setOnSpawned(scriptFunc_onSpawned func)
{
	onSpawned = func;
}

void e2::EntityScriptInterface::setOnWavePreparing(scriptFunc_onWavePreparing func)
{
	onWavePreparing = func;
}

void e2::EntityScriptInterface::setOnWaveStart(scriptFunc_onWaveStart func)
{
	onWaveStart = func;
}

void e2::EntityScriptInterface::setOnWaveEnding(scriptFunc_onWaveEnding func)
{
	onWaveEnding = func;
}

void e2::EntityScriptInterface::setOnWaveEnd(scriptFunc_onWaveEnd func)
{
	onWaveEnd = func;
}

void e2::EntityScriptInterface::setOnMobSpawned(scriptFunc_onMobSpawned func)
{
	onMobSpawned = func;
}

void e2::EntityScriptInterface::setOnMobDestroyed(scriptFunc_onMobDestroyed func)
{
	onMobDestroyed = func;
}

void e2::EntityScriptInterface::setOnSelected(scriptFunc_onSelected func)
{
	onSelected = func;
}

void e2::EntityScriptInterface::setOnDeselected(scriptFunc_onDeselected func)
{
	onDeselected = func;
}

bool e2::EntityScriptInterface::hasCreateState()
{
	return createState != nullptr;
}

bool e2::EntityScriptInterface::hasUpdate()
{
	return update != nullptr;
}

bool e2::EntityScriptInterface::hasDrawUI()
{
	return drawUI != nullptr;
}

bool e2::EntityScriptInterface::hasUpdateAnimation()
{
	return updateAnimation != nullptr;
}

bool e2::EntityScriptInterface::hasPlayerRelevant()
{
	return playerRelevant != nullptr;
}


bool e2::EntityScriptInterface::hasGrugRelevant()
{
	return grugRelevant != nullptr;
}

bool e2::EntityScriptInterface::hasGrugTick()
{
	return grugTick != nullptr;
}

bool e2::EntityScriptInterface::hasCollectRevenue()
{
	return collectRevenue != nullptr;
}

bool e2::EntityScriptInterface::hasCollectExpenditure()
{
	return collectExpenditure != nullptr;
}

bool e2::EntityScriptInterface::hasOnHit()
{
	return onHit != nullptr;
}

bool e2::EntityScriptInterface::hasOnTargetChanged()
{
	return onTargetChanged != nullptr;
}

bool e2::EntityScriptInterface::hasOnTargetClicked()
{
	return onTargetClicked != nullptr;
}

bool e2::EntityScriptInterface::hasUpdateCustomAction()
{
	return updateCustomAction != nullptr;
}

bool e2::EntityScriptInterface::hasOnActionTrigger()
{
	return onActionTrigger != nullptr;
}

bool e2::EntityScriptInterface::hasOnTurnStart()
{
	return onTurnStart != nullptr;
}

bool e2::EntityScriptInterface::hasOnTurnEnd()
{
	return onTurnEnd != nullptr;
}

bool e2::EntityScriptInterface::hasOnBeginMove()
{
	return onBeginMove != nullptr;
}

bool e2::EntityScriptInterface::hasOnEndMove()
{
	return onEndMove != nullptr;
}


bool e2::EntityScriptInterface::hasOnWaveUpdate()
{
	return onWaveUpdate != nullptr;
}

bool e2::EntityScriptInterface::hasOnWavePreparing()
{
	return onWavePreparing != nullptr;
}

bool e2::EntityScriptInterface::hasOnWaveStart()
{
	return onWaveStart != nullptr;
}

bool e2::EntityScriptInterface::hasOnWaveEnding()
{
	return onWaveEnding != nullptr;
}

bool e2::EntityScriptInterface::hasOnWaveEnd()
{
	return onWaveEnd != nullptr;
}

bool e2::EntityScriptInterface::hasOnSpawned()
{
	return onSpawned != nullptr;
}

bool e2::EntityScriptInterface::hasOnMobSpawned()
{
	return onMobSpawned != nullptr;
}

bool e2::EntityScriptInterface::hasOnMobDestroyed()
{
	return onMobDestroyed != nullptr;
}

bool e2::EntityScriptInterface::hasOnSelected()
{
	return onSelected != nullptr;
}

bool e2::EntityScriptInterface::hasOnDeselected()
{
	return onDeselected != nullptr;
}
