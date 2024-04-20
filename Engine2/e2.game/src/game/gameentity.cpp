
#include "game/gameentity.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;


void e2::GameEntity::updateAnimation(double seconds)
{
	if (dead)
	{
		game()->queueDestroyEntity(this);
		return;
	}

	if (inView && !meshProxy->enabled())
		meshProxy->enable();

	if (!inView && meshProxy->enabled())
		meshProxy->disable();

	// never need to do these basic things if not in view
	if (inView)
	{
		meshRotation = glm::slerp(meshRotation, meshTargetRotation, glm::min(1.0f, float(10.0f * seconds)));

		if (skinProxy)
		{
			m_mainPose->applyBindPose();

			double poseBlend = glm::clamp(m_lastChangePose.durationSince().seconds() / m_lerpTime, 0.0, 1.0);
			if (poseBlend >= 1.0)
				m_oldPose = nullptr;

			if (m_oldPose)
				m_oldPose->updateAnimation(seconds, !inView);

			if (m_currentPose)
				m_currentPose->updateAnimation(seconds, !inView);

			if (m_oldPose && m_currentPose)
			{
				m_mainPose->applyBlend(m_oldPose, m_currentPose, poseBlend);
			}
			else if (m_currentPose)
			{
				m_mainPose->applyPose(m_currentPose);
			}

			if (m_actionPose && m_actionPose->playing())
			{
				m_actionPose->updateAnimation(seconds, !inView);

				double actionCurrentTime = m_actionPose->time();
				double actionTotalTime = m_actionPose->animation()->timeSeconds();
				double blendInCoeff = glm::smoothstep(0.0, m_actionBlendInTime, actionCurrentTime);
				double blendOutCoeff = 1.0 - glm::smoothstep(actionTotalTime - m_actionBlendOutTime, actionTotalTime, actionCurrentTime);
				double actionBlendCoeff = blendInCoeff * blendOutCoeff;
				m_mainPose->blendWith(m_actionPose, actionBlendCoeff);
			}

			m_mainPose->updateSkin();

			skinProxy->applyPose(m_mainPose);
		}

		if (meshProxy)
		{
			meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
			meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale);
			meshProxy->modelMatrixDirty = true;
		}
	}

	if (specification->scriptInterface.updateAnimation)
	{
		specification->scriptInterface.updateAnimation(this, seconds);
	}

}



void e2::GameEntity::updateCustomAction(double seconds)
{
	if (specification->scriptInterface.updateCustomAction)
	{
		specification->scriptInterface.updateCustomAction(this, seconds);
		return;
	}
}

void e2::GameEntity::collectRevenue(ResourceTable& outRevenueTable)
{
	if (specification->scriptInterface.collectRevenue)
	{
		return specification->scriptInterface.collectRevenue(this, outRevenueTable);
	}
}

void e2::GameEntity::collectExpenditure(ResourceTable& outExpenditureTable)
{
	if (specification->scriptInterface.collectExpenditure)
	{
		return specification->scriptInterface.collectExpenditure(this, outExpenditureTable);
	}
}

void e2::GameEntity::initialize()
{
	if(isLocal())
		spreadVisibility();

	if (specification->meshAsset && !meshProxy)
	{
		e2::MeshProxyConfiguration proxyConf{};
		proxyConf.mesh = specification->meshAsset;

		meshProxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
		meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale);

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

	setPose("idle");
	movePointsLeft = specification->movePoints;
	attackPointsLeft = specification->attackPoints;
	buildPointsLeft = specification->buildPoints;
}

void e2::GameEntity::onHit(e2::GameEntity* instigator, float damage)
{
	if (specification->scriptInterface.onHit)
	{
		specification->scriptInterface.onHit(this, instigator, damage);
		return;
	}

	health -= damage;
	if (health <= 0.0f)
		game()->queueDestroyEntity(this);
}

void e2::GameEntity::onTargetChanged(e2::Hex const& location)
{
	if (specification->scriptInterface.onTargetChanged)
	{
		specification->scriptInterface.onTargetChanged(this, location);
		return;
	}
}

void e2::GameEntity::onTargetClicked()
{
	if (specification->scriptInterface.onTargetClicked)
	{
		specification->scriptInterface.onTargetClicked(this);
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

	for (e2::EntityAnimationAction &action : m_animationActions)
		e2::destroy(action.pose);


	for (e2::EntityAnimationPose& pose: m_animationPoses)
		e2::destroy(pose.pose);

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

		m_animationActions.push(newAction);
	}

	for (auto& [id, pose] : specification->poses)
	{
		e2::EntityAnimationPose newPose;

		newPose.id = id;
		newPose.pose = e2::create<e2::AnimationPose>(specification->skeletonAsset, pose.animationAsset, true);
		newPose.blendTime = pose.blendTime;

		m_animationPoses.push(newPose);
	}
}

void e2::GameEntity::drawUI(e2::UIContext* ctx)
{
	// if this function was overridden via script, invoke that instead
	if (specification->scriptInterface.drawUI)
	{
		specification->scriptInterface.drawUI(this, ctx);
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

bool e2::GameEntity::grugRelevant()
{
	if (specification->scriptInterface.grugRelevant)
	{
		return specification->scriptInterface.grugRelevant(this);
	}


	return false;
}

bool e2::GameEntity::grugTick(double seconds)
{
	if (specification->scriptInterface.grugTick)
	{
		return specification->scriptInterface.grugTick(this, seconds);
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

e2::Game* e2::GameEntity::game()
{
	return m_game;
}

glm::vec2 e2::GameEntity::meshPlanarCoords()
{
	return glm::vec2(meshPosition.x, meshPosition.z);
}

void e2::GameEntity::buildProxy()
{
	

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


void e2::GameEntity::setPose(e2::Pose* pose, double lerpTime)
{
	if (!skinProxy)
		return;

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

	playAction(action->pose, action->blendInTime, action->blendOutTime);
}

void e2::GameEntity::playAction(e2::AnimationPose* anim, double blendIn /*= 0.2f*/, double blendOut /*= 0.2f*/)
{
	if (!skinProxy)
		return;

	m_actionPose = anim;
	m_actionBlendInTime = blendIn;
	m_actionBlendOutTime = blendOut;

	m_actionPose->play(false);
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

	setPose(pose->pose, pose->blendTime);
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
	if (specification->scriptInterface.onTurnEnd)
	{
		specification->scriptInterface.onTurnEnd(this);
	}
}

void e2::GameEntity::onTurnStart()
{
	movePointsLeft = specification->movePoints;
	attackPointsLeft = specification->attackPoints;
	buildPointsLeft = specification->buildPoints;

	if (m_currentlyBuilding)
	{
		e2::UnitBuildResult result = m_currentlyBuilding->tick();
		m_buildMessage = result.buildMessage;
		if (result.didSpawn)
			m_currentlyBuilding = nullptr;
	}
	else
	{
		if(m_buildMessage.size() > 0)
			m_buildMessage = "";
	}

	if (specification->scriptInterface.onTurnStart)
	{
		specification->scriptInterface.onTurnStart(this);
	}
}


void e2::GameEntity::onBeginMove()
{
	if (specification->scriptInterface.onBeginMove)
	{
		specification->scriptInterface.onBeginMove(this);
		return;
	}

	setPose("move");
}

void e2::GameEntity::onEndMove()
{
	if (specification->scriptInterface.onEndMove)
	{
		specification->scriptInterface.onEndMove(this);
		return;
	}

	setPose("idle");
}

bool e2::GameEntity::canBuild(e2::UnitBuildAction& action)
{
	if (m_currentlyBuilding)
		return false;

	e2::GameEmpire* empire = game()->empireById(empireId);

	e2::ResourceTable const& funds = empire->resources.funds;
	e2::ResourceTable const& cost = action.specification->cost;
	bool affordCost = funds.gold >= cost.gold
		&& funds.metal >= cost.metal
		&& funds.meteorite >= cost.meteorite
		&& funds.oil >= cost.oil
		&& funds.stone >= cost.stone
		&& funds.uranium >= cost.uranium
		&& funds.wood >= cost.wood;

	e2::ResourceTable const& profits = empire->resources.profits;
	e2::ResourceTable const& upkeep = action.specification->upkeep;
	bool affordUpkeep = profits.gold >= upkeep.gold
		&& profits.metal >= upkeep.metal
		&& profits.meteorite >= upkeep.meteorite
		&& profits.oil >= upkeep.oil
		&& profits.stone >= upkeep.stone
		&& profits.uranium >= upkeep.uranium
		&& profits.wood >= upkeep.wood;

	return affordCost && affordUpkeep;
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
				bool extraSlotFree = game->entityAtHex(e2::EntityLayerIndex::Unit, n.offsetCoords()) == nullptr && game->hexGrid()->getCalculatedTileData(n).isPassable(specification->passableFlags);

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
			result.spawnLocation = e2::Hex(slotToUse);

			game->spawnEntity(specification->id, result.spawnLocation, owner->empireId);

			result.buildMessage = std::format("Just finished building {}.", specification->displayName);
			result.didSpawn = true;
		}
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
	std::filesystem::recursive_directory_iterator;
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

			newSpec.displayName = entity.at("displayName").template get<std::string>();

			if (entity.contains("passableFlags"))
			{
				for (json flag : entity.at("passableFlags"))
				{
					std::string flagStr = e2::toLower(flag.template get<std::string>());
					if (flagStr == "land")
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
				newSpec.maxHealth = entity.at("maxHealth").template get<double>();

			if (entity.contains("moveSpeed"))
				newSpec.moveSpeed = entity.at("moveSpeed").template get<double>();

			if (entity.contains("movePoints"))
				newSpec.movePoints = entity.at("movePoints").template get<int64_t>();

			if (entity.contains("buildPoints"))
				newSpec.buildPoints = entity.at("buildPoints").template get<int64_t>();

			if (entity.contains("attackPoints"))
				newSpec.attackPoints = entity.at("attackPoints").template get<int64_t>();

			if (entity.contains("attackStrength"))
				newSpec.attackStrength = entity.at("attackStrength").template get<double>();

			if (entity.contains("defensiveModifier"))
				newSpec.defensiveModifier = entity.at("defensiveModifier").template get<double>();

			if (entity.contains("retaliatoryModifier"))
				newSpec.retaliatoryModifier = entity.at("retaliatoryModifier").template get<double>();

			if (entity.contains("sightRange"))
				newSpec.sightRange = entity.at("sightRange").template get<int64_t>();

			if (entity.contains("meshScale"))
			{
				json& meshScale = entity.at("meshScale");
				newSpec.meshScale = glm::vec3(meshScale[0].template get<double>(), meshScale[1].template get<double>(), meshScale[2].template get<double>());
			}

			if (entity.contains("meshHeightOffset"))
				newSpec.meshHeightOffset = entity.at("meshHeightOffset").template get<double>();

			if (entity.contains("buildTurns"))
				newSpec.buildTurns = entity.at("buildTurns").template get<int64_t>();

			if (entity.contains("upkeep"))
			{
				json& upkeep = entity.at("upkeep");
				if (upkeep.contains("wood"))
					newSpec.upkeep.wood = upkeep.at("wood").template get<double>();
				if (upkeep.contains("metal"))
					newSpec.upkeep.metal = upkeep.at("metal").template get<double>();
				if (upkeep.contains("gold"))
					newSpec.upkeep.gold = upkeep.at("gold").template get<double>();
				if (upkeep.contains("stone"))
					newSpec.upkeep.stone = upkeep.at("stone").template get<double>();
				if (upkeep.contains("uranium"))
					newSpec.upkeep.uranium = upkeep.at("uranium").template get<double>();
				if (upkeep.contains("oil"))
					newSpec.upkeep.oil = upkeep.at("oil").template get<double>();
				if (upkeep.contains("meteorite"))
					newSpec.upkeep.meteorite = upkeep.at("meteorite").template get<double>();
			}

			if (entity.contains("cost"))
			{
				json& cost = entity.at("cost");
				if (cost.contains("wood"))
					newSpec.upkeep.wood = cost.at("wood").template get<double>();
				if (cost.contains("metal"))
					newSpec.upkeep.metal = cost.at("metal").template get<double>();
				if (cost.contains("gold"))
					newSpec.upkeep.gold = cost.at("gold").template get<double>();
				if (cost.contains("stone"))
					newSpec.upkeep.stone = cost.at("stone").template get<double>();
				if (cost.contains("uranium"))
					newSpec.upkeep.uranium = cost.at("uranium").template get<double>();
				if (cost.contains("oil"))
					newSpec.upkeep.oil = cost.at("oil").template get<double>();
				if (cost.contains("meteorite"))
					newSpec.upkeep.meteorite = cost.at("meteorite").template get<double>();
			}


			newSpec.meshAssetPath = entity.at("mesh").template get<std::string>();
			if (entity.contains("skeleton"))
				newSpec.skeletonAssetPath = entity.at("skeleton").template get<std::string>();

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
					newPose.blendTime = pose.at("blendTime").template get<double>();
					newPose.animationAssetPath = pose.at("asset").template get<std::string>();

					std::string poseName = pose.at("name").template get<std::string>();
					newSpec.poses[poseName] = newPose;
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
					newAction.blendInTime = action.at("blendInTime").template get<double>();
					newAction.blendOutTime = action.at("blendOutTime").template get<double>();
					newAction.animationAssetPath = action.at("asset").template get<std::string>();

					std::string actionName = action.at("name").template get<std::string>();
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
