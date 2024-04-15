
#include "game/game.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/audiomanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"
#include "game/mob.hpp"

#include "game/militaryunit.hpp"
#include "game/builderunit.hpp"

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <fmod_errors.h>

#pragma warning(disable : 4996)

#include <filesystem>
#include <Shlobj.h>
#include <ctime>

e2::Game::Game(e2::Context* ctx)
	: e2::Application(ctx)
{
	readAllSaveMetas();
}

e2::Game::~Game()
{
	
}

void e2::Game::saveGame(uint8_t slot)
{
	if (m_empireTurn != m_localEmpireId)
	{
		LogError("refuse to save game on AI turn");
		return;
	}

	e2::SaveMeta newMeta;
	newMeta.exists = true;
	newMeta.timestamp = std::time(nullptr);
	newMeta.slot = slot;

	e2::Buffer buf;

	buf << int64_t(newMeta.timestamp);

	m_hexGrid->saveToBuffer(buf);

	buf << m_startViewOrigin;
	buf << m_viewOrigin;
	buf << m_turn;

	// store all ais 
	for (EmpireId i = 1; i < e2::maxNumEmpires-1; i++)
	{
		buf << int32_t(m_empires[i] != nullptr ? 1 : 0);

		if (m_empires[i])
		{
			// @todo store ai shit here yo 
		}

	}

	buf << uint64_t(m_structures.size());
	for (e2::GameStructure* structure : m_structures)
	{
		// structure type 
		buf << structure->type()->fqn;

		// structure empire id 
		buf << int64_t(structure->empireId);

		// structure tileIndex 
		buf << structure->tileIndex;

		structure->writeForSave(buf);
	}

	buf << uint64_t(m_units.size());
	for (e2::GameUnit* unit : m_units)
	{
		// unit type 
		buf << unit->type()->fqn;

		// unit empire id 
		buf << int64_t(unit->empireId);

		// unit tileIndex 
		buf << unit->tileIndex;

		unit->writeForSave(buf);
	}


	// write discovered empires 
	buf << uint64_t(m_discoveredEmpires.size());
	for (e2::GameEmpire* discEm : m_discoveredEmpires)
		buf << int64_t(discEm->id);


	buf.writeToFile(newMeta.fileName());

	saveSlots[slot] = newMeta;

}

e2::SaveMeta e2::Game::readSaveMeta(uint8_t slot)
{
	e2::SaveMeta meta;
	meta.slot = slot;
	meta.cachedFileName = meta.fileName();

	// header is just int64_t timestamp
	constexpr uint64_t headerSize = 8 ;

	e2::Buffer buf;
	uint64_t bytesRead = buf.readFromFile(meta.cachedFileName, 0, headerSize);
	if (bytesRead != headerSize)
	{
		meta.cachedDisplayName = meta.displayName();
		saveSlots[slot] = meta;
		return meta;
	}

	meta.exists = true;
	int64_t time{};
	buf >> time;
	meta.timestamp = time;

	meta.cachedDisplayName = meta.displayName();


	saveSlots[slot] = meta;

	

	return meta;

}

void e2::Game::readAllSaveMetas()
{
	for (uint8_t slot = 0; slot < e2::numSaveSlots; slot++)
		readSaveMeta(slot);
}

void e2::Game::loadGame(uint8_t slot)
{
	// header is just uint64_t discoveredTiles + int64_t timestamp
	constexpr uint64_t headerSize = 8 ;

	e2::Buffer buf;
	uint64_t readBytes = buf.readFromFile(saveSlots[slot].fileName(), headerSize, 0);
	if (readBytes == 0)
	{
		LogError("save slot missing or corrupted"); 
		return;
	}

	nukeGame();
	setupGame();

	m_hexGrid->loadFromBuffer(buf);

	buf >> m_startViewOrigin;
	buf >> m_viewOrigin;
	buf >> m_turn;

	m_localEmpireId = spawnEmpire();
	m_localEmpire = m_empires[m_localEmpireId];
	discoverEmpire(m_localEmpireId);

	for (EmpireId i = 1; i < e2::maxNumEmpires - 1; i++)
	{
		int32_t hasAiEmpire = 0;
		buf >> hasAiEmpire;

		if (hasAiEmpire != 0)
		{
			// @todo store ai shit here yo 
			m_empires[i] = e2::create<e2::GameEmpire>(this, i);
			m_undiscoveredEmpires.insert(m_empires[i]);
			m_aiEmpires.insert(m_empires[i]);
			m_empires[i]->ai = e2::create<e2::CommanderAI>(this, i);


		}

	}




	uint64_t numStructures{};
	buf >> numStructures;
	for (uint64_t i = 0; i < numStructures; i++)
	{
		e2::Name fqn;
		buf >> fqn;

		int64_t empireId{};
		buf >> empireId;

		glm::ivec2 tileIndex;
		buf >> tileIndex;

		e2::Type* ty = e2::Type::fromName(fqn);
		if (!ty || !ty->inherits("e2::GameStructure"))
		{
			LogError("no such type, or doesn't inherit proper parent, corrupted save");
			continue;
		}

		if (empireId > (std::numeric_limits<EmpireId>::max)())
		{
			LogError("unsupported datatype in savefile, try updating your game before loading this save!");
			continue;
		}

		e2::GameStructure* newStructure = ty->create()->cast<e2::GameStructure>();
		newStructure->postConstruct(this, tileIndex, (EmpireId)empireId);

		newStructure->readForSave(buf);

		postSpawnStructure(newStructure);
	}



	uint64_t numUnits{};
	buf >> numUnits;
	for (uint64_t i = 0; i < numUnits; i++)
	{
		e2::Name fqn;
		buf >> fqn;

		int64_t empireId{};
		buf >> empireId;

		glm::ivec2 tileIndex;
		buf >> tileIndex;

		e2::Type* ty = e2::Type::fromName(fqn);
		if (!ty || !ty->inherits("e2::GameUnit"))
		{
			LogError("no such type, or doesn't inherit proper parent, corrupted save");
			continue;
		}

		if (empireId > (std::numeric_limits<EmpireId>::max)())
		{
			LogError("unsupported datatype in savefile, try updating your game before loading this save!");
			continue;
		}


		e2::GameUnit* newUnit = ty->create()->cast<e2::GameUnit>();
		newUnit->postConstruct(this, tileIndex, (EmpireId)empireId);

		newUnit->readForSave(buf);

		postSpawnUnit(newUnit);
	}

	uint64_t numDiscoveredEmpires{};
	buf >> numDiscoveredEmpires;
	for (uint64_t i = 0; i < numDiscoveredEmpires;i++)
	{
		int64_t empId;
		buf >> empId;

		if (empId > (std::numeric_limits<EmpireId>::max)())
		{
			LogError("unsupported datatype in savefile, try updating your game before loading this save!");
			continue;
		}


		discoverEmpire((EmpireId)empId);
	}

	startGame();
}

void e2::Game::setupGame()
{
	m_hexGrid = e2::create<e2::HexGrid>(this);

	m_startViewOrigin = glm::vec2(528.97f, 587.02f);
	m_viewOrigin = m_startViewOrigin;
	m_hexGrid->initializeWorldBounds(m_viewOrigin);
}

void e2::Game::nukeGame()
{
	e2::ITexture* outlineTextures[2] = {nullptr, nullptr};
	m_session->renderer()->setOutlineTextures(outlineTextures);

	deselectStructure();
	deselectUnit();

	m_unitsPendingDestroy.clear();

	auto unitsClone = m_units;
	for (GameUnit* unit : unitsClone)
		destroyUnit(unit->tileIndex);

	auto structuresClone = m_structures;
	for (GameStructure* structure: structuresClone)
		destroyStructure(structure->tileIndex);

	for (EmpireId i = 0; i < e2::maxNumEmpires-1; i++)
	{
		destroyEmpire(i);
	}

	if (m_hexGrid)
	{

		asyncManager()->waitForPredicate([this]()->bool {
			return m_hexGrid->numJobsInFlight() == 0;
		});
		e2::destroy(m_hexGrid);
		m_hexGrid = nullptr;
	}

	m_globalState = GlobalState::Menu;
	m_inGameMenuState = InGameMenuState::Main;
	m_state = GameState::TurnPreparing;
	m_turn = 0;
	m_empireTurn = 0;
	m_turnState = TurnState::Unlocked;
	m_timeDelta = 0.0;
	m_localEmpireId = 0;
	m_localEmpire = nullptr;
}

void e2::Game::exitToMenu()
{
	nukeGame();
	setupGame();
}

void e2::Game::finalizeBoot()
{
	auto am = assetManager();

	m_uiTextureResources = am->get("assets/UI_ResourceIcons.e2a").cast<e2::Texture2D>();

	m_irradianceMap = am->get("assets/kloofendal_irr.e2a").cast<e2::Texture2D>();
	m_radianceMap = am->get("assets/kloofendal_rad.e2a").cast<e2::Texture2D>();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());

	m_entityMeshes.resize(size_t(e2::EntityType::Count));
	m_entitySkeletons.resize(size_t(e2::EntityType::Count));
	for (uint8_t i = 0; i < size_t(e2::EntityType::Count); i++)
	{
		m_entityMeshes[i] = am->get("assets/structures/SM_BuildingPlaceholder.e2a").cast<e2::Mesh>();
		m_entitySkeletons[i] = nullptr;
	}

	m_animationIndex.resize(uint8_t(e2::AnimationIndex::Count));

	for (uint8_t i = 0; i < (uint8_t)e2::AnimationIndex::Count; i++)
	{
		m_animationIndex[i] = am->get("assets/characters/A_SoldierDance.e2a").cast<e2::Animation>();
	}


	m_entityMeshes[size_t(e2::EntityType::Structure_OreMine)] = am->get("assets/environment/SM_Mine.e2a").cast<e2::Mesh>();
	m_entityMeshes[size_t(e2::EntityType::Structure_GoldMine)] = am->get("assets/environment/SM_GoldMine.e2a").cast<e2::Mesh>();
	m_entityMeshes[size_t(e2::EntityType::Structure_UraniumMine)] = am->get("assets/environment/SM_UraniumMine.e2a").cast<e2::Mesh>();
	m_entityMeshes[size_t(e2::EntityType::Structure_Quarry)] = am->get("assets/environment/SM_Quarry.e2a").cast<e2::Mesh>();
	m_entityMeshes[size_t(e2::EntityType::Structure_SawMill)] = am->get("assets/environment/SM_SawMill.e2a").cast<e2::Mesh>();





	m_entityMeshes[size_t(e2::EntityType::Unit_Grunt)] = am->get("assets/characters/SM_Soldier.e2a").cast<e2::Mesh>();
	m_entitySkeletons[size_t(e2::EntityType::Unit_Grunt)] = am->get("assets/characters/SK_Soldier.e2a").cast<e2::Skeleton>();

	m_entityMeshes[size_t(e2::EntityType::Unit_AssaultCraft)] = am->get("assets/vehicles/SM_Boat.e2a").cast<e2::Mesh>();
	m_entitySkeletons[size_t(e2::EntityType::Unit_AssaultCraft)] = am->get("assets/vehicles/SK_Boat.e2a").cast<e2::Skeleton>();

	m_entityMeshes[size_t(e2::EntityType::Unit_Tank)] = am->get("assets/vehicles/SM_Tank.e2a").cast<e2::Mesh>();
	m_entitySkeletons[size_t(e2::EntityType::Unit_Tank)] = am->get("assets/vehicles/SK_Tank.e2a").cast<e2::Skeleton>();

	m_entityMeshes[size_t(e2::EntityType::Unit_MobileMOB)] = am->get("assets/vehicles/SM_MobileMob.e2a").cast<e2::Mesh>();


	m_entityMeshes[size_t(e2::EntityType::Unit_Engineer)] = am->get("assets/characters/SM_Engineer.e2a").cast<e2::Mesh>();
	m_entitySkeletons[size_t(e2::EntityType::Unit_Engineer)] = am->get("assets/characters/SK_Engineer.e2a").cast<e2::Skeleton>();

	m_animationIndex[(uint8_t)e2::AnimationIndex::SoldierIdle] = am->get("assets/characters/A_SoldierIdle.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::SoldierHit] = am->get("assets/characters/A_SoldierHit.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::SoldierFire] = am->get("assets/characters/A_SoldierFire.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::SoldierDie] = am->get("assets/characters/A_SoldierDie.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::SoldierRun] = am->get("assets/characters/A_SoldierRun.e2a").cast<e2::Animation>();

	m_animationIndex[(uint8_t)e2::AnimationIndex::EngineerIdle] = am->get("assets/characters/A_EngineerIdle.e2a").cast<e2::Animation>();
	//m_animationIndex[(uint8_t)e2::AnimationIndex::EngineerHit] = am->get("assets/characters/A_SoldierHit.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::EngineerBuild] = am->get("assets/characters/A_EngineerBuild.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::EngineerDie] = am->get("assets/characters/A_EngineerDie.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::EngineerRun] = am->get("assets/characters/A_EngineerRun.e2a").cast<e2::Animation>();


	m_animationIndex[(uint8_t)e2::AnimationIndex::CombatBoatDrive] = am->get("assets/vehicles/A_BoatDriving.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::CombatBoatIdle] = am->get("assets/vehicles/A_BoatIdle.e2a").cast<e2::Animation>();

	m_animationIndex[(uint8_t)e2::AnimationIndex::TankDrive] = am->get("assets/vehicles/A_Tank_Moving.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::TankFire] = am->get("assets/vehicles/A_Tank_Fire.e2a").cast<e2::Animation>();
	m_animationIndex[(uint8_t)e2::AnimationIndex::TankIdle] = am->get("assets/vehicles/A_Tank_Idle.e2a").cast<e2::Animation>();


	m_testSound = am->get("assets/audio/S_Test.e2a").cast<e2::Sound>();

	am->returnALJ(m_bootTicket);

	m_empires.resize(e2::maxNumEmpires);
	for (EmpireId i = 0; uint64_t(i) < e2::maxNumEmpires - 1; i++)
	{
		m_empires[i] = nullptr;
	}

	setupGame();

#if defined(E2_PROFILER)
	profiler()->start();
#endif
}

void e2::Game::initialize()
{


	m_session = e2::create<e2::GameSession>(this);

	m_bootBegin = e2::timeNow();

	auto am = assetManager();

	e2::ALJDescription alj;

	am->prescribeALJ(alj, "assets/audio/S_Test.e2a");

	am->prescribeALJ(alj, "assets/SM_HexBase.e2a");
	am->prescribeALJ(alj, "assets/SM_HexBaseHigh.e2a");

	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest001.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest002.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest003.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PineForest004.e2a");

	am->prescribeALJ(alj, "assets/environment/SM_MineTrees.e2a");


	am->prescribeALJ(alj, "assets/UI_ResourceIcons.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PalmTree001.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PalmTree002.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PalmTree003.e2a");
	am->prescribeALJ(alj, "assets/environment/trees/SM_PalmTree004.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_Mine.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_GoldMine.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_UraniumMine.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_Quarry.e2a");
	am->prescribeALJ(alj, "assets/environment/SM_SawMill.e2a");

	am->prescribeALJ(alj, "assets/kloofendal_irr.e2a");
	am->prescribeALJ(alj, "assets/kloofendal_rad.e2a");

	am->prescribeALJ(alj, "assets/characters/SM_Gus.e2a");
	am->prescribeALJ(alj, "assets/characters/SK_Gus.e2a");
	am->prescribeALJ(alj, "assets/characters/A_GusWave.e2a");

	am->prescribeALJ(alj, "assets/characters/SK_Soldier.e2a");
	am->prescribeALJ(alj, "assets/characters/SM_Soldier.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierDance.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierIdle.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierHit.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierFire.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierDie.e2a");
	am->prescribeALJ(alj, "assets/characters/A_SoldierRun.e2a");


	am->prescribeALJ(alj, "assets/characters/SK_Engineer.e2a");
	am->prescribeALJ(alj, "assets/characters/SM_Engineer.e2a");
	am->prescribeALJ(alj, "assets/characters/A_EngineerIdle.e2a");
	am->prescribeALJ(alj, "assets/characters/A_EngineerBuild.e2a");
	am->prescribeALJ(alj, "assets/characters/A_EngineerDie.e2a");
	am->prescribeALJ(alj, "assets/characters/A_EngineerRun.e2a");





	am->prescribeALJ(alj, "assets/vehicles/SK_Boat.e2a");
	am->prescribeALJ(alj, "assets/vehicles/SM_Boat.e2a");
	am->prescribeALJ(alj, "assets/vehicles/A_BoatIdle.e2a");
	am->prescribeALJ(alj, "assets/vehicles/A_BoatDriving.e2a");

	am->prescribeALJ(alj, "assets/vehicles/SK_Tank.e2a");
	am->prescribeALJ(alj, "assets/vehicles/SM_Tank.e2a");
	am->prescribeALJ(alj, "assets/vehicles/A_Tank_Idle.e2a");
	am->prescribeALJ(alj, "assets/vehicles/A_Tank_Moving.e2a");
	am->prescribeALJ(alj, "assets/vehicles/A_Tank_Fire.e2a");


	am->prescribeALJ(alj, "assets/vehicles/SM_MobileMob.e2a");

	am->prescribeALJ(alj, "assets/structures/SM_BuildingPlaceholder.e2a");

	m_bootTicket = am->queueALJ(alj);
	
}

void e2::Game::shutdown()
{

	nukeGame();

	e2::destroy(m_session);

	m_irradianceMap = nullptr;
	m_radianceMap = nullptr;
	m_uiTextureResources = nullptr;

	for (uint32_t i = 0; i < m_animationIndex.size(); i++)
	{
		m_animationIndex[i] = nullptr;
	}

	for (uint32_t i = 0; i < m_entityMeshes.size(); i++)
	{
		m_entityMeshes[i] = nullptr;
	}

	for (uint32_t i = 0; i < m_entitySkeletons.size(); i++)
	{
		m_entitySkeletons[i] = nullptr;
	}



}

void e2::Game::update(double seconds)
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();

	if (m_globalState == GlobalState::Boot)
	{
		m_session->tick(seconds);

		glm::vec2 winSize = m_session->window()->size();
		float textWidth = m_session->uiContext()->calculateSDFTextWidth(FontFace::Serif, 22.0f, "Loading... ");
		m_session->uiContext()->drawSDFTextCarousel(FontFace::Serif, 22.f, 0xFFFFFFFF, { winSize.x - textWidth - 16.f, winSize.y - 32.0f }, "Loading... ", 8.0f, m_bootBegin.durationSince().seconds() * 4.0f);

		if (assetManager()->queryALJ(m_bootTicket).status == ALJStatus::Completed && m_bootBegin.durationSince().seconds() > 4.0f)
		{
			m_globalState = GlobalState::Menu;
			finalizeBoot();
		}
		return;
	}
	 
	auto& kb = ui->keyboardState();
	
	if (kb.pressed(Key::Enter) && kb.state(Key::LeftAlt))
	{
		session->window()->setFullscreen(!session->window()->isFullscreen());
	}

	if (kb.pressed(e2::Key::O))
	{
		FMOD_RESULT result = audioManager()->coreSystem()->playSound(m_testSound->fmodSound(), nullptr, false, nullptr);
		if (result != FMOD_OK)
		{
			LogError("Fmod: {}: {}", int32_t(result), FMOD_ErrorString(result));
		}
		else
		{
			LogError("Fmod OK!");
		}
	}


	m_timeDelta = seconds;
	if (m_globalState == GlobalState::Menu)
		updateMenu(seconds);
	else if (m_globalState == GlobalState::Game)
		updateGame(seconds);
	else if (m_globalState == GlobalState::InGameMenu)
		updateInGameMenu(seconds);

	auto view = renderer->view();
	glm::dmat4 listenerTransform = glm::mat4(1.0f);
	listenerTransform = glm::translate(listenerTransform, view.origin);
	listenerTransform = listenerTransform * glm::toMat4(view.orientation);

	audioManager()->setListenerTransform(listenerTransform);
	
}

void e2::Game::updateInGameMenu(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());


	if (kb.pressed(Key::Escape))
	{
		if (m_inGameMenuState == InGameMenuState::Main)
			m_globalState = GlobalState::Game;
		else
			m_inGameMenuState = InGameMenuState::Main;
	}

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	glm::vec2 size(256.0f, 320.0f);
	glm::vec2 offset = (glm::vec2(ui->size()) / 2.0f) - (size / 2.0f);

	ui->pushFixedPanel("ingameMenuContainer", offset, size);

	if (m_inGameMenuState == InGameMenuState::Main)
	{


		ui->beginStackV("ingameMenu");

		if (ui->button("btnCont", "Continue"))
			m_globalState = GlobalState::Game;

		if (ui->button("btnSave", "Save.."))
			m_inGameMenuState = InGameMenuState::Save;

		if (ui->button("btnLoad", "Load.."))
			m_inGameMenuState = InGameMenuState::Load;

		if (ui->button("btnExitMenu", "Exit to Menu"))
			exitToMenu();

		if (ui->button("btnQuitDesktop", "Quit to Desktop"))
		{
			nukeGame();
			engine()->shutdown();
		}

		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Save)
	{
		ui->beginStackV("saveMenu");

		for (uint8_t i = 0; i < e2::numSaveSlots; i++)
		{
			if (ui->button(std::format("save{}", uint32_t(i)), saveSlots[i].displayName()))
				saveGame(i);
		}


		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Load)
	{
		ui->beginStackV("loadMenu");

		for (uint8_t i = 0; i < e2::numSaveSlots; i++)
		{
			if (ui->button(std::format("load{}", uint32_t(i)), saveSlots[i].displayName()))
				loadGame(i);
		}


		ui->endStackV();
	}
	else if (m_inGameMenuState == InGameMenuState::Options)
	{
	}

	ui->popFixedPanel();
}

void e2::Game::updateGame(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());



	// update input state
	glm::vec2 resolution = renderer->resolution();
	m_cursor = mouse.relativePosition;
	m_cursorUnit = (m_cursor / resolution);
	m_cursorNdc = m_cursorUnit * 2.0f - 1.0f;
	m_cursorPlane = renderer->view().unprojectWorldPlane(renderer->resolution(), m_cursorNdc);
	e2::Hex newHex = e2::Hex(m_cursorPlane);
	m_hexChanged = newHex != m_prevCursorHex;
	m_prevCursorHex = m_cursorHex;
	m_cursorHex = newHex;

	m_cursorTile = m_hexGrid->getTileData(m_cursorHex.offsetCoords());

	constexpr float invisibleChunkLifetime = 3.0f;

	// debug buttons 
	if (kb.keys[int16_t(e2::Key::Num1)].pressed)
	{
		m_altView = !m_altView;
		gameSession()->window()->mouseLock(m_altView);
		if (m_altView)
		{
			m_altViewOrigin =glm::vec3(m_viewOrigin.x, -5.0f, m_viewOrigin.y);
		}
			

	}



	if (kb.keys[int16_t(e2::Key::F1)].pressed)
	{
		m_hexGrid->clearAllChunks();
	}

	if (kb.keys[int16_t(e2::Key::F2)].pressed)
	{
		m_hexGrid->clearLoadTime();
	}

	if (kb.keys[int16_t(e2::Key::F5)].pressed)
	{
		renderManager()->invalidatePipelines();
	}

	if (kb.keys[int16_t(e2::Key::F9)].pressed)
	{
		m_hexGrid->invalidateFogOfWarShaders();
	}

#if defined(E2_PROFILER)
	if (kb.keys[int16_t(e2::Key::F4)].pressed)
	{
		profiler()->stop();
		auto rep =profiler()->report();
		std::stringstream ss;
		ss << std::endl;
		ss << "------------------------------" << std::endl;
		ss << "Profiler report, for " << profiler()->frameCount() << " frames" << std::endl;
		ss << "------------------------------" << std::endl;
		ss << "Groups:" << std::endl;

		for (auto& g : rep.groups)
		{
			ss << std::format("{}: avg frame {:.3f} ms, high frame {:.3f} ms", g.displayName, g.avgTimeInFrame * 1000.0, g.highTimeInFrame * 1000.0) << std::endl;
		}

		for (auto& f : rep.functions)
		{
			ss << std::format("{}: avg {:.3f} ms, high {:.3f} ms | avg frame {:.3f} ms, high frame {:.3f} ms", f.displayName, f.avgTimeInScope * 1000.0, f.highTimeInScope * 1000.0, f.avgTimeInFrame * 1000.0, f.highTimeInFrame * 1000.0) << std::endl;
		}
		LogNotice("{}", ss.str());
		profiler()->start();
	}
#endif


	if (kb.pressed(Key::Escape))
	{
		m_globalState = GlobalState::InGameMenu;
	}

	updateCamera(seconds);

	if (m_hexChanged)
	{
		onNewCursorHex();
	}

	updateGameState();

	updateAnimation(seconds);

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	//ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->outlineTexture(renderManager()->frameIndex()));

	drawUI();

	/*
	if (m_selectedUnit)
	{
		int32_t i = 0;
		e2::Hex prev;
		for (e2::Hex h : m_unitHoverPath)
		{
			if (i++ == 0)
			{
				prev = h;
				continue;
			}

			glm::vec3 start = prev.localCoords();
			glm::vec3 end = h.localCoords();
			renderer->debugLine({ 0.0, 0.0, 1.0 }, start + e2::worldUp() * 0.2f , end + e2::worldUp() * 0.2f);
			prev = h;
		}

		for (auto& [index, hexAS] : m_unitAS.hexIndex)
		{
			if (!hexAS->towardsOrigin)
				continue;

			glm::vec3 start = e2::Hex(hexAS->index).localCoords();
			glm::vec3 end = e2::Hex(hexAS->towardsOrigin->index).localCoords();
			renderer->debugLine({ 1.0, 0.0, 0.0 }, start, end);
		}
	}*/

	//ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->fogOfWarMask());

	/*
	renderer->debugLine(glm::vec3(1.0f), m_viewPoints.topLeft, m_viewPoints.topRight);
	renderer->debugLine(glm::vec3(1.0f), m_viewPoints.topRight, m_viewPoints.bottomRight);
	renderer->debugLine(glm::vec3(1.0f), m_viewPoints.bottomRight, m_viewPoints.bottomLeft);
	renderer->debugLine(glm::vec3(1.0f), m_viewPoints.bottomLeft, m_viewPoints.topLeft);

	renderer->debugLine(glm::vec3(1.0f, 0.0f, 0.0f), m_viewPoints.leftRay.position, m_viewPoints.leftRay.position  + m_viewPoints.leftRay.parallel);
	renderer->debugLine(glm::vec3(1.0f, 0.0f, 0.0f), m_viewPoints.topRay.position, m_viewPoints.topRay.position + m_viewPoints.topRay.parallel);
	renderer->debugLine(glm::vec3(1.0f, 0.0f, 0.0f), m_viewPoints.rightRay.position, m_viewPoints.rightRay.position + m_viewPoints.rightRay.parallel);
	renderer->debugLine(glm::vec3(1.0f, 0.0f, 0.0f), m_viewPoints.bottomRay.position, m_viewPoints.bottomRay.position + m_viewPoints.bottomRay.parallel);
	renderer->debugLine(glm::vec3(0.0f, 1.0f, 0.0f), m_viewPoints.leftRay.position, m_viewPoints.leftRay.position + m_viewPoints.leftRay.perpendicular);
	renderer->debugLine(glm::vec3(0.0f, 1.0f, 0.0f), m_viewPoints.topRay.position, m_viewPoints.topRay.position + m_viewPoints.topRay.perpendicular);
	renderer->debugLine(glm::vec3(0.0f, 1.0f, 0.0f), m_viewPoints.rightRay.position, m_viewPoints.rightRay.position + m_viewPoints.rightRay.perpendicular);
	renderer->debugLine(glm::vec3(0.0f, 1.0f, 0.0f), m_viewPoints.bottomRay.position, m_viewPoints.bottomRay.position + m_viewPoints.bottomRay.perpendicular);*/

	for (e2::GameUnit* unit : m_unitsPendingDestroy)
	{
		destroyUnit(unit->tileIndex);
	}
	m_unitsPendingDestroy.clear();
}

void e2::Game::updateMenu(double seconds)
{
	static double timer = 0.0;
	static int64_t frames = 0;
	frames++;
	if(frames> 5)
		timer += seconds;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());

	if (!m_haveBegunStart && kb.pressed(Key::Escape))
		timer = 20.0;

	m_viewZoom = 0.5f;


	m_view = calculateRenderView(m_viewOrigin);
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view);
	renderer->setView(m_view);


	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	glm::vec2 resolution = renderer->resolution();
	



	if (m_mainMenuState != MainMenuState::Main)
	{
		if (kb.pressed(Key::Escape))
		{
			m_mainMenuState = MainMenuState::Main;
		}
		double width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0, "Reveal & Annihilate");


		double menuHeight = 280.0;
		double menuOffset = resolution.y / 2.0 - menuHeight / 2.0;
		double cursorY = menuOffset;
		double xOffset = resolution.x / 2.0 - width / 2.0;

		if (m_mainMenuState == MainMenuState::Load)
		{
			for (uint8_t sl = 0; sl < e2::numSaveSlots; sl++)
			{
				double slotWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, saveSlots[sl].cachedDisplayName);

				bool slotHovered = mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + slotWidth
					&& mouse.relativePosition.y > cursorY && mouse.relativePosition.y < cursorY + 40.0;

				ui->drawSDFText(FontFace::Serif, 24.0f, 0x000000FF, glm::vec2(xOffset, cursorY + 20.0), saveSlots[sl].cachedDisplayName);
				if (saveSlots[sl].exists && slotHovered && leftMouse.clicked)
				{
					loadGame(sl);
				}

				cursorY += 40.0;
			}
		}



		return;
	}



	double globalMenuFade = m_haveBegunStart ? double(1.0 - glm::smoothstep(0.0, 2.0, m_beginStartTime.durationSince().seconds())) : 1.0;

	// tinted block
	double blockTimer1 = glm::smoothstep(12.0, 15.0, double(timer));
	double blockAlpha = 0.9f - blockTimer1 * 0.9f;
	ui->drawQuad({}, resolution, e2::UIColor(102, 88, 66, uint8_t(blockAlpha * 255.0)));

	// black screen
	double blockAlpha2 = 1.0 - glm::smoothstep(8.0, 10.0, double(timer));
	ui->drawQuad({}, resolution, e2::UIColor(0, 0, 0, uint8_t(blockAlpha2 * 255.0)));


	double authorWidth = ui->calculateSDFTextWidth(FontFace::Serif, 36.0f, "Fredrik Haikarainen");
	double authorWidth2 = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "A game by");
	double authorAlpha = (double)glm::smoothstep(3.0, 4.0, timer);
	double authorAlpha2 = (double)glm::smoothstep(2.0, 3.0, timer);

	double authorFadeOut = double(1.0 - glm::smoothstep(6.0, 7.0, timer));
	authorAlpha *= authorFadeOut;
	authorAlpha2 *= authorFadeOut;

	e2::UIColor authorColor(255, 255, 255, uint8_t(authorAlpha * 255.0));
	e2::UIColor authorColor2(255, 255, 255, uint8_t(authorAlpha2 * 255.0));
	ui->drawSDFText(FontFace::Serif, 36.0, authorColor, glm::vec2(resolution.x / 2.0 - authorWidth / 2.0, resolution.y / 2.0), "Fredrik Haikarainen");
	ui->drawSDFText(FontFace::Serif, 14.0, authorColor2, glm::vec2(resolution.x / 2.0 - authorWidth2 / 2.0, (resolution.y / 2.0) - 36.0), "A game by");

	double width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0f, "Reveal & Annihilate");

	double menuHeight = 280.0;
	double menuOffset = resolution.y / 2.0 - menuHeight / 2.0;

	double cursorY = menuOffset;
	double xOffset = resolution.x / 2.0 - width / 2.0;


	double blockAlpha3 = (double)glm::smoothstep(10.0, 12.0, timer);

	e2::UIColor textColor(0, 0, 0, uint8_t(blockAlpha3 * 255.0 * globalMenuFade));
	e2::UIColor textColor2(0, 0, 0, uint8_t(blockTimer1 * 170.0 * globalMenuFade));
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor, glm::vec2(xOffset, cursorY), "Reveal & Annihilate");

	static double a = 0.0;
	a += seconds;

	// outwards 
	double sa = glm::simplex(glm::vec2(a*2.0, 0.0));
	// rotation
	double sb = glm::simplex(glm::vec2(a*50.0 + 21.f, 0.0));

	sa = glm::pow(sa, 4.0);
	sb = glm::pow(sb, 2.0);

	if (glm::abs(sa) < 0.2f)
		sa = 0.0;

	if (glm::abs(sb) < 0.5f)
		sb = 0.0;

	glm::vec2 ori(0.0f, float(-sa) * 10.0f);
	glm::vec2 offset1 = e2::rotate2d(ori, float(sb) * 360.0f);

	glm::vec2 ori2(0.0f, float(-sb) * 5.0f);
	glm::vec2 offset2 = e2::rotate2d(ori2, float(sb) * 360.0f);



	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset1, "Reveal & Annihilate");
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset2, "Reveal & Annihilate");

	cursorY += 64.0;

	double blockAlphaNewGame = glm::smoothstep(15.0, 15.25, timer);
	e2::UIColor textColorNewGame(0, 0, 0, uint8_t(blockAlphaNewGame * 170.0 * globalMenuFade));
	double newGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "New Game");
	bool newGameHovered = !m_haveBegunStart && timer > 15.25&& mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + newGameWidth
		&& mouse.relativePosition.y > cursorY + 20.0 && mouse.relativePosition.y < cursorY + 60.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, newGameHovered ? 0x000000FF : textColorNewGame, glm::vec2(xOffset, cursorY + 40.0), "New Game");
	if (!m_haveBegunStart && newGameHovered && leftMouse.clicked)
	{
		beginStartGame();
	}

	double blockAlphaLoadGame = glm::smoothstep(15.25, 15.5, timer);
	e2::UIColor textColorLoadGame(0, 0, 0, uint8_t(blockAlphaLoadGame * 170.0 * globalMenuFade));
	double loadGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Load Game");
	bool loadGameHovered = !m_haveBegunStart && timer > 15.5 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + loadGameWidth
		&& mouse.relativePosition.y > cursorY + 60.0 && mouse.relativePosition.y < cursorY + 100.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, loadGameHovered ? 0x000000FF : textColorLoadGame, glm::vec2(xOffset, cursorY + 40.0 * 2), "Load Game");
	if (!m_haveBegunStart && loadGameHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Load;
	}



	double blockAlphaOptions = glm::smoothstep(15.5, 15.75, timer);
	e2::UIColor textColorOptions(0, 0, 0, uint8_t(blockAlphaOptions * 170.0 * globalMenuFade));
	double optionsWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Options");
	bool optionsHovered = !m_haveBegunStart && timer > 15.75 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 100.0 && mouse.relativePosition.y < cursorY + 140.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, optionsHovered ? 0x000000FF : textColorOptions, glm::vec2(xOffset, cursorY + 40.0 * 3), "Options");
	if (!m_haveBegunStart && optionsHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Options;
	}


	double blockAlphaQuit = glm::smoothstep(15.75, 16.0, timer);
	e2::UIColor textColorQuit(0, 0, 0, uint8_t(blockAlphaQuit * 170.0 * globalMenuFade));
	double quitWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Quit");
	bool quitHovered = !m_haveBegunStart && timer > 16.0 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 140.0 && mouse.relativePosition.y < cursorY + 180.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, quitHovered ? 0x000000FF : textColorQuit, glm::vec2(xOffset, cursorY + 40.0 * 4), "Quit");
	if (quitHovered && leftMouse.clicked)
	{

		nukeGame();

		engine()->shutdown();
	}

	if (m_haveBegunStart && !m_haveStreamedStart && m_hexGrid->numJobsInFlight() == 0 && m_beginStartTime.durationSince().seconds() > 0.25f)
	{
		m_haveStreamedStart = true;
		m_beginStreamTime = e2::timeNow();

		glm::vec2 startCoords = e2::Hex(m_startLocation).planarCoords();
		m_hexGrid->initializeWorldBounds(startCoords);

		// spawn local empire
		m_localEmpireId = spawnEmpire();
		m_localEmpire = m_empires[m_localEmpireId];
		discoverEmpire(m_localEmpireId);
		spawnUnit<e2::MobileMOB>(m_startLocation, m_localEmpireId);

	}
	if (m_haveStreamedStart)
	{
		double sec = m_beginStreamTime.durationSince().seconds();
		double a = glm::smoothstep(0.0, 2.0, sec);
		m_viewOrigin = glm::mix(m_startViewOrigin, e2::Hex(m_startLocation).planarCoords(), glm::exponentialEaseInOut(a));
	}

	if (m_haveStreamedStart && m_beginStreamTime.durationSince().seconds() > 2.1)
	{
		
		resumeWorldStreaming();
		startGame();
		m_haveBegunStart = false;
		m_haveStreamedStart = false;
	}

}

void e2::Game::pauseWorldStreaming()
{
	m_hexGrid->m_streamingPaused = true;
	m_hexGrid->clearQueue();
}

void e2::Game::resumeWorldStreaming()
{
	m_hexGrid->m_streamingPaused = false;
}

void e2::Game::forceStreamLocation(glm::vec2 const& planarCoords)
{
	glm::vec2 offset = planarCoords - m_viewOrigin;
	e2::Viewpoints2D forceView = m_viewPoints;
	forceView.bottomLeft += offset;
	forceView.bottomRight += offset;
	forceView.topLeft += offset;
	forceView.topRight += offset;
	forceView.view.origin.x += offset.x;
	forceView.view.origin.z += offset.y;
	forceView.calculateDerivatives();

	m_hexGrid->forceStreamView(forceView);
}

void e2::Game::beginStartGame()
{
	m_haveBegunStart = true;
	m_beginStartTime = e2::timeNow();
	do {} while (!findStartLocation(e2::randomIvec2(glm::ivec2(-1024), glm::ivec2(1024)), { 1024, 1024 }, m_startLocation, false));
	pauseWorldStreaming();
	forceStreamLocation(e2::Hex(m_startLocation).planarCoords());
}

bool e2::Game::findStartLocation(glm::ivec2 const& offset, glm::ivec2 const& rangeSize, glm::ivec2 &outLocation, bool forAi)
{
	glm::ivec2 returner;
	int32_t numTries = 0;

	// plop us down somehwere nice 
	std::unordered_set<glm::ivec2> attemptedStartLocations;
	bool foundStartLocation{};
	while (!foundStartLocation)
	{
		glm::ivec2 rangeStart = offset - (rangeSize / 2);
		glm::ivec2 rangeEnd = offset + (rangeSize / 2);
		glm::ivec2 startLocation = e2::randomIvec2(rangeStart, rangeEnd);

		//glm::ivec2 startLocation = e2::randomIvec2({ -512, -512 }, { 512, 512 });
		if (attemptedStartLocations.contains(startLocation))
			continue;

		attemptedStartLocations.insert(startLocation);

		e2::Hex startHex(startLocation);
		e2::TileData startTile = m_hexGrid->calculateTileDataForHex(startHex);
		if (!startTile.isPassable(e2::PassableFlags::Land))
			continue;

		constexpr bool ignoreVisibility = true;
		auto as = e2::create<e2::PathFindingAccelerationStructure>(this, startHex, 64, ignoreVisibility, e2::PassableFlags::Land);
		uint64_t numWalkableHexes = as->hexIndex.size();
		e2::destroy(as);

		if (numWalkableHexes < 64)
			continue;

		if (forAi)
		{

		}

		returner = startLocation;
		foundStartLocation = true;

		if (numTries++ >= 20)
			break;
	}

	if(foundStartLocation)
		outLocation = returner;

	return foundStartLocation;
}

void e2::Game::startGame()
{
	if (m_globalState != GlobalState::Menu)
		return;
	m_globalState = GlobalState::Game;

	// kick off gameloop
	onStartOfTurn();


}

glm::vec2 e2::Game::worldToPixels(glm::vec3 const& world)
{
	auto renderer = gameSession()->renderer();
	glm::dvec2 resolution = renderer->resolution();
	glm::dmat4 vpMatrix = game()->view().calculateProjectionMatrix(resolution) * game()->view().calculateViewMatrix();

	glm::vec4 viewPos = vpMatrix * glm::dvec4(glm::dvec3(world), 1.0);
	viewPos = viewPos / viewPos.z;

	glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(resolution);
	return offset;

}

e2::ApplicationType e2::Game::type()
{
	return e2::ApplicationType::Game;
}

e2::Game* e2::Game::game()
{
	return this;
}

void e2::Game::updateCamera(double seconds)
{
	updateMainCamera(seconds);
	updateAltCamera(seconds);
}

void e2::Game::updateGameState()
{
	if (m_state == GameState::TurnPreparing)
	{
		// do turn prepare things here , and then move on to turn when done 
		bool notDoneYet = false;
		if (notDoneYet)
			return;

		onTurnPreparingEnd();
		m_state = GameState::Turn;
		onStartOfTurn();
	}
	else if (m_state == GameState::Turn)
	{
		if (m_turnState == TurnState::Unlocked)
			updateTurn();
		else if (m_turnState == TurnState::UnitAction_Move)
			updateUnitMove();
		else if (m_turnState == TurnState::EntityAction_Target)
			updateEntityTarget();
		else if (m_turnState == TurnState::EntityAction_Generic)
			updateEntityAction();
			
	}
	else if (m_state == GameState::TurnEnding)
	{
		bool notDoneYet = false;
		if (notDoneYet)
			return;

		onTurnEndingEnd();

		// flip through to next empire to let the mhave its turn, if it doesnt exist, keep doing it until we find one or we run out
		m_empireTurn++;
		while (!m_empires[m_empireTurn])
		{
			if (m_empireTurn < e2::maxNumEmpires - 1)
				m_empireTurn++;
			else
			{
				m_empireTurn = 0;
				m_turn++;
				break;
			}
		}
		
		m_state = GameState::TurnPreparing;
		onTurnPreparingBegin();
	}
}

void e2::Game::updateTurn()
{
	// if local is making turn
	if (m_empireTurn == 0)
		updateTurnLocal();
	else
		updateTurnAI();
}

void e2::Game::updateTurnLocal()
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	auto& leftMouse = mouse.buttons[uint8_t(e2::MouseButton::Left)];
	auto& rightMouse = mouse.buttons[uint8_t(e2::MouseButton::Right)];

	// turn logic here
	if (!m_uiHovered && leftMouse.clicked && leftMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftShift))
		{
			if(m_cursorTile && m_cursorTile->getWater() == TileFlags::WaterNone)
				spawnUnit<e2::Tank>(m_cursorHex, 0);
			else 
				spawnUnit<e2::CombatBoat>(m_cursorHex, 0);
			return;
		}
		e2::GameUnit* unitAtHex = nullptr;
		e2::GameStructure* structureAtHex = nullptr;

		
		auto finder = m_unitIndex.find(m_cursorHex.offsetCoords());
		if (finder != m_unitIndex.end())
			unitAtHex = finder->second;
		
		if (unitAtHex && unitAtHex->empireId != m_localEmpireId)
			unitAtHex = nullptr;

		auto finder2 = m_structureIndex.find(m_cursorHex.offsetCoords());
		if (finder2 != m_structureIndex.end())
			structureAtHex = finder2->second;

		if (structureAtHex && structureAtHex->empireId != m_localEmpireId)
			structureAtHex = nullptr;

		if (unitAtHex && unitAtHex == m_selectedUnit && structureAtHex)
			selectStructure(structureAtHex);
		else if (unitAtHex && unitAtHex != m_selectedUnit)
			selectUnit(unitAtHex);
		else if (structureAtHex && structureAtHex == m_selectedStructure && unitAtHex)
			selectUnit(unitAtHex);
		else if (structureAtHex && structureAtHex != m_selectedStructure)
			selectStructure(structureAtHex);
		else if (structureAtHex)
			selectStructure(structureAtHex);
		else if (unitAtHex)
			selectUnit(unitAtHex);
		else if (!structureAtHex && !unitAtHex)
			deselect();
	}

	if (!m_uiHovered && rightMouse.clicked && rightMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftShift))
		{
			LogNotice("destroying unit at {}", m_cursorHex.offsetCoords());
			destroyUnit(m_cursorHex);
			return;
		}

		moveSelectedUnitTo(m_cursorHex);
	}
}

void e2::Game::updateTurnAI()
{
	e2::GameEmpire* empire = m_empires[m_empireTurn];
	if (empire->ai)
		empire->ai->grugBrainTick();
}


void e2::Game::endTurn()
{
	if (m_state != GameState::Turn || m_turnState != TurnState::Unlocked)
		return;

	onEndOfTurn();
	m_state = GameState::TurnEnding;
	onTurnEndingBegin();
}

void e2::Game::onTurnPreparingBegin()
{

}

void e2::Game::onTurnPreparingEnd()
{

}

void e2::Game::onStartOfTurn()
{
	// ignore visibility for AI since we dont want to see what theyre up to 
	if (m_empireTurn == m_localEmpireId)
	{
		while (m_undiscoveredEmpires.size() < 3)
			spawnAIEmpire();


		// clear and calculate visibility
		m_hexGrid->clearVisibility();

		for (e2::GameUnit* unit : m_localEmpire->units)
		{
			unit->spreadVisibility();
		}

		for (e2::GameStructure* structure : m_localEmpire->structures)
		{
			structure->spreadVisibility();
		}
	}

	m_empires[m_empireTurn]->resources.onNewTurn();

	for (e2::GameUnit* unit : m_empires[m_empireTurn]->units)
	{
		unit->onTurnStart();
	}

	for (e2::GameStructure* structure : m_empires[m_empireTurn]->structures)
	{
		structure->onTurnStart();
	}
}

void e2::Game::onEndOfTurn()
{
	for (e2::GameUnit* unit : m_empires[m_empireTurn]->units)
	{
		unit->onTurnEnd();
	}

	for (e2::GameStructure* structure : m_empires[m_empireTurn]->structures)
	{
		structure->onTurnEnd();
	}
}

void e2::Game::onTurnEndingBegin()
{

}

void e2::Game::onTurnEndingEnd()
{
	deselect();
}

void e2::Game::updateUnitMove()
{

	if (m_selectedUnit->moveType == UnitMoveType::Linear)
	{
		m_unitMoveDelta += float(m_timeDelta) * m_selectedUnit->moveSpeed;

		while (m_unitMoveDelta > 1.0f)
		{
			m_unitMoveDelta -= 1.0;
			m_unitMoveIndex++;

			if (m_unitMoveIndex >= m_unitMovePath.size() - 1)
			{
				e2::Hex prevHex = m_unitMovePath[m_unitMovePath.size() - 2];
				e2::Hex finalHex = m_unitMovePath[m_unitMovePath.size() - 1];
				// we are done
				if (m_selectedUnit->isLocal())
					m_selectedUnit->rollbackVisibility();

				m_unitIndex.erase(m_selectedUnit->tileIndex);
				m_selectedUnit->tileIndex = finalHex.offsetCoords();
				m_unitIndex[m_selectedUnit->tileIndex] = m_selectedUnit;

				if (m_selectedUnit->isLocal())
					m_selectedUnit->spreadVisibility();



				float angle = e2::radiansBetween(finalHex.localCoords(), prevHex.localCoords());
				m_selectedUnit->setMeshTransform(finalHex.localCoords(), angle);

				resolveSelectedUnit();

				m_selectedUnit->onEndMove();

				m_turnState = TurnState::Unlocked;

				return;
			}
		}

		e2::Hex currHex = m_unitMovePath[m_unitMoveIndex];
		e2::Hex nextHex = m_unitMovePath[m_unitMoveIndex + 1];

		glm::vec3 currHexPos = currHex.localCoords();
		glm::vec3 nextHexPos = nextHex.localCoords();

		glm::vec3 newPos = glm::mix(currHexPos, nextHexPos, m_unitMoveDelta);
		float angle = radiansBetween(nextHexPos, currHexPos);
		m_selectedUnit->setMeshTransform(newPos, angle);
	}
	else if (m_selectedUnit->moveType == e2::UnitMoveType::Smooth)
	{

		m_unitMoveDelta += float(m_timeDelta) * m_selectedUnit->moveSpeed;

		while (m_unitMoveDelta > 1.0f)
		{
			m_unitMoveDelta -= 1.0;
			m_unitMoveIndex++;

			if (m_unitMoveIndex >= m_unitMovePath.size() - 1)
			{
				e2::Hex prevHex = m_unitMovePath[m_unitMovePath.size() - 2];
				e2::Hex finalHex = m_unitMovePath[m_unitMovePath.size() - 1];
				// we are done
				if (m_selectedUnit->isLocal())
					m_selectedUnit->rollbackVisibility();

				m_unitIndex.erase(m_selectedUnit->tileIndex);
				m_selectedUnit->tileIndex = finalHex.offsetCoords();
				m_unitIndex[m_selectedUnit->tileIndex] = m_selectedUnit;

				if (m_selectedUnit->isLocal())
					m_selectedUnit->spreadVisibility();



				float angle = e2::radiansBetween(finalHex.localCoords(), prevHex.localCoords());
				m_selectedUnit->setMeshTransform(finalHex.localCoords(), angle);


				resolveSelectedUnit();


				m_selectedUnit->onEndMove();

				m_turnState = TurnState::Unlocked;

				return;
			}
		}


		e2::Hex currHex = m_unitMovePath[m_unitMoveIndex];
		e2::Hex nextHex = m_unitMovePath[m_unitMoveIndex + 1];

		glm::vec3 currHexPos = currHex.localCoords();
		glm::vec3 nextHexPos = nextHex.localCoords();


		glm::vec3 prevHexPos = m_unitMoveIndex > 0 ? m_unitMovePath[m_unitMoveIndex - 1].localCoords() : currHexPos - (nextHexPos - currHexPos);

		glm::vec3 nextNextHexPos = m_unitMoveIndex + 2 < m_unitMovePath.size() ? m_unitMovePath[m_unitMoveIndex + 2].localCoords() : nextHexPos + (nextHexPos - currHexPos);


		glm::vec3 newPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta);
		glm::vec3 newPos2 = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta + 0.01f);


		constexpr bool debugRender = false;
		if (debugRender)
		{
			for (uint32_t k = 0; k < m_unitMovePath.size() - 1; k++)
			{

				e2::Hex currHex = m_unitMovePath[k];
				e2::Hex nextHex = m_unitMovePath[k + 1];

				glm::vec3 currHexPos = currHex.localCoords();
				glm::vec3 nextHexPos = nextHex.localCoords();


				glm::vec3 prevHexPos = k > 0 ? m_unitMovePath[k - 1].localCoords() : currHexPos - (nextHexPos - currHexPos);

				glm::vec3 nextNextHexPos = k + 2 < m_unitMovePath.size() ? m_unitMovePath[k + 2].localCoords() : nextHexPos + (nextHexPos - currHexPos);


				constexpr uint32_t debugResolution = 8;
				for (uint32_t i = 0; i < debugResolution; i++)
				{
					float ii = glm::clamp(float(i) / float(debugResolution), 0.0f, 1.0f);
					float ii2 = glm::clamp(float(i + 1) / float(debugResolution), 0.0f, 1.0f);
					glm::vec3 currPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, ii);
					glm::vec3 nextPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, ii2);

					gameSession()->renderer()->debugLine(glm::vec3(1.0f, ii, 1.0f), currPos, nextPos);
				}
			}
		}


		float angle = radiansBetween(newPos2, newPos);
		m_selectedUnit->setMeshTransform(newPos, angle);
	}
}

void e2::Game::updateEntityTarget()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::GameEntity* ent = selectedEntity();
	if (!ent)
		return;

	if (m_hexChanged)
	{
		ent->onEntityTargetChanged(m_cursorHex);
	}

	if (leftMouse.clicked)
	{
		ent->onEntityTargetClicked();
	}
}

void e2::Game::postSpawnUnit(e2::GameUnit* unit)
{
	unit->initialize();

	m_units.insert(unit);
	m_unitIndex[unit->tileIndex] = unit;

	if (m_empires[unit->empireId])
		m_empires[unit->empireId]->units.insert(unit);

	m_empires[unit->empireId]->resources.fiscalStreams.insert(unit);
}

void e2::Game::drawUI()
{
	m_uiHovered = false;

	if (!m_altView)
	{
		drawResourceIcons();

		drawStatusUI();

		drawUnitUI();

		drawMinimapUI();

		drawFinalUI();
	}

	drawDebugUI();
}

void e2::Game::drawResourceIcons()
{
	// @todo move this to the respective structures instead 
	return;

	/*
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();


	glm::dvec2 resolution = renderer->resolution();
	glm::dmat4 vpMatrix = m_view.calculateProjectionMatrix(resolution) * m_view.calculateViewMatrix();



	for (e2::ChunkState* chunk : m_hexGrid->m_chunksInView)
	{
		glm::ivec2 chunkIndex = chunk->chunkIndex;
		glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::hexChunkResolution);
		
		for (int32_t y = 0; y < e2::hexChunkResolution; y++)
		{
			for (int32_t x = 0; x < e2::hexChunkResolution; x++)
			{
				glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
				
				e2::TileData* tileData = m_hexGrid->getTileData(worldIndex);
				if (!tileData)
					continue;

				if (!m_hexGrid->isVisible(worldIndex))
					continue;

				e2::TileFlags resource = tileData->getResource();
				e2::TileFlags abundance = tileData->getAbundance();

				glm::vec4 viewPos = vpMatrix* glm::dvec4( glm::dvec3(e2::Hex(worldIndex).localCoords()) + e2::worldUp() * 0.4, 1.0);
				viewPos = viewPos / viewPos.z;

				glm::vec2 offset = (glm::vec2(viewPos.x, viewPos.y) * 0.5f + 0.5f) * glm::vec2(resolution);

				



				auto drawIcon = [abundance, offset, ui, this](glm::vec2 const& uvOffset)	{
					glm::vec2 uvScale = { 1.0f / 7.0f, 1.0f };
					if (abundance == TileFlags::Abundance1)
					{
						glm::vec2 size(32.0f, 32.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset, size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance2)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance3)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-6.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(2.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 0.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
					else if (abundance == TileFlags::Abundance4)
					{
						glm::vec2 size(24.0f, 24.0f);
						glm::vec2 iconOffset = offset - (size / 2.0f);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, -8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(-8.0f, 8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
						ui->drawTexturedQuad(iconOffset + glm::vec2(8.0f, 8.0f), size, 0xFFFFFFFF, m_uiTextureResources->handle(), uvOffset, uvScale);
					}
				};

				// gold 
				if(resource == e2::TileFlags::ResourceGold)
					drawIcon({ (1.0f / 7.0f) * 0.0f, 0.0f });
				// wood
				else if (resource == e2::TileFlags::ResourceForest && tileData->improvedResource)
					drawIcon({ (1.0f / 7.0f) * 1.0f, 0.0f });
				// stone 
				else if (resource == e2::TileFlags::ResourceStone && tileData->improvedResource)
					drawIcon({ (1.0f / 7.0f) * 2.0f, 0.0f });
				// metal
				else if (resource == e2::TileFlags::ResourceOre)
					drawIcon({ (1.0f / 7.0f) * 3.0f, 0.0f });
				// oil
				else if (resource == e2::TileFlags::ResourceOil)
					drawIcon({ (1.0f / 7.0f) * 4.0f, 0.0f });
				// uranium
				else if (resource == e2::TileFlags::ResourceUranium)
					drawIcon({ (1.0f / 7.0f) * 5.0f, 0.0f });
			}
		}
	}*/

}

void e2::Game::drawStatusUI()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	ui->drawQuadShadow({ -2.0f, -2.0f }, { winSize.x + 4.0f, 30.0f }, 2.0f, 0.9f, 2.0f);

	float xCursor = 0.0f;
	uint8_t fontSize = 12;

	std::string str;
	float strWidth;

	e2::GameResources& resources = m_empires[m_localEmpireId]->resources;

	// Gold
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 0.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.gold, resources.profits.gold);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f}, str);
	xCursor += strWidth;

	// Wood
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 1.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.wood, resources.profits.wood);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Stone
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 2.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.stone, resources.profits.stone);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Metal
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 3.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.metal, resources.profits.metal);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Oil
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 4.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.oil, resources.profits.oil);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Uranium
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 5.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.uranium, resources.profits.uranium);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Meteorite
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 6.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{:} ({:+})", resources.funds.meteorite, resources.profits.meteorite);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;


	str = std::format("Turn {}", m_turn);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	xCursor = winSize.x - strWidth - 16.0f;
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	

	/*ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 1.0f), 14.0f }, std::format("Wood: {0:10} ({0:+})", m_resources.funds.wood, m_resources.profits.wood));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 2.0f), 14.0f }, std::format("Stone: {0:10} ({0:+})", m_resources.funds.stone, m_resources.profits.stone));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 3.0f), 14.0f }, std::format("Metal: {0:10} ({0:+})", m_resources.funds.metal, m_resources.profits.metal));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 4.0f), 14.0f }, std::format("Oil: {0:10} ({0:+})", m_resources.funds.oil, m_resources.profits.oil));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 5.0f), 14.0f }, std::format("Uranium: {0:10} ({0:+})", m_resources.funds.uranium, m_resources.profits.uranium));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 6.0f), 14.0f }, std::format("Meteorite: {0:10} ({0:+})", m_resources.funds.meteorite, m_resources.profits.meteorite));*/



}


void e2::Game::drawUnitUI()
{
	if (!m_selectedUnit && !m_selectedStructure)
		return;
	
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	float width = 450.0f;
	float height = 180.0f;

	glm::vec2 offset = { winSize.x / 2.0 - width / 2.0, winSize.y - height - 16.0f };

	if (mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height)
		m_uiHovered = true;

	ui->drawQuadShadow(offset, {width, height}, 8.0f, 0.9f, 4.0f);
	//uint8_t fontSize = 12;
	//std::string str = m_selectedUnit->displayName;
	//ui->drawRasterText(e2::FontFace::Serif, 14, 0xFFFFFFFF, offset + glm::vec2(8.f, 14.f), str);

	ui->pushFixedPanel("test", offset + glm::vec2(4.0f, 4.0f), glm::vec2(width - 8.0f, height - 8.0f));

	if (m_selectedUnit && m_turnState == TurnState::Unlocked)
		m_selectedUnit->drawUI(ui);
	else if (m_selectedStructure && m_turnState == TurnState::Unlocked)
		m_selectedStructure->drawUI(ui);

	ui->popFixedPanel();


}

void e2::Game::drawMinimapUI()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];
	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	glm::uvec2 miniSize = m_hexGrid->minimapSize();

	float width = (float)miniSize.x;
	float height = (float)miniSize.y;

	glm::vec2 offset = {  16.0f , winSize.y - height - 16.0f };

	bool hovered = !m_viewDragging && 
		(mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height);
	if(hovered)
	{
		m_uiHovered = true;
	}


	ui->drawQuadShadow(offset- glm::vec2(4.0f, 4.0f), {width + 8.0f, height + 8.0f}, 8.0f, 0.9f, 4.0f);
	ui->drawTexturedQuad(offset, { width, height }, 0xFFFFFFFF, m_hexGrid->minimapTexture(renderManager()->frameIndex()));

	if (hovered && leftMouse.state)
	{
		e2::Aabb2D viewBounds = m_hexGrid->viewBounds();
		glm::vec2 normalizedMouse = (mouse.relativePosition - offset) / glm::vec2(width, height);
		glm::vec2 worldMouse = viewBounds.min + normalizedMouse * (viewBounds.max - viewBounds.min);
		m_viewOrigin = worldMouse;
	}
}

void e2::Game::drawDebugUI()
{
#if defined(E2_PROFILER)
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::EngineMetrics& metrics = engine()->metrics();

	float yOffset = 64.0f;
	float xOffset = 12.0f;
	ui->drawQuadShadow({ 0.0f, yOffset - 16.0f }, { 320.0f, 200.0f }, 8.0f, 0.9f, 4.0f);
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset }, std::format("^2Avg. {:.1f} ms, potential fps: {:.1f}", metrics.frameTimeMsMean, 1000.0f / metrics.frameTimeMsMean));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 1.0f) }, std::format("^3High {:.1f} ms, potential fps: {:.1f}", metrics.frameTimeMsHigh, 1000.0f / metrics.frameTimeMsHigh));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 2.0f) }, std::format("^4Real fps: {:.1f}", metrics.realCpuFps));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 3.0f) }, std::format("^5Num. chunks: {}", m_hexGrid->numChunks()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 4.0f) }, std::format("^6Visible chunks: {}", m_hexGrid->numVisibleChunks()));
	//ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 5.0f) }, std::format("^7Num. chunk meshes: {}", m_hexGrid->numChunkMeshes()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 6.0f) }, std::format("^8High loadtime: {:.2f}ms", m_hexGrid->highLoadTime()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 7.0f) }, std::format("^9Jobs in flight: {}", m_hexGrid->numJobsInFlight()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 8.0f) }, std::format("^2View Origin: {}", m_viewOrigin));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 9.0f) }, std::format("^3View Velocity: {}", m_viewVelocity));
#endif
}

void e2::Game::drawFinalUI()
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	e2::IWindow* wnd = session->window();
	glm::vec2 winSize = wnd->size();

	float width = 180.0f;
	float height = 180.0f;

	glm::vec2 offset = { winSize.x - width - 16.0f, winSize.y - height - 16.0f };

	if (mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height)
		m_uiHovered = true;

	ui->drawQuadShadow(offset, { width, height }, 8.0f, 0.9f, 4.0f);
	//uint8_t fontSize = 12;
	//std::string str = m_selectedUnit->displayName;
	//ui->drawRasterText(e2::FontFace::Serif, 14, 0xFFFFFFFF, offset + glm::vec2(8.f, 14.f), str);

	ui->pushFixedPanel("test", offset + glm::vec2(4.0f, 4.0f), glm::vec2(width - 8.0f, height - 8.0f));
	ui->beginStackV("test2");

	if (ui->button("te", "End turn"))
	{
		endTurn();
	}

	ui->endStackV();
	ui->popFixedPanel();
}

void e2::Game::onNewCursorHex()
{
	if (m_selectedUnit)
	{
		m_unitHoverPath = m_unitAS->find(m_cursorHex);
	}
}

e2::EmpireId e2::Game::spawnEmpire()
{
	for (e2::EmpireId i = 0; i < m_empires.size(); i++)
	{
		if (!m_empires[i])
		{
			m_empires[i] = e2::create<e2::GameEmpire>(this, i);
			m_undiscoveredEmpires.insert(m_empires[i]);
			return i;
		}
	}

	LogError("failed to spawn empire, we full rn");
	return 0;
}

void e2::Game::destroyEmpire(EmpireId empireId)
{
	e2::GameEmpire* empire = m_empires[empireId];
	m_empires[empireId] = nullptr;

	if (!empire)
		return;

	m_aiEmpires.erase(empire);
	m_discoveredEmpires.erase(empire);
	m_undiscoveredEmpires.erase(empire);

	if (empire->ai)
		e2::destroy(empire->ai);

	e2::destroy(empire);
}

void e2::Game::spawnAIEmpire()
{
	e2::EmpireId newEmpireId = spawnEmpire();
	if (newEmpireId == 0)
		return;

	e2::GameEmpire* newEmpire = m_empires[newEmpireId];
	m_aiEmpires.insert(newEmpire);

	newEmpire->ai = e2::create<e2::CommanderAI>(this, newEmpireId);

	e2::Aabb2D worldBounds = m_hexGrid->worldBounds();
	bool foundLocation = false;
	glm::ivec2 aiStartLocation;
	do
	{
		glm::vec2 random = e2::randomVec2(worldBounds.min, worldBounds.max);
		float distLeft = glm::abs(worldBounds.min.x - random.x);
		float distRight = glm::abs(worldBounds.max.x - random.x);
		float distUp = glm::abs(worldBounds.min.y - random.y);
		float distDown = glm::abs(worldBounds.max.y - random.y);
		glm::vec2 offsetDir{};

		if (distLeft < distRight && distLeft < distUp && distLeft < distDown)
		{
			random.x = worldBounds.min.x;
			offsetDir = { -1.0f, 0.0f };
		}
		else if (distRight < distLeft && distRight < distUp && distRight < distDown)
		{
			random.x = worldBounds.max.x;
			offsetDir = { 1.0f, 0.0f };
		}
		else if (distUp < distRight && distUp < distLeft && distUp < distDown)
		{
			random.y = worldBounds.min.y;
			offsetDir = { 0.0f, -1.0f };
		}
		else
		{
			random.y = worldBounds.max.y;
			offsetDir = { 0.0f, 1.0f };
		}

		glm::vec2 offsetPoint = random + offsetDir * 64.0f;

		foundLocation = findStartLocation(e2::Hex(offsetPoint).offsetCoords(), { 64, 64 }, aiStartLocation, true);
	} while (!foundLocation);


	spawnStructure<e2::MainOperatingBase>(aiStartLocation, newEmpireId);
	

}

void e2::Game::deselect()
{
	deselectUnit();
	deselectStructure();
}

void e2::Game::applyDamage(e2::GameEntity* entity, e2::GameEntity* instigator, float damage)
{
	entity->onHit(instigator, damage);
}

void e2::Game::resolveSelectedUnit()
{
	if (m_unitAS)
		e2::destroy(m_unitAS);
	m_unitAS = e2::create<PathFindingAccelerationStructure>(m_selectedUnit);

	m_hexGrid->clearOutline();

	for (auto& [coords, hexAS] : m_unitAS->hexIndex)
	{
		m_hexGrid->pushOutline(coords);
	}
}

void e2::Game::unresolveSelectedUnit()
{
	if (m_unitAS)
		e2::destroy(m_unitAS);
	m_unitAS = nullptr;

	m_hexGrid->clearOutline();
}

void e2::Game::selectUnit(e2::GameUnit* unit)
{
	if (!unit || unit == m_selectedUnit)
		return;


	deselectStructure();

	m_selectedUnit = unit;

	resolveSelectedUnit();
}

void e2::Game::deselectUnit()
{
	if (!m_selectedUnit)
		return;

	unresolveSelectedUnit();

	m_selectedUnit = nullptr;
}

void e2::Game::moveSelectedUnitTo(e2::Hex const& to)
{
	if (m_state != GameState::Turn || m_turnState != TurnState::Unlocked || !m_selectedUnit)
		return;

	if (to.offsetCoords() == m_selectedUnit->tileIndex)
		return;

	m_unitMovePath = m_unitAS->find(to);
	if (m_unitMovePath.size() == 0)
		return;
	else if (m_unitMovePath.size() - 1 > m_selectedUnit->movePointsLeft )
		return;

	m_hexGrid->clearOutline();

	m_selectedUnit->movePointsLeft -= int32_t(m_unitMovePath.size()) - 1;

	m_selectedUnit->onBeginMove();

	m_turnState = TurnState::UnitAction_Move;
	m_unitMoveIndex = 0;
	m_unitMoveDelta = 0.0f;
	
}

void e2::Game::beginCustomEntityAction()
{
	if (m_turnState == TurnState::Unlocked)
		m_turnState = TurnState::EntityAction_Generic;
}


void e2::Game::endCustomEntityAction()
{
	if (m_turnState == TurnState::EntityAction_Generic)
		m_turnState = TurnState::Unlocked;
}

void e2::Game::updateEntityAction()
{
	e2::GameEntity* ent = selectedEntity();
	ent->updateEntityAction(m_timeDelta);
}

void e2::Game::beginEntityTargeting()
{
	if (m_turnState == TurnState::Unlocked)
		m_turnState = TurnState::EntityAction_Target;
}

void e2::Game::endEntityTargeting()
{
	if (m_turnState == TurnState::EntityAction_Target)
		m_turnState = TurnState::Unlocked;
}



void e2::Game::updateMainCamera(double seconds)
{
	constexpr float moveSpeed = 5.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	m_viewZoom -= float(mouse.scrollOffset) * 0.1f;
	m_viewZoom = glm::clamp(m_viewZoom, 0.0f, 1.0f);

	

	glm::vec2 oldOrigin = m_viewOrigin;

	// move main camera 
	if (kb.keys[int16_t(e2::Key::Left)].state)
	{
		m_viewOrigin.x -= moveSpeed * float(seconds);
	}
	if (kb.keys[int16_t(e2::Key::Right)].state)
	{
		m_viewOrigin.x += moveSpeed * float(seconds);
	}

	if (kb.keys[int16_t(e2::Key::Up)].state)
	{
		m_viewOrigin.y -= moveSpeed * float(seconds);
	}
	if (kb.keys[int16_t(e2::Key::Down)].state)
	{
		m_viewOrigin.y += moveSpeed * float(seconds);
	}

	if (kb.pressed(e2::Key::K))
	{
		m_viewOrigin = {};
	}

	if (leftMouse.pressed && !m_uiHovered)
	{
		m_dragView = calculateRenderView(m_viewDragOrigin);
		// save where in world we pressed
		m_cursorDragOrigin = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc);
		m_viewDragOrigin = m_viewOrigin;
		m_viewDragging = true;
	}


	const float dragMultiplier = glm::mix(0.01f, 0.025f, m_viewZoom);
	if (leftMouse.state)
	{
		if (m_viewDragging)
		{
			glm::vec2 newDrag = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc);
			glm::vec2 dragOffset = newDrag - m_cursorDragOrigin;

			m_viewOrigin = m_viewDragOrigin - dragOffset;
		}
	}
	else
	{
		m_viewDragging = false;
	}

	e2::Aabb2D viewBounds = m_hexGrid->viewBounds();
	m_viewOrigin.x = glm::clamp(m_viewOrigin.x, viewBounds.min.x, viewBounds.max.x);
	m_viewOrigin.y = glm::clamp(m_viewOrigin.y, viewBounds.min.y, viewBounds.max.y);

	m_viewVelocity = m_viewOrigin - oldOrigin;

	m_view = calculateRenderView(m_viewOrigin);
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view);
	if (!m_altView)
	{
		renderer->setView(m_view);
	}
}


e2::RenderView e2::Game::calculateRenderView(glm::vec2 const &viewOrigin)
{
	float viewFov = glm::mix(55.0f, 35.0f, m_viewZoom);
	float viewAngle = glm::mix(35.5f, 50.0f, m_viewZoom);
	float viewDistance = glm::mix(2.5f, 30.0f, m_viewZoom);

	glm::quat orientation = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(viewAngle), { 1.0f, 0.0f, 0.0f });

	e2::RenderView newView{};
	newView.fov = viewFov;
	newView.clipPlane = { 0.1f, 100.0f };
	newView.origin = glm::vec3(viewOrigin.x, 0.0f, viewOrigin.y) + (orientation * glm::vec3(e2::worldForward()) * -viewDistance);
	newView.orientation = orientation;

	return newView;
}



void e2::Game::destroyUnit(e2::Hex const& location)
{
	auto finder = m_unitIndex.find(location.offsetCoords());
	if (finder == m_unitIndex.end())
		return;

	e2::GameUnit* unit = finder->second;

	if(unit->isLocal())
		unit->rollbackVisibility();

	if (m_selectedUnit && m_selectedUnit == unit)
		deselectUnit();

	m_unitIndex.erase(unit->tileIndex);
	m_units.erase(unit);

	if (m_empires[unit->empireId])
	{
		m_empires[unit->empireId]->units.erase(unit);

		m_empires[unit->empireId]->resources.fiscalStreams.erase(unit);
	}

	e2::destroy(unit);
}


namespace
{
	std::vector<e2::Hex> tmpHex;
}

void e2::Game::queueDestroyUnit(e2::GameUnit* unit)
{
	m_unitsPendingDestroy.insert(unit);
}

e2::GameEntity* e2::Game::selectedEntity()
{
	if (m_selectedUnit)
		return m_selectedUnit;

	if (m_selectedStructure)
		return m_selectedStructure;

	return nullptr;
}

void e2::Game::harvestWood(e2::Hex const& location, EmpireId empire)
{
	e2::TileData* tileData = m_hexGrid->getTileData(location.offsetCoords());
	if (!tileData)
		return;

	bool hasForest = (tileData->flags & e2::TileFlags::FeatureForest) != TileFlags::FeatureNone;
	float woodProfit = tileData->getWoodAbundanceAsFloat() * 14.0f;

	if (!hasForest)
		return;

	m_empires[empire]->resources.funds.wood += woodProfit;
	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::FeatureForest)));

	if (tileData->forestProxy)
		e2::destroy(tileData->forestProxy);

	tileData->forestProxy = nullptr;
	tileData->forestMesh = nullptr;

}

void e2::Game::removeWood(e2::Hex const& location)
{
	e2::TileData* tileData = m_hexGrid->getTileData(location.offsetCoords());
	if (!tileData)
		return;

	bool hasForest = (tileData->flags & e2::TileFlags::FeatureForest) != TileFlags::FeatureNone;
	if (!hasForest)
		return;

	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::FeatureForest)));

	if (tileData->forestProxy)
		e2::destroy(tileData->forestProxy);

	tileData->forestProxy = nullptr;
	tileData->forestMesh = nullptr;
}

void e2::Game::selectStructure(e2::GameStructure* structure)
{
	if (!structure || structure == m_selectedStructure)
		return;

	deselect();

	m_selectedStructure = structure;

	m_hexGrid->clearOutline();

	tmpHex.clear();
	e2::Hex::circle(e2::Hex(m_selectedStructure->tileIndex), m_selectedStructure->sightRange, ::tmpHex);

	for (e2::Hex h : ::tmpHex)
	{
		m_hexGrid->pushOutline(h.offsetCoords());
	}
}

void e2::Game::deselectStructure()
{
	if (!m_selectedStructure)
		return;

	m_selectedStructure = nullptr;
	m_hexGrid->clearOutline();

}

void e2::Game::postSpawnStructure(e2::GameStructure* structure)
{
	structure->initialize();

	m_structures.insert(structure);
	m_structureIndex[structure->tileIndex] = structure;

	if (m_empires[structure->empireId])
		m_empires[structure->empireId]->structures.insert(structure);

	m_empires[structure->empireId]->resources.fiscalStreams.insert(structure);

	removeWood(e2::Hex(structure->tileIndex));

	e2::TileData* existingData = m_hexGrid->getTileData(structure->tileIndex);
	if (existingData)
	{
		existingData->empireId = structure->empireId;
	}
}

void e2::Game::destroyStructure(e2::Hex const& location)
{
	auto finder = m_structureIndex.find(location.offsetCoords());
	if (finder == m_structureIndex.end())
		return;

	e2::GameStructure* structure = finder->second;

	if (structure->isLocal())
		structure->rollbackVisibility();

	if (m_selectedStructure && m_selectedStructure == structure)
		deselectStructure();

	m_structureIndex.erase(structure->tileIndex);
	m_structures.erase(structure);

	if (m_empires[structure->empireId])
	{
		m_empires[structure->empireId]->structures.erase(structure);
		m_empires[structure->empireId]->resources.fiscalStreams.erase(structure);
	}

	e2::destroy(structure);
}



e2::GameUnit* e2::Game::unitAtHex(glm::ivec2 const& hex)
{
	auto finder = m_unitIndex.find(hex);
	if (finder == m_unitIndex.end())
		return nullptr;

	return finder->second;
}

e2::GameStructure* e2::Game::structureAtHex(glm::ivec2 const& hex)
{
	auto finder = m_structureIndex.find(hex);
	if (finder == m_structureIndex.end())
		return nullptr;

	return finder->second;
}

e2::MeshPtr e2::Game::getEntityMesh(e2::EntityType type)
{
	return m_entityMeshes[uint8_t(type)];
}

e2::SkeletonPtr e2::Game::getEntitySkeleton(e2::EntityType  type)
{
	return m_entitySkeletons[uint8_t(type)];
}


void e2::Game::discoverEmpire(EmpireId empireId)
{
	// @todo optimize
	m_discoveredEmpires.insert(m_empires[empireId]);
	m_undiscoveredEmpires.erase(m_empires[empireId]);
}

void e2::Game::updateAltCamera(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();


	if (m_altView)
	{

		if (mouse.moved)
		{
			m_altViewYaw += mouse.moveDelta.x * viewSpeed;
			m_altViewPitch -= mouse.moveDelta.y * viewSpeed;
			m_altViewPitch = glm::clamp(m_altViewPitch, -89.f, 89.f);
		}

		glm::quat altRotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewYaw), glm::vec3(e2::worldUp()));
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewPitch), glm::vec3(e2::worldRight()));

		if (kb.keys[int16_t(e2::Key::Space)].state)
		{
			m_altViewOrigin -= glm::vec3(e2::worldUp()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::LeftControl)].state)
		{
			m_altViewOrigin -= -glm::vec3(e2::worldUp()) * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::A)].state)
		{
			m_altViewOrigin -= altRotation * -glm::vec3(e2::worldRight()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::D)].state)
		{
			m_altViewOrigin -= altRotation * glm::vec3(e2::worldRight()) * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::W)].state)
		{
			m_altViewOrigin -= altRotation * glm::vec3(e2::worldForward()) * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::S)].state)
		{
			m_altViewOrigin -= altRotation * -glm::vec3(e2::worldForward()) * moveSpeed * float(seconds);
		}

		altRotation = glm::rotate(altRotation, glm::radians(180.0f), glm::vec3(e2::worldUp()));

		e2::RenderView newView{};
		newView.fov = 65.0;
		newView.clipPlane = { 0.1, 100.0 };
		newView.origin = m_altViewOrigin;
		newView.orientation = altRotation;
		renderer->setView(newView);
	}

}


void e2::Game::updateAnimation(double seconds)
{
	// 1 / 30 = 33.333ms = 30 fps 
	constexpr double targetFrameTime = 1.0 / 60.0;
	m_accumulatedAnimationTime += seconds;

	int32_t numTicks = 0;
	while (m_accumulatedAnimationTime > targetFrameTime)
	{
		numTicks++;
		m_accumulatedAnimationTime -= targetFrameTime;
	}

	if (numTicks < 1)
		return;

	e2::Aabb2D viewAabb = m_viewPoints.toAabb();

	for (e2::GameUnit* unit : m_units)
	{
		// update inView, first check aabb to give chance for implicit early exit
		glm::vec2 unitCoords = unit->visualPlanarCoords();
		unit->inView = viewAabb.isWithin(unitCoords) && m_viewPoints.isWithin(unitCoords, 1.0f);

		// @todo consider using this instead if things break
		//for(int32_t i = 0; i < numTicks; i++)
		//	unit->updateAnimation(targetFrameTime);

		unit->updateAnimation(targetFrameTime * double(numTicks));
	}


}

e2::PathFindingAccelerationStructure::PathFindingAccelerationStructure(e2::GameUnit* unit)
{
	if (!unit)
		return;

	e2::Game* game = unit->game();
	e2::HexGrid* grid = game->hexGrid();

	e2::Hex originHex = e2::Hex(unit->tileIndex);
	origin = e2::create<e2::PathFindingHex>(originHex);
	origin->isBegin = true;

	hexIndex[unit->tileIndex] = origin;


	std::queue<e2::PathFindingHex*> queue;
	std::unordered_set<e2::Hex> processed;

	queue.push(origin);

	while (!queue.empty())
	{
		e2::PathFindingHex* curr  = queue.front();
		queue.pop();

		for (e2::Hex n : curr->index.neighbours())
		{
			if (curr->stepsFromOrigin + 1 > unit->movePointsLeft) 
				continue;

			if (processed.contains(n))
				continue;

			glm::ivec2 coords = n.offsetCoords();

			if (hexIndex.contains(coords))
				continue;

			// ignore hexes not directly visible
			if (!grid->isVisible(coords))
				continue;

			// ignore hexes that are occupied by unpassable biome
			e2::TileData tile = unit->game()->hexGrid()->calculateTileDataForHex(n);
			if (!tile.isPassable(unit->passableFlags))
				continue;

			// ignore hexes that are occupied by units
			e2::GameUnit* otherUnit = game->unitAtHex(coords);
			if (otherUnit)
				continue;

			e2::PathFindingHex* newHex = e2::create<e2::PathFindingHex>(n);
			newHex->towardsOrigin = curr;
			newHex->stepsFromOrigin = curr->stepsFromOrigin + 1;
			hexIndex[coords] = newHex;
			

			queue.push(newHex);
		}

		processed.insert(curr->index);
	}
}

e2::PathFindingAccelerationStructure::PathFindingAccelerationStructure(e2::GameContext* ctx, e2::Hex const& start, uint64_t range, bool ignoreVisibility, e2::PassableFlags passableFlags)
{
	if (!ctx)
		return;

	e2::Game* game = ctx->game();
	e2::HexGrid* grid = game->hexGrid();
	glm::ivec2 tileIndex = start.offsetCoords();

	e2::Hex originHex = start;
	origin = e2::create<e2::PathFindingHex>(originHex);
	origin->isBegin = true;

	hexIndex[tileIndex] = origin;


	std::queue<e2::PathFindingHex*> queue;
	std::unordered_set<e2::Hex> processed;

	queue.push(origin);

	while (!queue.empty())
	{
		e2::PathFindingHex* curr = queue.front();
		queue.pop();

		for (e2::Hex n : curr->index.neighbours())
		{
			if (curr->stepsFromOrigin + 1 > range)
				continue;

			if (processed.contains(n))
				continue;

			glm::ivec2 coords = n.offsetCoords();

			if (hexIndex.contains(coords))
				continue;

			// ignore hexes not directly visible
			if (!ignoreVisibility && !grid->isVisible(coords))
				continue;

			// ignore hexes that are occupied by unpassable biome
			e2::TileData tile = ctx->game()->hexGrid()->calculateTileDataForHex(n);
			if (!tile.isPassable(passableFlags))
				continue;

			// ignore hexes that are occupied by units
			e2::GameUnit* otherUnit = game->unitAtHex(coords);
			if (otherUnit)
				continue;

			e2::PathFindingHex* newHex = e2::create<e2::PathFindingHex>(n);
			newHex->towardsOrigin = curr;
			newHex->stepsFromOrigin = curr->stepsFromOrigin + 1;
			hexIndex[coords] = newHex;


			queue.push(newHex);
		}

		processed.insert(curr->index);
	}
}

e2::PathFindingAccelerationStructure::~PathFindingAccelerationStructure()
{
	for (auto& [coords, as] : hexIndex)
		e2::destroy(as);
}

e2::PathFindingAccelerationStructure::PathFindingAccelerationStructure()
{

}

std::vector<e2::Hex> e2::PathFindingAccelerationStructure::find(e2::Hex const& target)
{

	auto targetFinder = hexIndex.find(target.offsetCoords());
	if (targetFinder == hexIndex.end())
		return {};


	std::vector<e2::Hex> returner;
	e2::PathFindingHex* cursor = targetFinder->second;
	while (cursor)
	{
		returner.push_back(cursor->index);
		if (cursor->isBegin)
			break;
		cursor = cursor->towardsOrigin;
	}

	std::reverse(returner.begin(), returner.end());

	return returner;

}

e2::PathFindingHex::PathFindingHex(e2::Hex const& _index)
	: index(_index)
{

}

e2::PathFindingHex::~PathFindingHex()
{

}

std::string e2::SaveMeta::displayName()
{
	if (!exists)
		return std::format("{}: empty", uint32_t(slot+1));

	char timeString[std::size("hh:mm:ss yyyy-mm-dd")];

	std::strftime(std::data(timeString), std::size(timeString), "%T %F", (std::gmtime)(&timestamp));

	return std::format("{}: {}", uint32_t(slot+1), std::string(timeString));

}

std::string e2::SaveMeta::fileName()
{
	PWSTR saveGameFolder{};
	if (SHGetKnownFolderPath(FOLDERID_SavedGames, KF_FLAG_DEFAULT, nullptr, &saveGameFolder) != S_OK)
	{
		LogError("SHGetKnownFolderPath failed, returning backup folder");
		return std::filesystem::path(std::format("./saves/{}.sav", uint32_t(slot))).string();
	}

	std::wstring wstr = saveGameFolder;
	CoTaskMemFree(saveGameFolder);

	std::string str;
	std::transform(wstr.begin(), wstr.end(), std::back_inserter(str), [](wchar_t c) {
		return (char)c;
	});

	return std::filesystem::path(std::format("{}/reveal_and_annihilate/{}.sav",str, uint32_t(slot))).string();
}
