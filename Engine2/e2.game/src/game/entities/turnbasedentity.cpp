
#include "game/entities/turnbasedentity.hpp"
#include "game/game.hpp"
#include "game/wave.hpp"
#include <e2/e2.hpp>




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

chaiscript::Boxed_Value e2::TurnbasedScriptInterface::invokeCreateState(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(chaiscript::Boxed_Value, {}, createState, entity);
}

void e2::TurnbasedScriptInterface::invokeDrawUI(e2::TurnbasedEntity* entity, e2::UIContext* ui)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(drawUI, entity, ui);
}

void e2::TurnbasedScriptInterface::invokeUpdate(e2::TurnbasedEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(update, entity, seconds);
}

void e2::TurnbasedScriptInterface::invokeUpdateAnimation(e2::TurnbasedEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(updateAnimation, entity, seconds);
}

bool e2::TurnbasedScriptInterface::invokePlayerRelevant(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, playerRelevant, entity);
}

bool e2::TurnbasedScriptInterface::invokeGrugRelevant(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, grugRelevant, entity);
}

bool e2::TurnbasedScriptInterface::invokeGrugTick(e2::TurnbasedEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE_WITH_RETURN(bool, false, grugTick, entity, seconds);
}

void e2::TurnbasedScriptInterface::invokeCollectRevenue(e2::TurnbasedEntity* entity, e2::ResourceTable& outTable)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(collectRevenue, entity, outTable);
}

void e2::TurnbasedScriptInterface::invokeCollectExpenditure(e2::TurnbasedEntity* entity, e2::ResourceTable& outTable)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(collectExpenditure, entity, outTable);
}

void e2::TurnbasedScriptInterface::invokeOnHit(e2::TurnbasedEntity* entity, e2::TurnbasedEntity* instigator, float dmg)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onHit, entity, instigator, dmg);
}

void e2::TurnbasedScriptInterface::invokeOnTargetChanged(e2::TurnbasedEntity* entity, glm::ivec2 const& hex)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTargetChanged, entity, hex);
}

void e2::TurnbasedScriptInterface::invokeOnTargetClicked(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTargetClicked, entity);
}

void e2::TurnbasedScriptInterface::invokeUpdateCustomAction(e2::TurnbasedEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(updateCustomAction, entity, seconds);
}

void e2::TurnbasedScriptInterface::invokeOnActionTrigger(e2::TurnbasedEntity* entity, e2::Name action, e2::Name trigger)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onActionTrigger, entity, action.string(), trigger.string());
}

void e2::TurnbasedScriptInterface::invokeOnTurnStart(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTurnStart, entity);
}

void e2::TurnbasedScriptInterface::invokeOnTurnEnd(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onTurnEnd, entity);
}

void e2::TurnbasedScriptInterface::invokeOnBeginMove(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onBeginMove, entity);
}

void e2::TurnbasedScriptInterface::invokeOnEndMove(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onEndMove, entity);
}

void e2::TurnbasedScriptInterface::invokeOnWaveUpdate(e2::TurnbasedEntity* entity, double seconds)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveUpdate, entity, seconds);
}

void e2::TurnbasedScriptInterface::invokeOnWavePreparing(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWavePreparing, entity);
}

void e2::TurnbasedScriptInterface::invokeOnWaveStart(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveStart, entity);
}

void e2::TurnbasedScriptInterface::invokeOnWaveEnding(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveEnding, entity);
}

void e2::TurnbasedScriptInterface::invokeOnWaveEnd(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onWaveEnd, entity);
}

void e2::TurnbasedScriptInterface::invokeOnSpawned(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onSpawned, entity);
}

void e2::TurnbasedScriptInterface::invokeOnMobSpawned(e2::TurnbasedEntity* entity, e2::Mob* mob)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onMobSpawned, entity, mob);
}

void e2::TurnbasedScriptInterface::invokeOnMobDestroyed(e2::TurnbasedEntity* entity, e2::Mob* mob)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onMobSpawned, entity, mob);
}

void e2::TurnbasedScriptInterface::invokeOnSelected(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onSelected, entity);
}

void e2::TurnbasedScriptInterface::invokeOnDeselected(e2::TurnbasedEntity* entity)
{
	E2_PROFILE_SCOPE_CTX(Scripting, entity->game());
	INVOKE_SCRIPT_HANDLE(onDeselected, entity);
}

void e2::TurnbasedScriptInterface::setCreateState(scriptFunc_createState func)
{
	createState = func;
}

void e2::TurnbasedScriptInterface::setDrawUI(scriptFunc_drawUI func)
{
	drawUI = func;
}

void e2::TurnbasedScriptInterface::setUpdate(scriptFunc_update func)
{
	update = func;
}

void e2::TurnbasedScriptInterface::setUpdateAnimation(scriptFunc_updateAnimation func)
{
	updateAnimation = func;
}

void e2::TurnbasedScriptInterface::setPlayerRelevant(scriptFunc_playerRelevant func)
{
	playerRelevant = func;
}

void e2::TurnbasedScriptInterface::setGrugRelevant(scriptFunc_grugRelevant func)
{
	grugRelevant = func;
}

void e2::TurnbasedScriptInterface::setGrugTick(scriptFunc_grugTick func)
{
	grugTick = func;
}

void e2::TurnbasedScriptInterface::setCollectRevenue(scriptFunc_fiscal func)
{
	collectRevenue = func;
}

void e2::TurnbasedScriptInterface::setCollectExpenditure(scriptFunc_fiscal func)
{
	collectExpenditure = func;
}

void e2::TurnbasedScriptInterface::setOnHit(scriptFunc_onHit func)
{
	onHit = func;
}

void e2::TurnbasedScriptInterface::setOnTargetChanged(scriptFunc_onTargetChanged func)
{
	onTargetChanged = func;
}

void e2::TurnbasedScriptInterface::setOnTargetClicked(scriptFunc_onTargetClicked func)
{
	onTargetClicked = func;
}

void e2::TurnbasedScriptInterface::setUpdateCustomAction(scriptFunc_updateCustomAction func)
{
	updateCustomAction = func;
}

void e2::TurnbasedScriptInterface::setOnActionTrigger(scriptFunc_onActionTrigger func)
{
	onActionTrigger = func;
}

void e2::TurnbasedScriptInterface::setOnTurnStart(scriptFunc_onTurnStart func)
{
	onTurnStart = func;
}

void e2::TurnbasedScriptInterface::setOnTurnEnd(scriptFunc_onTurnEnd func)
{
	onTurnEnd = func;
}

void e2::TurnbasedScriptInterface::setOnBeginMove(scriptFunc_onBeginMove func)
{
	onBeginMove = func;
}

void e2::TurnbasedScriptInterface::setOnEndMove(scriptFunc_onEndMove func)
{
	onEndMove = func;
}

void e2::TurnbasedScriptInterface::setOnWaveUpdate(scriptFunc_onWaveUpdate func)
{
	onWaveUpdate = func;
}

void e2::TurnbasedScriptInterface::setOnSpawned(scriptFunc_onSpawned func)
{
	onSpawned = func;
}

void e2::TurnbasedScriptInterface::setOnWavePreparing(scriptFunc_onWavePreparing func)
{
	onWavePreparing = func;
}

void e2::TurnbasedScriptInterface::setOnWaveStart(scriptFunc_onWaveStart func)
{
	onWaveStart = func;
}

void e2::TurnbasedScriptInterface::setOnWaveEnding(scriptFunc_onWaveEnding func)
{
	onWaveEnding = func;
}

void e2::TurnbasedScriptInterface::setOnWaveEnd(scriptFunc_onWaveEnd func)
{
	onWaveEnd = func;
}

void e2::TurnbasedScriptInterface::setOnMobSpawned(scriptFunc_onMobSpawned func)
{
	onMobSpawned = func;
}

void e2::TurnbasedScriptInterface::setOnMobDestroyed(scriptFunc_onMobDestroyed func)
{
	onMobDestroyed = func;
}

void e2::TurnbasedScriptInterface::setOnSelected(scriptFunc_onSelected func)
{
	onSelected = func;
}

void e2::TurnbasedScriptInterface::setOnDeselected(scriptFunc_onDeselected func)
{
	onDeselected = func;
}

bool e2::TurnbasedScriptInterface::hasCreateState()
{
	return createState != nullptr;
}

bool e2::TurnbasedScriptInterface::hasUpdate()
{
	return update != nullptr;
}

bool e2::TurnbasedScriptInterface::hasDrawUI()
{
	return drawUI != nullptr;
}

bool e2::TurnbasedScriptInterface::hasUpdateAnimation()
{
	return updateAnimation != nullptr;
}

bool e2::TurnbasedScriptInterface::hasPlayerRelevant()
{
	return playerRelevant != nullptr;
}


bool e2::TurnbasedScriptInterface::hasGrugRelevant()
{
	return grugRelevant != nullptr;
}

bool e2::TurnbasedScriptInterface::hasGrugTick()
{
	return grugTick != nullptr;
}

bool e2::TurnbasedScriptInterface::hasCollectRevenue()
{
	return collectRevenue != nullptr;
}

bool e2::TurnbasedScriptInterface::hasCollectExpenditure()
{
	return collectExpenditure != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnHit()
{
	return onHit != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnTargetChanged()
{
	return onTargetChanged != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnTargetClicked()
{
	return onTargetClicked != nullptr;
}

bool e2::TurnbasedScriptInterface::hasUpdateCustomAction()
{
	return updateCustomAction != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnActionTrigger()
{
	return onActionTrigger != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnTurnStart()
{
	return onTurnStart != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnTurnEnd()
{
	return onTurnEnd != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnBeginMove()
{
	return onBeginMove != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnEndMove()
{
	return onEndMove != nullptr;
}


bool e2::TurnbasedScriptInterface::hasOnWaveUpdate()
{
	return onWaveUpdate != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnWavePreparing()
{
	return onWavePreparing != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnWaveStart()
{
	return onWaveStart != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnWaveEnding()
{
	return onWaveEnding != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnWaveEnd()
{
	return onWaveEnd != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnSpawned()
{
	return onSpawned != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnMobSpawned()
{
	return onMobSpawned != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnMobDestroyed()
{
	return onMobDestroyed != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnSelected()
{
	return onSelected != nullptr;
}

bool e2::TurnbasedScriptInterface::hasOnDeselected()
{
	return onDeselected != nullptr;
}

e2::TurnbasedSpecification::TurnbasedSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::TurnbasedEntity");
}

e2::TurnbasedSpecification::~TurnbasedSpecification()
{
}

void e2::TurnbasedSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	if(obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);


	if (obj.contains("badgeId"))
	{
		badgeId = obj.at("badgeId").template get<std::string>();
	}
	else
	{
		badgeId = id;
	}

	if (obj.contains("waveRelevant"))
	{
		waveRelevant = obj.at("waveRelevant").template get<bool>();
	}

	if (obj.contains("passableFlags"))
	{
		for (nlohmann::json &flag : obj.at("passableFlags"))
		{
			std::string flagStr = e2::toLower(flag.template get<std::string>());
			if (flagStr == "none")
				passableFlags = e2::PassableFlags::None;
			else if (flagStr == "land")
				passableFlags |= e2::PassableFlags::Land;
			else if (flagStr == "watershallow")
				passableFlags |= e2::PassableFlags::WaterShallow;
			else if (flagStr == "waterdeep")
				passableFlags |= e2::PassableFlags::WaterDeep;
			else if (flagStr == "water")
				passableFlags |= e2::PassableFlags::WaterDeep | e2::PassableFlags::WaterShallow;
			else if (flagStr == "air")
				passableFlags |= e2::PassableFlags::Air;
		}
	}


	if (obj.contains("moveType"))
	{
		std::string moveTypeStr = e2::toLower(obj.at("moveType").template get<std::string>());
		if (moveTypeStr == "static")
			moveType = EntityMoveType::Static;
		else if (moveTypeStr == "linear")
			moveType = EntityMoveType::Linear;
		else if (moveTypeStr == "smooth")
			moveType = EntityMoveType::Smooth;
	}

	if (obj.contains("layerIndex"))
	{
		std::string layerIndexStr = e2::toLower(obj.at("layerIndex").template get<std::string>());
		if (layerIndexStr == "unit")
			layerIndex = EntityLayerIndex::Unit;
		else if (layerIndexStr == "air")
			layerIndex = EntityLayerIndex::Air;
		else if (layerIndexStr == "structure")
			layerIndex = EntityLayerIndex::Structure;
	}



	if (obj.contains("maxHealth"))
		maxHealth = obj.at("maxHealth").template get<float>();

	if (obj.contains("moveSpeed"))
		moveSpeed = obj.at("moveSpeed").template get<float>();

	if (obj.contains("movePoints"))
		movePoints = obj.at("movePoints").template get<int32_t>();

	if (obj.contains("buildPoints"))
		buildPoints = obj.at("buildPoints").template get<int32_t>();

	if (obj.contains("attackPoints"))
		attackPoints = obj.at("attackPoints").template get<int32_t>();


	if (obj.contains("showMovePoints"))
		showMovePoints = obj.at("showMovePoints").template get<bool>();

	if (obj.contains("showBuildPoints"))
		showBuildPoints = obj.at("showBuildPoints").template get<bool>();

	if (obj.contains("showAttackPoints"))
		showAttackPoints = obj.at("showAttackPoints").template get<bool>();


	if (obj.contains("attackStrength"))
		attackStrength = obj.at("attackStrength").template get<float>();

	if (obj.contains("defensiveModifier"))
		defensiveModifier = obj.at("defensiveModifier").template get<float>();

	if (obj.contains("retaliatoryModifier"))
		retaliatoryModifier = obj.at("retaliatoryModifier").template get<float>();

	if (obj.contains("sightRange"))
		sightRange = obj.at("sightRange").template get<int32_t>();

	if (obj.contains("attackRange"))
		attackRange = obj.at("attackRange").template get<int32_t>();

	if (obj.contains("buildTurns"))
		buildTurns = obj.at("buildTurns").template get<int32_t>();

	if (obj.contains("upkeep"))
	{
		nlohmann::json& upkeepObj = obj.at("upkeep");
		if (upkeepObj.contains("gold"))
			upkeep.gold = upkeepObj.at("gold").template get<float>();
		if (upkeepObj.contains("wood"))
			upkeep.wood = upkeepObj.at("wood").template get<float>();
		if (upkeepObj.contains("iron"))
			upkeep.iron = upkeepObj.at("iron").template get<float>();
		if (upkeepObj.contains("steel"))
			upkeep.steel = upkeepObj.at("steel").template get<float>();
	}

	if (obj.contains("cost"))
	{
		nlohmann::json& costObj = obj.at("cost");

		if (costObj.contains("gold"))
			cost.gold = costObj.at("gold").template get<float>();
		if (costObj.contains("wood"))
			cost.wood = costObj.at("wood").template get<float>();
		if (costObj.contains("iron"))
			cost.iron = costObj.at("iron").template get<float>();
		if (costObj.contains("steel"))
			cost.steel = costObj.at("steel").template get<float>();
	}

	if (obj.contains("revenue"))
	{
		nlohmann::json& revenueObj = obj.at("revenue");
		if (revenueObj.contains("gold"))
			revenue.gold = revenueObj.at("gold").template get<float>();
		if (revenueObj.contains("wood"))
			revenue.wood = revenueObj.at("wood").template get<float>();
		if (revenueObj.contains("iron"))
			revenue.iron = revenueObj.at("iron").template get<float>();
		if (revenueObj.contains("steel"))
			revenue.steel = revenueObj.at("steel").template get<float>();
	}


	if (obj.contains("script"))
	{
		chaiscript::ChaiScript* script = ctx->game()->scriptEngine();
		try
		{
			scriptInterface = script->eval_file<TurnbasedScriptInterface>(obj.at("script").template get<std::string>());

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

}

void e2::TurnbasedSpecification::finalize()
{
	mesh.finalize(game);
}



e2::TurnbasedEntity::TurnbasedEntity()
	: e2::Entity()
	, m_fog(this, 3, 1)
{

}

e2::TurnbasedEntity::~TurnbasedEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);
}


void e2::TurnbasedEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_turnbasedSpecification = m_specification->cast<e2::TurnbasedSpecification>();

	m_movePointsLeft = m_turnbasedSpecification->movePoints;
	m_attackPointsLeft = m_turnbasedSpecification->attackPoints;
	m_buildPointsLeft = m_turnbasedSpecification->buildPoints;
	m_health = m_turnbasedSpecification->maxHealth;

	if (m_turnbasedSpecification->scriptInterface.hasCreateState())
	{
		m_scriptState = m_turnbasedSpecification->scriptInterface.invokeCreateState(this);
	}

	m_mesh = e2::create<e2::SkeletalMeshComponent>(&m_turnbasedSpecification->mesh, this);

	m_fog.setRange(m_turnbasedSpecification->sightRange, 1);
}

void e2::TurnbasedEntity::setupTurnbased(glm::ivec2 const& tile, EmpireId empire)
{
	m_tileIndex = tile;
	m_empireId = empire;
	setMeshTransform(e2::Hex(m_tileIndex).localCoords(), 0.0f);
	m_transform->setRotation(glm::angleAxis(0.0f, e2::worldUpf()), e2::TransformSpace::World);
}

void e2::TurnbasedEntity::updateAnimation(double seconds)
{
	// slerp the rotation towards target
	glm::quat currentRotation = m_transform->getRotation(e2::TransformSpace::World);
	currentRotation = glm::slerp(currentRotation, m_targetRotation, glm::clamp(float(seconds) * 10.0f, 0.01f, 1.0f));
	m_transform->setRotation(currentRotation, e2::TransformSpace::World);

	m_mesh->updateAnimation(seconds);
	m_mesh->applyTransform();


	m_fog.refresh();
}

void e2::TurnbasedEntity::update(double seconds)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasUpdate())
	{
		m_turnbasedSpecification->scriptInterface.invokeUpdate(this, seconds);
		return;
	}
}

void e2::TurnbasedEntity::onActionTrigger(e2::Name action, e2::Name trigger)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnActionTrigger())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnActionTrigger(this, action, trigger);
		return;
	}
}

void e2::TurnbasedEntity::updateCustomAction(double seconds)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasUpdateCustomAction())
	{
		m_turnbasedSpecification->scriptInterface.invokeUpdateCustomAction(this, seconds);
		return;
	}
}

void e2::TurnbasedEntity::onWavePreparing()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnWavePreparing())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnWavePreparing(this);
		return;
	}
}

void e2::TurnbasedEntity::onWaveStart()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnWaveStart())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnWaveStart(this);
		return;
	}
}

void e2::TurnbasedEntity::onWaveUpdate(double seconds)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnWaveUpdate())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnWaveUpdate(this, seconds);
		return;
	}
}

void e2::TurnbasedEntity::onWaveEnding()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnWaveEnding())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnWaveEnding(this);
		return;
	}
}

void e2::TurnbasedEntity::onWaveEnd()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnWaveEnd())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnWaveEnd(this);
		return;
	}
}

void e2::TurnbasedEntity::onMobSpawned(e2::Mob* mob)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnMobSpawned())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnMobSpawned(this, mob);
		return;
	}
}

void e2::TurnbasedEntity::onMobDestroyed(e2::Mob* mob)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnMobDestroyed())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnMobDestroyed(this, mob);
		return;
	}
}

void e2::TurnbasedEntity::collectRevenue(ResourceTable& outRevenueTable)
{
	
	// if script specifies this function, run that instead and early return 
	if (m_turnbasedSpecification->scriptInterface.hasCollectRevenue())
	{
		m_turnbasedSpecification->scriptInterface.invokeCollectRevenue(this, outRevenueTable);
		return;
	}

	// if entity json specifies that revenue is scaled by abundance, then do so
	float multiplier = 1.0f;
	if (m_turnbasedSpecification->revenueByAbundance)
	{
		multiplier = game()->hexGrid()->getExistingTileData(m_tileIndex)->getAbundanceAsFloat();
	}

	// apply revenue scaled by multiplier
	outRevenueTable += m_turnbasedSpecification->revenue * multiplier;
}

void e2::TurnbasedEntity::collectExpenditure(ResourceTable& outExpenditureTable)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasCollectExpenditure())
	{
		return m_turnbasedSpecification->scriptInterface.invokeCollectExpenditure(this, outExpenditureTable);
	}

	outExpenditureTable += m_turnbasedSpecification->upkeep;
}

void e2::TurnbasedEntity::onHit(e2::TurnbasedEntity* instigator, float damage)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnHit())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnHit(this, instigator, damage);
		return;
	}
}

void e2::TurnbasedEntity::onTargetChanged(glm::ivec2 const& location)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnTargetChanged())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnTargetChanged(this, location);
		return;
	}
}

void e2::TurnbasedEntity::onTargetClicked()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasOnTargetClicked())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnTargetClicked(this);
		return;
	}
}


bool e2::TurnbasedEntity::scriptEqualityPtr(TurnbasedEntity* lhs, TurnbasedEntity* rhs)
{
	return lhs == rhs;
}

e2::TurnbasedEntity* e2::TurnbasedEntity::scriptAssignPtr(TurnbasedEntity*& lhs, TurnbasedEntity* rhs)
{
	lhs = rhs;
	return lhs;
}


void e2::TurnbasedEntity::drawUI(e2::UIContext* ctx)
{
	
	// if this function was overridden via script, invoke that instead
	if (m_turnbasedSpecification->scriptInterface.hasDrawUI())
	{
		m_turnbasedSpecification->scriptInterface.invokeDrawUI(this, ctx);
		return;
	}

	ctx->beginStackV("entityUiStackV");

	ctx->gameLabel(std::format("**{}**", m_turnbasedSpecification->displayName), 12, e2::UITextAlign::Middle);

	ctx->endStackV();
}

void e2::TurnbasedEntity::writeForSave(e2::IStream& toBuffer)
{
	e2::Entity::writeForSave(toBuffer);
	toBuffer << m_targetRotation;
		

}

void e2::TurnbasedEntity::readForSave(e2::IStream& fromBuffer)
{
	e2::Entity::readForSave(fromBuffer);
	
}

bool e2::TurnbasedEntity::playerRelevant()
{
	if (m_sleeping)
		return false;

	if (m_turnbasedSpecification->scriptInterface.hasPlayerRelevant())
	{
		updateGrugVariables();
		return m_turnbasedSpecification->scriptInterface.invokePlayerRelevant(this);
	}

	return false;
}

bool e2::TurnbasedEntity::grugRelevant()
{
	
	if (m_turnbasedSpecification->scriptInterface.hasGrugRelevant())
	{
		return m_turnbasedSpecification->scriptInterface.invokeGrugRelevant(this);
	}


	return false;
}

bool e2::TurnbasedEntity::grugTick(double seconds)
{
	
	if (m_turnbasedSpecification->scriptInterface.hasGrugTick())
	{
		return m_turnbasedSpecification->scriptInterface.invokeGrugTick(this, seconds);
	}

	return false;
}


void e2::TurnbasedEntity::setMeshTransform(glm::vec3 const& pos, float angle)
{
	m_targetRotation = glm::angleAxis(angle, e2::worldUpf());
	m_transform->setTranslation(pos, e2::TransformSpace::World);
}

e2::UnitBuildAction e2::TurnbasedEntity::createBuildAction(e2::Name unitId)
{
	return e2::UnitBuildAction(this, nullptr /** @todo */);
}

bool e2::TurnbasedEntity::isBuilding()
{
	return m_currentlyBuilding;
}

void e2::TurnbasedEntity::updateGrugVariables()
{
	e2::PathFindingAS* newAS = e2::create<e2::PathFindingAS>(this);
	grugCanMove = newAS->grugCanMove;
	grugCanAttack = newAS->grugTarget != nullptr && newAS->grugTargetMovePoints == 0;
	e2::destroy(newAS);

}

e2::Mob* e2::TurnbasedEntity::closestMobWithinRange(float range)
{
	e2::Wave* wave = game()->wave();
	if (!wave)
		return nullptr;

	float closestDistance = std::numeric_limits<float>::max();
	e2::Mob* closestMob = nullptr;

	for (e2::Mob* mob : wave->mobs)
	{
		float currentDistance = glm::distance(mob->meshPlanarCoords(), planarCoords());
		if (!mob || currentDistance < closestDistance)
		{
			closestMob = mob;
			closestDistance = currentDistance;
		}
	}

	return closestMob;
}

bool e2::TurnbasedEntity::isLocal()
{
	return m_empireId == game()->localEmpireId();
}

void e2::TurnbasedEntity::onTurnEnd()
{
	if (m_turnbasedSpecification->scriptInterface.hasOnTurnEnd())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnTurnEnd(this);
	}
}

void e2::TurnbasedEntity::onTurnStart()
{
	m_movePointsLeft = m_turnbasedSpecification->movePoints;
	m_attackPointsLeft = m_turnbasedSpecification->attackPoints;
	m_buildPointsLeft = m_turnbasedSpecification->buildPoints;

	if (m_currentlyBuilding)
	{
		e2::UnitBuildResult result = m_currentlyBuilding->tick();
		m_buildMessage = result.buildMessage;
		if (result.didSpawn)
			m_currentlyBuilding = nullptr;
	}
	else
	{
		if (m_buildMessage.size() > 0)
			m_buildMessage = "";
	}

	if (m_turnbasedSpecification->scriptInterface.hasOnTurnStart())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnTurnStart(this);
	}
}


void e2::TurnbasedEntity::onBeginMove()
{
	if (m_turnbasedSpecification->scriptInterface.hasOnBeginMove())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnBeginMove(this);
		return;
	}

	m_mesh->setPose("move");
}

void e2::TurnbasedEntity::onEndMove()
{
	if (m_turnbasedSpecification->scriptInterface.hasOnEndMove())
	{
		m_turnbasedSpecification->scriptInterface.invokeOnEndMove(this);
		return;
	}

	m_mesh->setPose("idle");
}

void e2::TurnbasedEntity::onSpawned()
{
	if (m_turnbasedSpecification->scriptInterface.hasOnSpawned())
		m_turnbasedSpecification->scriptInterface.invokeOnSpawned(this);
}

void e2::TurnbasedEntity::turnTowards(glm::ivec2 const& hex)
{
	float angle = e2::radiansBetween(e2::Hex(hex).localCoords(), e2::Hex(m_tileIndex).localCoords());
	m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
}

void e2::TurnbasedEntity::turnTowardsPlanar(glm::vec3 const& planar)
{
	float angle = e2::radiansBetween(planar, m_transform->getTranslation(e2::TransformSpace::World));
	m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
}

bool e2::TurnbasedEntity::canBuild(e2::UnitBuildAction& action)
{
	if (m_currentlyBuilding)
		return false;

	e2::GameEmpire* empire = game()->empireById(m_empireId);

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



void e2::TurnbasedEntity::build(e2::UnitBuildAction& action)
{
	if (canBuild(action))
	{
		m_currentlyBuilding = &action;
		m_buildMessage = std::format("Building {}, {} turns left.", m_currentlyBuilding->specification->displayName, m_currentlyBuilding->buildTurnsLeft);
	}
}

void e2::TurnbasedEntity::cancelBuild()
{
	m_currentlyBuilding = nullptr;
	m_buildMessage = "";
}

e2::UnitBuildAction::UnitBuildAction(e2::TurnbasedEntity* inOwner, e2::TurnbasedSpecification const* spec)
	: owner(inOwner)
	, specification(spec)
{
	buildTurnsLeft = specification->buildTurns;
}

e2::UnitBuildResult e2::UnitBuildAction::tick()
{
	if (!owner || !specification)
		return {};

	e2::UnitBuildResult result;
	e2::Game* game = owner->game();
	glm::ivec2 const& tileIndex = owner->getTileIndex();
	// subtract turn
	if (buildTurnsLeft > 0)
		buildTurnsLeft--;

	// if we are done
	if (buildTurnsLeft <= 0)
	{

		bool hasSlot = false;
		glm::ivec2 slotToUse = tileIndex;
		bool mainSlotFree = game->entityAtHex(e2::EntityLayerIndex::Unit, slotToUse) == nullptr;
		if (mainSlotFree)
			hasSlot = true;
		else
		{
			for (e2::Hex& n : e2::Hex(tileIndex).neighbours())
			{
				slotToUse = n.offsetCoords();
				bool extraSlotFree = game->entityAtHex(e2::EntityLayerIndex::Unit, slotToUse) == nullptr && game->hexGrid()->calculateTileData(slotToUse).isPassable(specification->passableFlags);

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

			//game->spawnEntity(specification->id, result.spawnLocation, owner->empireId); // @todo

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
