
#include "game/game.hpp"

#include "game/entities/player.hpp"
#include "game/entities/itementity.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/audiomanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"
#include "e2/transform.hpp"
#include "e2/buffer.hpp"

#include "game/components/physicscomponent.hpp"

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

#include <random>
#include <algorithm>

namespace
{
	std::vector<e2::Hex> tmpHex;
}


e2::Game::Game(e2::Context* ctx)
	: e2::Application(ctx)
	, m_radionManager(this)
	, m_playerState(this)
{
	
	m_hitLabels.resize(e2::maxNumHitLabels);
	for (uint64_t i = 0; i < maxNumHitLabels; i++)
	{
		m_hitLabels[i] = HitLabel();
	}

	readAllSaveMetas();

}

e2::Game::~Game()
{
	
}

void e2::Game::saveGame(uint8_t slot)
{
	//if (m_empireTurn != m_localEmpireId)
	//{
	//	LogError("refuse to save game on AI turn");
	//	return;
	//}

	e2::SaveMeta newMeta;
	newMeta.exists = true;
	newMeta.timestamp = std::time(nullptr);
	newMeta.slot = slot;

	{
		e2::FileStream buf(newMeta.fileName(), e2::FileMode(uint8_t(e2::FileMode::ReadWrite) | uint8_t(e2::FileMode::Truncate)));

		buf << int64_t(newMeta.timestamp);

		m_hexGrid->saveToBuffer(buf);

		buf << m_startViewOrigin;
		buf << m_targetViewOrigin;

		uint64_t numEntities = 0;
		for (e2::Entity* ent : m_entities)
		{
			if (ent->transient)
				continue;
			numEntities++;
		}

		std::vector<e2::Entity*> savedEntities;
		savedEntities.reserve(numEntities);
		buf << numEntities;
		for (e2::Entity* entity : m_entities)
		{
			if (entity->transient)
				continue;

			e2::EntitySpecification* spec = entity->getSpecification();

			// structure id
			buf << entity->uniqueId;
			buf << spec->id;

			buf << entity->getTransform()->getTranslation(e2::TransformSpace::World);
			buf << entity->getTransform()->getRotation(e2::TransformSpace::World);

			savedEntities.push_back(entity);

		}
		for (e2::Entity* entity : savedEntities)
		{
			if (entity->transient)
				continue;

			entity->writeForSave(buf);
		}

		m_playerState.writeForSave(buf);
		m_radionManager.writeForSave(buf);

		saveSlots[slot] = newMeta;
	}
	readAllSaveMetas();
}

e2::SaveMeta e2::Game::readSaveMeta(uint8_t slot)
{
	e2::SaveMeta meta;
	meta.slot = slot;
	meta.cachedFileName = meta.fileName();

	// header is just int64_t timestamp
	constexpr uint64_t headerSize = 8 ;

	e2::FileStream buf(meta.cachedFileName, e2::FileMode::ReadOnly);
	if (!buf.valid())
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

	nukeGame();
	setupGame();
	m_menuMusic.stop();

	e2::FileStream buf(saveSlots[slot].fileName(), e2::FileMode::ReadOnly);
	if (!buf.valid())
	{
		LogError("save slot missing or corrupted");
		return;
	}

	buf.read(sizeof(int64_t));

	m_hexGrid->loadFromBuffer(buf);


	m_hexGrid->clearAllChunks();

	buf >> m_startViewOrigin;
	buf >> m_targetViewOrigin;
	m_viewOrigin = m_targetViewOrigin;

	std::vector<e2::Entity*> loadedEntities;
	uint64_t numEntities{};
	buf >> numEntities;

	loadedEntities.reserve(numEntities);
	for (uint64_t i = 0; i < numEntities; i++)
	{
		uint64_t uniqueId{};
		buf >> uniqueId;

		e2::Name entityId;
		buf >> entityId;

		auto finder = m_entitySpecifications.find(entityId);
		if (finder == m_entitySpecifications.end())
		{
			LogError("invalid entity in savefile: {}", entityId);
			continue;
		}
		e2::EntitySpecification* spec = finder->second;
		glm::vec3 worldPosition;
		buf >> worldPosition;

		glm::quat worldRotation;
		buf >> worldRotation;

		e2::Entity* newEntity = spawnEntity(entityId, worldPosition, worldRotation, true);
		newEntity->uniqueId = uniqueId;
		m_entityMap[newEntity->uniqueId] = newEntity;

		if (e2::PlayerEntity* player = newEntity->cast<e2::PlayerEntity>())
		{
			m_playerState.entity = player;
		}

		loadedEntities.push_back(newEntity);

	}

	for (e2::Entity* entity : loadedEntities)
	{
		entity->readForSave(buf);
	}

	m_playerState.readForSave(buf);
	m_radionManager.readForSave(buf);

	startGame();
}

void e2::Game::setupGame()
{
	readAllSaveMetas();

	m_hexGrid = e2::create<e2::HexGrid>(this);


	e2::SoundPtr menuMusic= assetManager()->get("M_Menu.e2a").cast<e2::Sound>();
	audioManager()->playMusic(menuMusic, 0.6f, true, &m_menuMusic);

	m_startViewOrigin = glm::vec2(-1662.75f, 650.27f);
	m_viewOrigin = m_startViewOrigin;
	m_targetViewOrigin = m_viewOrigin;
	m_viewZoom = 0.3f;
	m_targetViewZoom = m_viewZoom;
	m_hexGrid->initializeWorldBounds(m_viewOrigin);
	

	for (auto& [name, spec] : m_entitySpecifications)
	{
		e2::ItemSpecification* itemSpec = spec->cast<e2::ItemSpecification>();
		if (itemSpec)
		{
			if (itemSpec->wieldHandler)
			{
				itemSpec->wieldHandler->onSetup();
			}
		}
	}
}

void e2::Game::nukeGame()
{
	e2::ITexture* outlineTextures[2] = {nullptr, nullptr};
	m_session->renderer()->setOutlineTextures(outlineTextures);
	m_menuMusic.stop();
	m_waterChannel.setVolume(0.0f);


	for (auto &[name, spec] : m_entitySpecifications)
	{
		e2::ItemSpecification* itemSpec = spec->cast<e2::ItemSpecification>();
		if (itemSpec)
		{
			if (itemSpec->wieldHandler)
			{
				itemSpec->wieldHandler->onNuke();
			}
		}
	}

	m_entitiesPendingDestroy.clear();
	
	auto entitiesClone = m_entities;
	for (e2::Entity* entity: entitiesClone)
		destroyEntity(entity);

	m_playerState.entity = nullptr;

	if (m_hexGrid)
	{
		asyncManager()->waitForPredicate([this]()->bool {
			return m_hexGrid->numJobsInFlight() == 0;
		});
		e2::destroy(m_hexGrid);
		m_hexGrid = nullptr;
	}

	m_radionManager.clearDiscovered();

	m_globalState = GlobalState::Menu;
	m_inGameMenuState = InGameMenuState::Main;
	//m_empireTurn = 0;
	m_turnState = TurnState::Unlocked;
	m_timeDelta = 0.0;
}

void e2::Game::exitToMenu()
{
	nukeGame();
	setupGame();
}

void e2::Game::finalizeBoot()
{
	finalizeSpecifications();


	auto am = assetManager();

	m_irradianceMap = am->get("T_Courtyard_Irradiance.e2a").cast<e2::Texture2D>();
	m_radianceMap = am->get("T_Courtyard_Radiance.e2a").cast<e2::Texture2D>();
	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());

	m_uiIconsSheet = am->get("S_UI_Icons.e2a").cast<e2::Spritesheet>();
	m_uiIconsSheet2 = am->get("S_UI_Icons2.e2a").cast<e2::Spritesheet>();
	m_uiUnitsSheet = am->get("S_UI_Units.e2a").cast<e2::Spritesheet>();

	uiManager()->registerGlobalSpritesheet("gameUi", m_uiIconsSheet);
	uiManager()->registerGlobalSpritesheet("icons", m_uiIconsSheet2);
	uiManager()->registerGlobalSpritesheet("units", m_uiUnitsSheet);


	// After this line, we may no longer safely fetch and store loaded assets
	am->returnALJ(m_bootTicket);

	
	audioManager()->playMusic(am->get("M_Ambient_Ocean.e2a").cast<e2::Sound>(), 0.0f, true, &m_waterChannel);

	setupGame();

#if defined(E2_PROFILER)
	profiler()->start();
#endif
}

void e2::Game::initialize()
{
	m_session = e2::create<e2::GameSession>(this);

	m_menuMusic = audioManager()->createChannel();
	m_waterChannel = audioManager()->createChannel();


	e2::ALJDescription alj;


	initializeSpecifications(alj);

	m_bootBegin = e2::timeNow();

	auto am = assetManager();

	



	e2::HexGrid::prescribeAssets(this, alj);


	am->prescribeALJ(alj, "T_Courtyard_Irradiance.e2a");
	am->prescribeALJ(alj, "T_Courtyard_Radiance.e2a");

	am->prescribeALJ(alj, "S_UI_Icons.e2a");
	am->prescribeALJ(alj, "S_UI_Icons2.e2a");
	am->prescribeALJ(alj, "S_UI_Units.e2a");

	am->prescribeALJ(alj, "S_Item_Drop.e2a");
	am->prescribeALJ(alj, "S_Item_Pickup.e2a");

	am->prescribeALJ(alj, "M_Menu.e2a");
	am->prescribeALJ(alj, "M_Ambient_Ocean.e2a");

	initializeScriptGraphs();

	m_bootTicket = am->queueALJ(alj);
	
	m_session->window()->showCursor(false);
}

void e2::Game::shutdown()
{
	nukeGame();

	e2::destroy(m_session);
	uiManager()->unregisterGlobalSpritesheet("gameUi");
	uiManager()->unregisterGlobalSpritesheet("icons");
	uiManager()->unregisterGlobalSpritesheet("units");


	m_irradianceMap = nullptr;
	m_radianceMap = nullptr;
	m_uiIconsSheet = nullptr;
	m_uiIconsSheet2 = nullptr;
	m_uiUnitsSheet = nullptr;

	for (auto& [id, graph] : m_scriptGraphs)
	{
		e2::destroy(graph);
	}
	m_scriptGraphs.clear();
}

void e2::Game::update(double seconds)
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();

	glm::quat sunRot = glm::identity<glm::quat>();

	sunRot = glm::rotate(sunRot, glm::radians(m_sunAngleA), e2::worldUpf());
	sunRot = glm::rotate(sunRot, glm::radians(m_sunAngleB), e2::worldRightf());

	renderer->setSun(sunRot, { 1.0f, 0.9f, 0.85f }, m_sunStrength);
	renderer->setIbl(m_iblStrength);
	renderer->whitepoint(glm::vec3(m_whitepoint));
	renderer->exposure(m_exposure);

	if (m_playerState.entity)
	{
		renderer->setPlayerPosition(m_playerState.entity->planarCoords());
	}

	if (m_globalState == GlobalState::Boot)
	{
		m_session->tick(seconds);

		glm::vec2 winSize = m_session->window()->size();
		float textWidth = m_session->uiContext()->calculateSDFTextWidth(FontFace::Serif, 22.0f, "Loading... ");
		m_session->uiContext()->drawSDFTextCarousel(FontFace::Serif, 22.f, 0xFFFFFFFF, { winSize.x - textWidth - 16.f, winSize.y - 32.0f }, "Loading... ", 8.0f, (float)m_bootBegin.durationSince().seconds() * 4.0f);

#if defined(E2_SHIPPING)
		bool bootPredicate = assetManager()->queryALJ(m_bootTicket).status == ALJStatus::Completed && m_bootBegin.durationSince().seconds() > 4.0f;
#else 
		bool bootPredicate = assetManager()->queryALJ(m_bootTicket).status == ALJStatus::Completed;
#endif

		if (bootPredicate)
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

	m_timeDelta = seconds;
	if (m_globalState == GlobalState::Menu)
		updateMenu(seconds);
	else if (m_globalState == GlobalState::Game)
		updateGame(seconds);
	else if (m_globalState == GlobalState::InGameMenu)
		updateInGameMenu(seconds);

	auto view = renderer->view();

	audioManager()->setListenerTransform(glm::inverse(view.calculateViewMatrix()));

	std::unordered_set<e2::Entity*> ecpy = m_entitiesPendingDestroy;
	for (e2::Entity* entity : ecpy)
	{
		if (entity->canBeDestroyed())
		{
			m_entitiesPendingDestroy.erase(entity);
			destroyEntity(entity);
		}
		
	}

	if(m_globalState != GlobalState::Boot)
		drawCrosshair();
	
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
	m_hexGrid->renderFogOfWar(); // actually renders minimap, cutmask for grass etc too!
	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

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
	m_cursorPlane = renderer->view().unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
	e2::Hex newHex = e2::Hex(m_cursorPlane);
	m_hexChanged = newHex != m_prevCursorHex;
	m_prevCursorHex = m_cursorHex;
	m_cursorHex = newHex.offsetCoords();

	m_cursorTile = m_hexGrid->getExistingTileData(m_cursorHex);

	m_proximityHexes.clear();
	e2::Hex::circle(e2::Hex(m_viewOrigin), 8, m_proximityHexes);
	int32_t divisor = 0;
	float proximity = 0.0f;
	for (e2::Hex hex : m_proximityHexes)
	{
		if (hexGrid()->getTileData(hex.offsetCoords()).isLand())
			continue;

		glm::vec2 hexCenter = hex.planarCoords();
		float thisDistance = glm::distance(hexCenter, m_viewOrigin);
		float distanceCoeff = glm::clamp(thisDistance / 8.0f, 0.0f, 1.0f);
		float proximityCoeff = 1.0f - distanceCoeff;
		proximity += proximityCoeff;
		divisor++;
	}
	if (divisor > 0)
	{
		m_waterProximity = proximity / float(divisor);
		m_waterChannel.setVolume(m_waterProximity * 0.35f);
	}

	constexpr float invisibleChunkLifetime = 3.0f;

	// debug buttons 
	if (kb.keys[int16_t(e2::Key::P)].pressed)
	{
		m_altView = !m_altView;
		gameSession()->window()->mouseLock(m_altView);
		if (m_altView)
		{
			m_altViewOrigin =glm::vec3(m_viewOrigin.x, -5.0f, m_viewOrigin.y);
		}

		m_hexGrid->debugDraw(m_altView);
			

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
	

	//m_cursorHex

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);


	if (m_currentGraph)
	{
		m_scriptExecutionContext->update(seconds);
		if (m_scriptExecutionContext->isDone())
		{
			e2::destroy(m_scriptExecutionContext);
			m_scriptExecutionContext = nullptr;
			m_currentGraph = nullptr;
		}
	}


	//e2::ITexture *tex = hexGrid()->grassCutMask().getTexture(renderManager()->frameIndex());

	//ui->drawTexturedQuad({ 48.0, 48.0 }, {256, 256}, 0xFFFFFFFF, tex);

	if(!kb.state(Key::C))
		drawUI();

	if (kb.pressed(Key::L))
	{
		runScriptGraph("test");
	}
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

#if !defined(E2_SHIPPING)
	//if (timer < 20.0)
	//	timer = 20.0;
#endif

	//m_viewZoom = 0.5f;


	m_view = calculateRenderView(m_viewOrigin);
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view, 0.0);
	renderer->setView(m_view);


	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();
	e2::ITexture* outlineTextures[2] = { m_hexGrid->outlineTexture(0), m_hexGrid->outlineTexture(1) };
	m_session->renderer()->setOutlineTextures(outlineTextures);

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
	m_menuMusic.setVolume(globalMenuFade);

	// tinted block
	double blockTimer1 = glm::smoothstep(12.0, 15.0, double(timer));
	double blockAlpha = 0.9f - blockTimer1 * 0.9f;
	ui->drawQuad({}, resolution, e2::UIColor(136, 131, 144, uint8_t(blockAlpha * 255.0)));

	// black screen
	double blockAlpha2 = 1.0 - glm::smoothstep(8.0, 10.0, double(timer));
	ui->drawQuad({}, resolution, e2::UIColor(14, 14, 15, uint8_t(blockAlpha2 * 255.0)));


	double authorWidth = ui->calculateSDFTextWidth(FontFace::Monospace, 36.0f, "prklinteractive");
	double authorAlpha = (double)glm::smoothstep(3.0, 4.0, timer);

	double authorFadeOut = double(1.0 - glm::smoothstep(6.0, 7.0, timer));
	authorAlpha *= authorFadeOut;

	e2::UIColor authorColor(136, 131, 144, uint8_t(authorAlpha * 255.0));
	ui->drawSDFText(FontFace::Monospace, 36.0, authorColor, glm::vec2(resolution.x / 2.0 - authorWidth / 2.0, resolution.y / 2.0), "prklinteractive");

	double width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0f, "Legend of Nifelheim");

	double menuHeight = 280.0;
	double menuOffset = resolution.y / 2.0 - menuHeight / 2.0;

	double cursorY = menuOffset;
	double middleX = resolution.x / 2.0;
	double xOffset = middleX - width / 2.0;


	double blockAlpha3 = (double)glm::smoothstep(15.0, 15.25, timer);

	e2::UIColor textColor(14, 14, 15, uint8_t(blockAlpha3 * 255.0 * globalMenuFade));
	e2::UIColor textColor2(14, 14, 15, uint8_t(blockTimer1 * 170.0 * globalMenuFade));
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor, glm::vec2(xOffset, cursorY), "Legend of Nifelheim");

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



	//ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset1, "Reveal & Annihilate");
	//ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset2, "Reveal & Annihilate");

	cursorY += 64.0;

	double blockAlphaNewGame = glm::smoothstep(15.0, 15.25, timer);
	e2::UIColor textColorNewGame(14, 14, 15, uint8_t(blockAlphaNewGame * 170.0 * globalMenuFade));
	double newGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "New Game");
	bool newGameHovered = !m_haveBegunStart && timer > 15.25 && mouse.relativePosition.x > middleX - newGameWidth / 2.0 && mouse.relativePosition.x < middleX + newGameWidth / 2.0
		&& mouse.relativePosition.y > cursorY + 20.0 && mouse.relativePosition.y < cursorY + 60.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, newGameHovered ? 0x000000FF : textColorNewGame, glm::vec2(middleX - newGameWidth / 2.0, cursorY + 40.0), "New Game");
	if (!m_haveBegunStart && newGameHovered && leftMouse.clicked)
	{
		beginStartGame();
	}

	double blockAlphaLoadGame = glm::smoothstep(15.25, 15.5, timer);
	e2::UIColor textColorLoadGame(14, 14, 15, uint8_t(blockAlphaLoadGame * 170.0 * globalMenuFade));
	double loadGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Load Game");
	bool loadGameHovered = !m_haveBegunStart && timer > 15.5 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + loadGameWidth
		&& mouse.relativePosition.y > cursorY + 60.0 && mouse.relativePosition.y < cursorY + 100.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, loadGameHovered ? 0x000000FF : textColorLoadGame, glm::vec2(xOffset, cursorY + 40.0 * 2), "Load Game");
	if (!m_haveBegunStart && loadGameHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Load;
	}



	double blockAlphaOptions = glm::smoothstep(15.5, 15.75, timer);
	e2::UIColor textColorOptions(14, 14, 15, uint8_t(blockAlphaOptions * 170.0 * globalMenuFade));
	double optionsWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Options");
	bool optionsHovered = !m_haveBegunStart && timer > 15.75 && mouse.relativePosition.x > xOffset && mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 100.0 && mouse.relativePosition.y < cursorY + 140.0;
	ui->drawSDFText(FontFace::Serif, 24.0f, optionsHovered ? 0x000000FF : textColorOptions, glm::vec2(xOffset, cursorY + 40.0 * 3), "Options");
	if (!m_haveBegunStart && optionsHovered && leftMouse.clicked)
	{
		m_mainMenuState = MainMenuState::Options;
	}


	double blockAlphaQuit = glm::smoothstep(15.75, 16.0, timer);
	e2::UIColor textColorQuit(14, 14, 15, uint8_t(blockAlphaQuit * 170.0 * globalMenuFade));
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

		e2::Entity *player = spawnEntity("player", e2::Hex(m_startLocation).localCoords());
		m_playerState.entity = player->cast<e2::PlayerEntity>();
		m_playerState.give("ironhatchet");
		m_playerState.give("ironsword");
		m_playerState.give("radion_ionizer");
		m_playerState.give("radion_linker");
		m_radionManager.discoverEntity("radion_powersource");
		m_radionManager.discoverEntity("radion_wirepost");
		m_radionManager.discoverEntity("radion_splitter");
		m_radionManager.discoverEntity("radion_capacitor");
		m_radionManager.discoverEntity("radion_switch");
		m_radionManager.discoverEntity("radion_crystal");
		m_radionManager.discoverEntity("radion_gateAND");
		m_radionManager.discoverEntity("radion_gateNAND");
		m_radionManager.discoverEntity("radion_gateNOR");
		m_radionManager.discoverEntity("radion_gateNOT");
		m_radionManager.discoverEntity("radion_gateOR");
		m_radionManager.discoverEntity("radion_gateXNOR");
		m_radionManager.discoverEntity("radion_gateXOR");

		for (uint32_t i = 0; i < 400; i++)
		{
			m_playerState.give("radion_cube");
		}

		//spawnCity(m_localEmpireId, m_startLocation);

		//spawnEntity("mobile_mob", m_startLocation, m_localEmpireId);

	}
	if (m_haveStreamedStart)
	{
		double sec = m_beginStreamTime.durationSince().seconds();
		double a = glm::smoothstep(0.0, 2.0, sec);
		m_viewOrigin = glm::mix(m_startViewOrigin, e2::Hex(m_startLocation).planarCoords(), glm::exponentialEaseInOut(a));
		m_targetViewOrigin = m_viewOrigin;
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
	do {} while (!findStartLocation(e2::randomIvec2(glm::ivec2(-1024), glm::ivec2(1024)), { 1024, 1024 }, m_startLocation));
	pauseWorldStreaming();
	forceStreamLocation(e2::Hex(m_startLocation).planarCoords());
}

bool e2::Game::findStartLocation(glm::ivec2 const& offset, glm::ivec2 const& rangeSize, glm::ivec2 &outLocation)
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

		e2::TileData startTile = m_hexGrid->calculateTileData(startLocation);
		if (!startTile.isPassable(e2::PassableFlags::Land))
			continue;

		constexpr bool ignoreVisibility = true;
		auto as = e2::create<e2::PathFindingAS>(this, e2::Hex(startLocation), 64, ignoreVisibility, e2::PassableFlags::Land, false, nullptr);
		uint64_t numWalkableHexes = as->hexIndex.size();
		e2::TileFlags testFlags = e2::TileFlags::None;
		uint64_t numGreen = 0;
		bool foundNonForested = false;
		for (auto& [index, tile] : as->hexIndex)
		{
			m_hexGrid->calculateBiome(tile->index.planarCoords(), testFlags);
			if ((testFlags & e2::TileFlags::BiomeMask) == e2::TileFlags::BiomeGrassland)
			{
				numGreen++;
			}

			if (!foundNonForested)
			{
				float baseHeight = m_hexGrid->calculateBaseHeight(tile->index.planarCoords());
				m_hexGrid->calculateFeaturesAndWater(tile->index.planarCoords(), baseHeight, testFlags);
				if ((testFlags & e2::TileFlags::FeatureMask) != e2::TileFlags::FeatureForest)
				{
					startLocation = tile->index.offsetCoords();
					foundNonForested = true;
				}
			}
		}
		e2::destroy(as);

		if (numWalkableHexes < 64 || numGreen < 18 || !foundNonForested)
			continue;

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
	m_turnState = TurnState::Realtime;
}

void e2::Game::addScreenShake(float intensity)
{
	m_shakeIntensity = glm::max(intensity, m_shakeIntensity);
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

	if (m_turnState == TurnState::Unlocked)
		updateTurn();
	else if (m_turnState == TurnState::Realtime)
		updateRealtime();
		
}

void e2::Game::updateTurn()
{
	// if local is making turn
	//if (m_empireTurn == m_localEmpireId)
		updateTurnLocal();
	//else
		//updateTurnAI();
}

void e2::Game::updateRealtime()
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	auto& leftMouse = mouse.buttons[uint8_t(e2::MouseButton::Left)];
	auto& rightMouse = mouse.buttons[uint8_t(e2::MouseButton::Right)];

	if (kb.pressed(e2::Key::Tab) && !scriptRunning())
	{
		m_targetViewZoom = 1.0f;
		m_turnState = e2::TurnState::Unlocked;
		//renderer->setDrawGrid(m_showGrid = true);
		return;
	}


	std::unordered_set<e2::Entity*> ents = m_entities;
	for (e2::Entity* entity : ents)
	{
		entity->update(m_timeDelta);
	}

	m_playerState.update(m_timeDelta);

	if (m_playerState.entity)
	{
		m_targetViewOrigin = m_playerState.entity->planarCoords();
		m_hexGrid->updateCutMask(m_playerState.entity->planarCoords());
	}

	if (!scriptRunning())
	{
		m_radionManager.update(m_timeDelta);
	}
}

void e2::Game::updateTurnLocal()
{
	if (scriptRunning())
	{
		return;
	}

	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();

	auto& leftMouse = mouse.buttons[uint8_t(e2::MouseButton::Left)];
	auto& rightMouse = mouse.buttons[uint8_t(e2::MouseButton::Right)];

	
	if (kb.pressed(e2::Key::Tab) && m_turnState == e2::TurnState::Unlocked)
	{
		m_targetViewZoom = 0.0f;
		m_turnState = e2::TurnState::Realtime;
		//renderer->setDrawGrid(m_showGrid = false);
		return;
	}

	// turn logic here
	if (!m_uiHovered && leftMouse.clicked && leftMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftControl))
		{
			// ugly line of code, no I wont fix it 
			bool onLand = m_cursorTile ? m_cursorTile->getWater() == TileFlags::WaterNone : m_hexGrid->calculateTileData(m_cursorHex).getWater() == TileFlags::WaterNone;
			
			if (m_playerState.entity)
			{
				e2::Transform* playerTransform = m_playerState.entity->getTransform();
				glm::vec3 oldPosition = playerTransform->getTranslation(e2::TransformSpace::World);
				glm::vec3 newPosition{ m_cursorPlane.x, oldPosition.y, m_cursorPlane.y };

				playerTransform->setTranslation(newPosition, e2::TransformSpace::World);
				m_playerState.entity->getMesh()->applyTransform();
			}

			return;
		}

	}


}

e2::Entity* e2::Game::spawnEntity(e2::Name entityId, glm::vec3 const& worldPosition, glm::quat const& worldRotation, bool forLoadGame)
{
	auto finder = m_entitySpecifications.find(entityId);
	if (finder == m_entitySpecifications.end())
	{
		LogError("no entity with id {}, returning null", entityId.cstring());
		return nullptr;
	}
	e2::EntitySpecification* spec = finder->second;


	e2::Object* newEntity = spec->entityType->create();
	if (!newEntity)
	{
		LogError("failed to create instance of type {} for entity with id {}", spec->entityType->fqn.cstring(), entityId.cstring());
		return nullptr;
	}

	e2::Entity* asEntity= newEntity->cast<e2::Entity>();
	if (!asEntity)
	{
		e2::destroy(newEntity);
		LogError("entity specification couldnt spawn entity instance, make sure entity type inherits e2::Entity and isnt pure virtual/abstract");
		return nullptr;
	}
	
	if (!forLoadGame)
	{
		asEntity->uniqueId = m_entityIdGiver++;
		m_entityMap[asEntity->uniqueId] = asEntity;
	}
	

	// post construct since we created it dynamically
	asEntity->postConstruct(this, spec, worldPosition, worldRotation);

	// insert into global entity set 
	m_entities.insert(asEntity);

	return asEntity;
}

e2::Entity* e2::Game::spawnCustomEntity(e2::Name specificationId, e2::Name entityTypeName, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	auto finder = m_entitySpecifications.find(specificationId);
	if (finder == m_entitySpecifications.end())
	{
		LogError("no entity with id {}, returning null", specificationId.cstring());
		return nullptr;
	}
	e2::EntitySpecification* spec = finder->second;

	e2::Type* entityType = e2::Type::fromName(entityTypeName);
	if (!entityType)
	{
		LogError("invalid entity typename");
		return nullptr;
	}

	e2::Object* newEntity = entityType->create();
	if (!newEntity)
	{
		LogError("failed to create instance of type {} for entity with id {}", entityType->fqn.cstring(), specificationId.cstring());
		return nullptr;
	}

	e2::Entity* asEntity = newEntity->cast<e2::Entity>();
	if (!asEntity)
	{
		e2::destroy(newEntity);
		LogError("entity specification couldnt spawn entity instance, make sure entity type inherits e2::Entity and isnt pure virtual/abstract");
		return nullptr;
	}

	asEntity->uniqueId = m_entityIdGiver++;
	asEntity->transient = true;

	// post construct since we created it dynamically
	asEntity->postConstruct(this, spec, worldPosition, worldRotation);
	// insert into global entity set 
	m_entities.insert(asEntity);

	return asEntity;
}

void e2::Game::drawUI()
{
	m_uiHovered = false;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();

	e2::UIContext* ui = gameSession()->uiContext();
	auto& mouse = ui->mouseState();

	if (!m_altView)
	{
		//drawResourceIcons();

		drawHitLabels();


		//drawStatusUI();

		//drawUnitUI();

		drawMinimapUI();

		if (m_playerState.entity)
		{
			m_playerState.renderInventory(game()->timeDelta());
		}

		//drawFinalUI();

		constexpr bool debugShadows = true;
		if(debugShadows)
		{
			glm::vec2 offset{ 300.0f, 40.0f };
			glm::vec2 size{ 120, 220.0f };


			bool hovered = !m_viewDragging &&
				(mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + size.x &&
					mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + size.y);
			if (hovered)
			{
				m_uiHovered = true;
			}



			ui->pushFixedPanel("envParams", offset, size);
			ui->beginStackV("envParamStack");

			ui->sliderFloat("sunAngleA", m_sunAngleA, -180.0f, 180.0f);
			ui->sliderFloat("sunAngleB", m_sunAngleB, 0.0f, 90.0f);
			ui->sliderFloat("sunStr", m_sunStrength, 0.0f, 10.0f);
			ui->sliderFloat("iblStr", m_iblStrength, 0.0f, 10.0f); 

			ui->sliderFloat("exposure", m_exposure, 0.0f, 20.0f);
			ui->sliderFloat("whitepoint", m_whitepoint, 0.0f, 20.0f);

			//ui->sliderFloat("mtnFreqScale", e2::mtnFreqScale, 0.001f, 0.1f, "%.6f");
			//ui->sliderFloat("mtnScale", e2::mtnScale, 0.25f, 10.0f, "%.6f");

			//ui->sliderFloat("treeScale", e2::treeScale, 0.1f, 2.0f);
			//ui->sliderFloat("treeSpread", e2::treeSpread, 0.1f, 2.0f);
			//ui->sliderInt("treeNum1", e2::treeNum1, 1, 50);
			//ui->sliderInt("treeNum2", e2::treeNum2, 1, 50);
			//ui->sliderInt("treeNum3", e2::treeNum3, 1, 50);

			if (ui->sliderFloat("fog", m_fog, 0.0f, 2.0f))
			{
				m_hexGrid->setFog(m_fog);
			}



			ui->endStackV();
			ui->popFixedPanel();
		}


		
	}

	drawDebugUI();

}

void e2::Game::drawCrosshair()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();

	e2::UIContext* ui = gameSession()->uiContext();

	float cursorOffset = 3.0f;
	float cursorSize = 6.0f;

	glm::vec2 mouse = ui->mouseState().relativePosition;
	ui->drawQuad(mouse + glm::vec2(cursorOffset, -1.0f), glm::vec2(cursorSize, 2.0f), e2::UIColor(255, 255, 255, 255));
	ui->drawQuad(mouse + glm::vec2(-cursorOffset - cursorSize, -1.0f), glm::vec2(cursorSize, 2.0f), e2::UIColor(255, 255, 255, 255));

	ui->drawQuad(mouse + glm::vec2(-1.0f, cursorOffset), glm::vec2(2.0f, cursorSize), e2::UIColor(255, 255, 255, 255));
	ui->drawQuad(mouse + glm::vec2(-1.0f, -cursorOffset - cursorSize), glm::vec2(2.0f, cursorSize), e2::UIColor(255, 255, 255, 255));
}

void e2::Game::drawResourceIcons()
{


}

void e2::Game::drawHitLabels()
{
	e2::GameSession* session = gameSession();
	e2::UIContext* ui = session->uiContext();
	for (HitLabel& label : m_hitLabels)
	{
		if (!label.active)
			continue;

		if (label.timeCreated.durationSince().seconds() > 1.0f)
		{
			label.active = false;
			continue;
		}

		label.velocity.y += 20.0f * (float)game()->timeDelta();

		label.offset += label.velocity * (float)game()->timeDelta();

		float fontSize = 14.0f + glm::smoothstep(0.0f, 1.0f, (float)label.timeCreated.durationSince().seconds()) * 10.0f;

		float alpha = (1.0f - glm::smoothstep(0.4f, 1.0f, (float)label.timeCreated.durationSince().seconds())) * 255.f;

		ui->drawSDFText(e2::FontFace::Sans, fontSize, e2::UIColor(255, 255, 255, (uint8_t)alpha), worldToPixels(label.offset), label.text);

	}
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

	glm::vec2 offset = { winSize.x - width - 16.0f , 48.0f };

	bool hovered = !m_viewDragging && 
		(mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height);
	if(hovered)
	{
		m_uiHovered = true;
	}


	//ui->drawQuadShadow(offset- glm::vec2(4.0f, 4.0f), {width + 8.0f, height + 8.0f}, 8.0f, 0.9f, 4.0f);
	ui->drawTexturedQuad(offset, { width, height }, 0xFFFFFFFF, m_hexGrid->minimapTexture(renderManager()->frameIndex()));
	ui->drawFrame(offset, { width, height }, e2::gamePanelDarkHalf, 1.0f);
	if (hovered && leftMouse.state)
	{
		e2::Aabb2D viewBounds = m_hexGrid->viewBounds();
		glm::vec2 normalizedMouse = (mouse.relativePosition - offset) / glm::vec2(width, height);
		glm::vec2 worldMouse = viewBounds.min + normalizedMouse * (viewBounds.max - viewBounds.min);
		m_viewOrigin = worldMouse;
		m_targetViewOrigin = m_viewOrigin;
	}

	{
		glm::vec2 pos = offset + glm::vec2(0.0f, height + 4.0f);
		glm::vec2 size = { width, 64 };

		ui->pushFixedPanel("zxccc", pos, size);
		
		ui->beginStackV("stttsV");
		
		if (ui->checkbox("toggleGrid", m_showGrid, "Show Grid"))
		{
			renderer->setDrawGrid(m_showGrid);
		}

		ui->checkbox("togglePhysics", m_showPhysics, "Show Physics");

		ui->endStackV();

		ui->popFixedPanel();
	}

	return;

	glm::vec2 pos = offset + glm::vec2(0.0f, height + 32.0f);
	glm::vec2 size = { width, 80 };

	bool btnHovered = (mouse.relativePosition.x > pos.x && mouse.relativePosition.x < pos.x + size.x &&
		mouse.relativePosition.y > pos.y && mouse.relativePosition.y < pos.y + size.y);
	ui->drawGamePanel(pos, size, btnHovered, 0.75f);
	if(btnHovered)
		ui->drawFrame(pos, size, e2::gameAccentHalf, 1.0f);
	else 
		ui->drawFrame(pos, size, e2::gamePanelDarkHalf, 1.0f);

	e2::Sprite* droneSprite = getUnitSprite("drone");
	ui->drawSprite(pos + glm::vec2{8.f, 8.f}, * droneSprite, 0xFFFFFFFF, 64.0f / 200.0f);

	ui->drawRasterText(e2::FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f, 20.f), "Train Drone", false);

	ui->drawRasterText(e2::FontFace::Sans, 10, 0xFFFFFF7F, pos + glm::vec2(80.0f, 36.f), "Builds districts and turrets.", false);

	e2::Sprite* goldSprite = getIconSprite("gold");
	ui->drawSprite(pos + glm::vec2(80.0f, 40.0f + 10.0f), *goldSprite, 0xFFFFFFFF, 20.0f / goldSprite->size.y);
	float w = ui->calculateTextWidth(FontFace::Sans, 14, "20");
	ui->drawRasterText(FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f + 20.0f + 10.0f, 40.0f + 20.0f), "20");

	e2::Sprite* steelSprite = getIconSprite("beam");
	ui->drawSprite(pos + glm::vec2(80.0f + 20.0f + 10.0f + w + 20.0f , 40.0f + 10.0f), *steelSprite, 0xFFFFFFFF, 20.0f / steelSprite->size.y);
	float w2 = ui->calculateTextWidth(FontFace::Sans, 14, "5");
	ui->drawRasterText(FontFace::Sans, 14, 0xFFFFFFFF, pos + glm::vec2(80.0f + 20.0f + 10.0f + w + 20.0f + 20.0f + 10.0f, 40.0f + 20.0f), "5");

	
}

void e2::Game::drawDebugUI()
{
	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { 12, 12 + (18.0f * 14.0f) }, std::format("^3View Origin: {}", m_viewOrigin));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { 12, 12 + (18.0f * 15.0f) }, std::format("^3Zoom: {}", m_targetViewZoom));

	
	if (m_showPhysics)
	{
		static std::vector<e2::Collision> tmp;
		tmp.reserve(64);

		for (e2::ChunkState* chunk : hexGrid()->m_chunksInView)
		{
			glm::ivec2 chunkIndex = chunk->chunkIndex;

			glm::ivec2 chunkTileOffset = chunkIndex * glm::ivec2(e2::hexChunkResolution);

			for (int32_t y = 0; y < e2::hexChunkResolution; y++)
			{
				for (int32_t x = 0; x < e2::hexChunkResolution; x++)
				{
					glm::ivec2 worldIndex = chunkTileOffset + glm::ivec2(x, y);
					tmp.clear();

					populateCollisions(worldIndex, e2::CollisionType::Component | e2::CollisionType::Tree, tmp, false);

					for (e2::Collision& c : tmp)
					{
						if (c.type == CollisionType::Tree)
						{
							session->renderer()->debugCircle({ 1.0f, 1.0f, 0.0f }, c.position, c.radius);
						}
						else if (c.type == CollisionType::Component)
						{
							session->renderer()->debugCircle({ 1.0f, 0.0f, 1.0f }, c.position, c.radius);
						}
					}
				}
			}
		}
	}
	



#if defined(E2_PROFILER)


	e2::EngineMetrics& metrics = engine()->metrics();

	float yOffset = 64.0f;
	float xOffset = 12.0f;
	ui->drawQuadShadow({ 0.0f, yOffset - 16.0f }, { 320.0f, 200.0f }, 8.0f, 0.9f, 4.0f);
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset }, std::format("^2Avg. {:.1f} ms, fps: {:.1f}", metrics.frameTimeMsMean, 1000.0f / metrics.frameTimeMsMean));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 1.0f) }, std::format("^3High {:.1f} ms, fps: {:.1f}", metrics.frameTimeMsHigh, 1000.0f / metrics.frameTimeMsHigh));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 2.0f) }, std::format("^4CPU FPS: {:.1f}", metrics.realCpuFps));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 3.0f) }, std::format("^5Num. chunks: {}", m_hexGrid->numChunks()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 4.0f) }, std::format("^6Visible chunks: {}", m_hexGrid->numVisibleChunks()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 5.0f) }, std::format("^7Entities: {}", m_entities.size()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 6.0f) }, std::format("^8High loadtime: {:.2f}ms", m_hexGrid->highLoadTime()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 7.0f) }, std::format("^9Jobs in flight: {}", m_hexGrid->numJobsInFlight()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 8.0f) }, std::format("^2View Origin: {}", m_viewOrigin));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 9.0f) }, std::format("^3View Velocity: {}", m_viewVelocity));



	//ui->drawTexturedQuad({ xOffset, yOffset + 18.0f * 11.0f }, { 384.f, 384.f }, 0xFFFFFFFF, renderer->shadowTarget());
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

	glm::vec2 btnSize(80.0f, 80.0f);
	float width = 368.0f;
	float height = 144.0f;
	float topA = 32.0f;
	float topB = 24.0f;
	float topHeight = 32.0f + 24.0f;
	float topBottomDelimiter = height - btnSize.y - topHeight;
	// 736 x 160

	glm::vec2 offset = { winSize.x - width - 16.0f, winSize.y - height - 16.0f };
	glm::vec2 cursor = offset;

	if (mouse.relativePosition.x > offset.x && mouse.relativePosition.x < offset.x + width &&
		mouse.relativePosition.y > offset.y && mouse.relativePosition.y < offset.y + height)
		m_uiHovered = true;

	e2::Sprite* citySprite = getUnitSprite("city");

	ui->drawQuad(cursor, { width, topA }, e2::gamePanelLight.withOpacity(0.75f));
	ui->drawQuad(cursor + glm::vec2(0.0f, topA), { width, topB }, e2::gamePanelDark.withOpacity(0.75f));

	float spriteScale = topA / citySprite->size.y;
	ui->drawSprite(cursor + glm::vec2(4.0f, 0.0f), *citySprite, 0xFFFFFFFF, spriteScale);
	ui->drawRasterText(e2::FontFace::Sans, 14, 0xFFFFFFFF, cursor + glm::vec2(4.0f, 0.0f) + glm::vec2(citySprite->size.x * spriteScale, topA / 2.0f), "City of **Mikkeli**");

	ui->drawRasterText(e2::FontFace::Sans, 10, 0xFFFFFFFF, cursor + glm::vec2(4.0f, topA + topB / 2.0f), "2 districts, researching **Diligence** (13 turns)");

	cursor.y += topHeight + topBottomDelimiter;

	glm::vec2 iconSize(44.f, 44.f);
	float iconOffsetY = 6.0f;
	float buttonPadding = 16.0f;
	float infoOpacity = 0.75f;


	auto drawBtn = [&](std::string const& textLabel, e2::Name spriteName) {
		bool hovered = (mouse.relativePosition.x > cursor.x && mouse.relativePosition.x < cursor.x + btnSize.x &&
			mouse.relativePosition.y > cursor.y && mouse.relativePosition.y < cursor.y + btnSize.y);


		e2::Sprite* sprite = getIconSprite(spriteName);
		ui->drawGamePanel(cursor, btnSize, hovered, infoOpacity);
		ui->drawFrame(cursor, btnSize, e2::UIColor(hovered ? e2::gameAccentHalf : e2::gamePanelDark).withOpacity(infoOpacity), 1.0f);
		ui->drawSprite(cursor + glm::vec2(btnSize.x / 2.0f - iconSize.x / 2.0f, iconOffsetY), *sprite, e2::UIColor(0xFFFFFFFF).withOpacity(infoOpacity), iconSize.y / sprite->size.y);
		float tw = ui->calculateTextWidth(FontFace::Sans, 14, textLabel);
		ui->drawRasterText(FontFace::Sans, 14, e2::UIColor(0xFFFFFFFF).withOpacity(infoOpacity), cursor + glm::vec2(btnSize.x / 2.0f - tw / 2.0f, 64.0f), textLabel);
		cursor.x += btnSize.x + buttonPadding;
	};

	drawBtn("*Details*", "info");
	drawBtn("*Research*", "research");
	drawBtn("*Upgrades*", "upgrade");
	drawBtn("*Economy*", "cash");


	/*
	ui->drawQuadShadow(offset, { width, height }, 8.0f, 0.9f, 4.0f);

	ui->pushFixedPanel("test", offset + glm::vec2(4.0f, 4.0f), glm::vec2(width - 8.0f, height - 8.0f));
	ui->beginStackH("test2", 40.0f);

		if (ui->gameGridButton("nextent", "gameUi.nextunit", "Next unit..", m_localTurnEntities.size() > 0))
			nextLocalEntity();
		
		if (ui->gameGridButton("endturn", "gameUi.endturn", "End turn", true))
			endTurn();

	ui->endStackH();
	ui->popFixedPanel();*/
}

void e2::Game::onNewCursorHex()
{

}

void e2::Game::spawnHitLabel(glm::vec3 const& worldLocation, std::string const& text)
{
	m_hitLabels[m_hitLabelIndex++] = e2::HitLabel(worldLocation, text);
	if (m_hitLabelIndex >= e2::maxNumHitLabels)
		m_hitLabelIndex = 0;
}


void e2::Game::updateMainCamera(double seconds)
{
	if (m_shakeIntensity > 0.0f)
		m_shakeIntensity -= (float)seconds;
	else
		m_shakeIntensity = 0.0f;

	constexpr float moveSpeed = 25.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	if (m_turnState != e2::TurnState::Realtime)
	{
		m_targetViewZoom -= float(mouse.scrollOffset) * 0.1f;
		m_targetViewZoom = glm::clamp(m_targetViewZoom, 0.3f, 1.0f);
	}
	m_targetViewZoom = glm::clamp(m_targetViewZoom, 0.0f, 1.0f);
	m_viewZoom = glm::mix(m_viewZoom, m_targetViewZoom, glm::clamp(float(seconds) * 16.0f, 0.0f, 1.0f));
	m_viewOrigin = glm::mix(m_viewOrigin, m_targetViewOrigin, glm::clamp(float(seconds) * 16.0f, 0.0f, 1.0f));


	glm::vec2 oldOrigin = m_viewOrigin;

	if (m_turnState != e2::TurnState::Realtime)
	{
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
			m_cursorDragOrigin = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
			m_viewDragOrigin = m_viewOrigin;
			m_viewDragging = true;
		}


		const float dragMultiplier = glm::mix(0.01f, 0.025f, m_viewZoom);
		if (leftMouse.state)
		{
			if (m_viewDragging)
			{
				glm::vec2 newDrag = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc, 0.0);
				glm::vec2 dragOffset = newDrag - m_cursorDragOrigin;

				m_viewOrigin = m_viewDragOrigin - dragOffset;
			}
		}
		else
		{
			m_viewDragging = false;
		}

		m_targetViewOrigin = m_viewOrigin;
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
	m_viewPoints = e2::Viewpoints2D(renderer->resolution(), m_view, 0.0);
	if (!m_altView)
	{
		e2::RenderView shakyView = m_view;
		shakyView.origin += e2::randomInUnitSphere() * m_shakeIntensity * 0.5f;
		glm::dquat shakyRight = glm::angleAxis(e2::randomFloat(-1.f, 1.f) * m_shakeIntensity * glm::radians(4.0f), e2::worldRightf());
		glm::dquat shakyUp = glm::angleAxis(e2::randomFloat(-1.f, 1.f) * m_shakeIntensity * glm::radians(4.0f), e2::worldUpf());
		shakyView.orientation = shakyRight * shakyUp * shakyView.orientation;
		renderer->setView(shakyView);
	}
}


e2::RenderView e2::Game::calculateRenderView(glm::vec2 const &viewOrigin)
{
	float viewFov = glm::mix(25.0f, 35.0f, m_viewZoom);
	float viewAngle =  glm::mix(40.0f, 55.0f, m_viewZoom);
	float viewDistance = glm::mix(10.0f, 60.0f, m_viewZoom);

	glm::quat orientation = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(viewAngle), { 1.0f, 0.0f, 0.0f });

	e2::RenderView newView{};
	newView.fov = viewFov;
	newView.clipPlane = { 0.1f, 100.0f };
	newView.origin = glm::vec3(viewOrigin.x, 0.0f, viewOrigin.y) + (orientation * glm::vec3(e2::worldForward()) * -viewDistance);
	newView.orientation = orientation;

	return newView;
}



void e2::Game::destroyEntity(e2::Entity* entity)
{
	m_entityMap.erase(entity->uniqueId);
	m_entities.erase(entity);
	e2::destroy(entity);
}

void e2::Game::queueDestroyEntity(e2::Entity* entity)
{
	entity->pendingDestroy = true;
	m_entitiesPendingDestroy.insert(entity);
}

e2::Entity* e2::Game::entityFromId(uint64_t id)
{
	auto finder = m_entityMap.find(id);
	if (finder != m_entityMap.end())
	{
		return finder->second;
	}

	LogWarning("no such entity");
	return nullptr;
}




e2::Sprite* e2::Game::getUiSprite(e2::Name name)
{
	return m_uiIconsSheet->getSprite(name);
}

e2::Sprite* e2::Game::getIconSprite(e2::Name name)
{
	return m_uiIconsSheet2->getSprite(name);
}

e2::Sprite* e2::Game::getUnitSprite(e2::Name name)
{
	return m_uiUnitsSheet->getSprite(name);
}

void e2::Game::initializeSpecifications(e2::ALJDescription &alj)
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("data/entities/"))
	{
		if (!entry.is_regular_file())
			continue;
		if (entry.path().extension() != ".json")
			continue;

		std::ifstream unitFile(entry.path());
		nlohmann::json entity;
		try
		{
			entity = nlohmann::json::parse(unitFile);
		}
		catch (nlohmann::json::parse_error& e)
		{
			LogError("failed to load specification, json parse error: {}", e.what());
			continue;
		}

		if (!entity.contains("id"))
		{
			LogError("entity file {} lacks id", entry.path().string());
			continue;
		}

		if (!entity.contains("specification"))
		{
			LogError("entity file {} lacks specification type", entry.path().string());
			continue;
		}
		e2::Name id = entity.at("id").template get<std::string>();
		e2::Name specification = entity.at("specification").template get<std::string>();
		e2::Type* specificationType = e2::Type::fromName(specification);
		if (!specificationType || !specificationType->inherits("e2::EntitySpecification"))
		{
			LogError("entity file {} provided invalid specification type {} (make sure it exists, and inherits e2::EntitySpecification)", entry.path().string(), specification);
			continue;
		}

		e2::Object* newObject = specificationType->create();
		if (!newObject)
		{
			LogError("failed to instance specification {}, make sure its not pure virtual", id);
			continue;
		}

		e2::EntitySpecification* newSpecification = newObject->cast<e2::EntitySpecification>();
		if (!newSpecification)
		{
			e2::destroy(newObject);
			LogError("specification instance for {} doesnt inherit entity specification", id);
			continue;
		}
		newSpecification->id = id;
		newSpecification->game = this;
		newSpecification->populate(this, entity);

		for (e2::Name dep : newSpecification->assets)
		{
			assetManager()->prescribeALJ(alj, dep);
		}

		m_entitySpecifications[id] = newSpecification;
	}



	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("data/items/"))
	{
		if (!entry.is_regular_file())
			continue;
		if (entry.path().extension() != ".json")
			continue;

		std::ifstream unitFile(entry.path());
		nlohmann::json item;
		try
		{
			item = nlohmann::json::parse(unitFile);
		}
		catch (nlohmann::json::parse_error& e)
		{
			LogError("failed to load specification, json parse error: {}", e.what());
			continue;
		}

		if (!item.contains("id"))
		{
			LogError("item file {} lacks id", entry.path().string());
			continue;
		}


		e2::ItemSpecification* newItemSpecification = e2::create<e2::ItemSpecification>();
		newItemSpecification->id = item.at("id").template get<std::string>();
		newItemSpecification->game = this;
		newItemSpecification->populate(this, item);

		for (e2::Name dep : newItemSpecification->assets)
		{
			assetManager()->prescribeALJ(alj, dep);
		}

		m_entitySpecifications[newItemSpecification->id] = newItemSpecification;
		m_itemSpecifications[newItemSpecification->id] = newItemSpecification;
	}

}

void e2::Game::finalizeSpecifications()
{
	for (auto [name, spec] : m_entitySpecifications)
	{
		spec->finalize();
	}
}

void e2::Game::registerCollisionComponent(e2::CollisionComponent* component, glm::ivec2 const& index)
{
	m_collisionComponents[index].insert(component);
}

e2::ItemSpecification* e2::Game::getItemSpecification(e2::Name name)
{
	auto finder = m_itemSpecifications.find(name);
	if (finder == m_itemSpecifications.end())
		return nullptr;

	return finder->second;
}

void e2::Game::initializeScriptGraphs()
{

	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("data/scripts/"))
	{
		if (!entry.is_regular_file())
			continue;
		if (entry.path().extension() != ".json")
			continue;

		e2::ScriptGraph* newGraph = e2::create<e2::ScriptGraph>(this, entry.path().string());
		if (newGraph->valid())
		{
			LogError("Loaded script graph {} from {}", newGraph->id(), entry.path().string());
			m_scriptGraphs[newGraph->id()] = newGraph;
		}
		else
		{
			LogError("Invalid script graph: {}", entry.path().string());
			e2::destroy(newGraph);
		}
	}
}

void e2::Game::runScriptGraph(e2::Name id)
{
	if (m_currentGraph)
	{
		LogError("Can't run script graph, one is already running!");
		return;
	}

	auto finder = m_scriptGraphs.find(id);
	if (finder == m_scriptGraphs.end())
	{
		LogError("No such graph: {}", id);
		return;
	}

	m_currentGraph = finder->second;
	m_scriptExecutionContext = e2::create<e2::ScriptExecutionContext>(m_currentGraph);
}

bool e2::Game::scriptRunning()
{
	return m_currentGraph != nullptr;
}

e2::RadionManager* e2::Game::radionManager()
{
	return &m_radionManager;
}

e2::EntitySpecification* e2::Game::getEntitySpecification(e2::Name name)
{
	auto finder = m_entitySpecifications.find(name);
	if (finder == m_entitySpecifications.end())
		return nullptr;

	return finder->second;
}


void e2::Game::populateCollisions(glm::ivec2 const& coordinate, CollisionType mask, std::vector<e2::Collision>& outCollisions, bool includeNeighbours)
{
	e2::Hex hex(coordinate);
	e2::TileData tileData = hexGrid()->getTileData(coordinate);

	auto addTrees = [&](e2::Hex& tc, e2::TileData& td) {
		if (td.isForested() && td.forestIndex >= 0)
		{
			uint32_t i = 0;
			e2::ForestState* forestState = hexGrid()->getForestState(td.forestIndex);

			for (e2::TreeState& treeState : forestState->trees)
			{
				glm::vec2 treeOffset = tc.planarCoords() + treeState.localOffset(&td, forestState);

				Collision newCollision;
				newCollision.type = e2::CollisionType::Tree;
				newCollision.position = treeOffset;
				newCollision.radius = 0.25f * treeState.scale;
				newCollision.hex = tc.offsetCoords();
				newCollision.treeIndex = i;

				outCollisions.push_back(newCollision);
				i++;
			}
		}
		else
		{
			e2::UnpackedForestState* forestState = hexGrid()->getUnpackedForestState(tc.offsetCoords());
			if (!forestState)
				return;

			uint32_t i = 0;
			for (e2::UnpackedTreeState& treeState : forestState->trees)
			{
				if (treeState.health <= 0.0f)
				{
					i++;
					continue;
				}

				Collision newCollision;
				newCollision.type = e2::CollisionType::Tree;
				newCollision.position = treeState.worldOffset;
				newCollision.radius = 0.25f * treeState.scale;
				newCollision.hex = tc.offsetCoords();
				newCollision.treeIndex = i;

				outCollisions.push_back(newCollision);
				i++;
			}
		}
	};

	auto addComponents = [&](e2::Hex& tc) {
		auto finder = m_collisionComponents.find(tc.offsetCoords());
		if (finder != m_collisionComponents.end())
		{
			for (e2::CollisionComponent* comp : finder->second)
			{
				e2::Collision newCollision;
				newCollision.type = CollisionType::Component;
				newCollision.component = comp;
				newCollision.radius = comp->radius();
				newCollision.hex = tc.offsetCoords();
				newCollision.position = comp->entity()->planarCoords();
				outCollisions.push_back(newCollision);
			}
		}
	};

	if((mask & e2::CollisionType::Tree) == e2::CollisionType::Tree)
		addTrees(hex, tileData);

	if ((mask & e2::CollisionType::Component) == e2::CollisionType::Component)
		addComponents(hex);


	e2::Collision newCollision;
	newCollision.hex = coordinate;
	newCollision.position = hex.planarCoords();
	newCollision.radius = 1.0f;

	if (tileData.isMountain() && (mask & e2::CollisionType::Mountain) == e2::CollisionType::Mountain)
	{
		newCollision.type = e2::CollisionType::Mountain;
		outCollisions.push_back(newCollision);
	}
	else if ((tileData.isShallowWater() || tileData.isDeepWater()) && (mask & e2::CollisionType::Water) == e2::CollisionType::Water)
	{
		newCollision.type = e2::CollisionType::Water;
		outCollisions.push_back(newCollision);
	}
	else if (tileData.isLand() && (mask & e2::CollisionType::Land) == e2::CollisionType::Land)
	{
		newCollision.type = e2::CollisionType::Land;
		outCollisions.push_back(newCollision);
	}

	if (!includeNeighbours)
		return;

	e2::StackVector<e2::Hex, 6> neighbours = hex.neighbours();
	for (e2::Hex& n : neighbours)
	{
		e2::TileData nd = hexGrid()->getTileData(n.offsetCoords());

		e2::Collision newCollision;
		newCollision.hex = n.offsetCoords();
		newCollision.position = n.planarCoords();
		newCollision.radius = 1.0f;

		if (nd.isMountain() && (mask & e2::CollisionType::Mountain) == e2::CollisionType::Mountain)
		{
			newCollision.type = e2::CollisionType::Mountain;
			outCollisions.push_back(newCollision);
		}
		else if ((nd.isShallowWater() || nd.isDeepWater()) && (mask & e2::CollisionType::Water) == e2::CollisionType::Water)
		{
			newCollision.type = e2::CollisionType::Water;
			outCollisions.push_back(newCollision);
		}
		else if (nd.isLand() && (mask & e2::CollisionType::Land) == e2::CollisionType::Land)
		{
			newCollision.type = e2::CollisionType::Land;
			outCollisions.push_back(newCollision);
		}

		if ((mask & e2::CollisionType::Tree) == e2::CollisionType::Tree)
			addTrees(n, nd);

		if ((mask & e2::CollisionType::Component) == e2::CollisionType::Component)
			addComponents(n);
	}

}

void e2::Game::unregisterCollisionComponent(e2::CollisionComponent* component, glm::ivec2 const &index)
{
	m_collisionComponents[index].erase(component);
}



void e2::Game::removeWood(glm::ivec2 const& location)
{
	hexGrid()->removeWood(location);
}

void e2::Game::removeResource(glm::ivec2 const& location)
{
	e2::TileData* tileData = m_hexGrid->getExistingTileData(location);
	if (!tileData)
		return;

	bool hasResource = (tileData->flags & e2::TileFlags::ResourceMask) != TileFlags::ResourceNone;
	if (!hasResource)
		return;

	tileData->flags = e2::TileFlags(uint16_t(tileData->flags) & (~uint16_t(e2::TileFlags::ResourceMask)));

	if (tileData->resourceProxy)
		e2::destroy(tileData->resourceProxy);

	tileData->resourceProxy = nullptr;
	tileData->resourceMesh = nullptr;
}
//
//void e2::Game::discoverEmpire(EmpireId empireId)
//{
//	// @todo optimize
//	m_discoveredEmpires.insert(m_empires[empireId]);
//	m_undiscoveredEmpires.erase(m_empires[empireId]);
//}

void e2::Game::updateAltCamera(double seconds)
{
	constexpr float moveSpeed = 2.5f;
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
		newView.clipPlane = { 0.1, 1000.0 };
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

	m_entitiesInView.clear();

	std::unordered_set<e2::Entity*> ents = m_entities;
	for (e2::Entity* entity : ents)
	{
		// update inView, first check aabb to give chance for implicit early exit
		glm::vec2 unitCoords = entity->planarCoords();

		entity->setInView(/*viewAabb.isWithin(unitCoords) &&*/ m_viewPoints.isWithin(unitCoords, 1.0f));

		if (entity->isInView())
		{
			m_entitiesInView.insert(entity);
		}

		// @todo consider using this instead if things break
		//for(int32_t i = 0; i < numTicks; i++)
		//	unit->updateAnimation(targetFrameTime);

		if(m_turnState == e2::TurnState::Realtime)
			entity->updateAnimation(targetFrameTime * double(numTicks));
	}

}

//
//e2::PathFindingAS::PathFindingAS(e2::GameEntity* entity, glm::ivec2 const& target)
//{
//	if (!entity)
//		return;
//
//	if (!entity->isLocal())
//	{
//		LogError("passed entity wasn't local; this constructor is only meant for local players!");
//		return;
//	}
//
//	if (e2::Hex::distance(entity->tileIndex, target) > e2::maxAutoMoveRange)
//	{
//		LogError("max range overflow");
//		return;
//	}
//
//	e2::Game* game = entity->game();
//	e2::HexGrid* grid = game->hexGrid();
//
//	e2::Hex originHex = e2::Hex(entity->tileIndex);
//	origin = e2::create<e2::PathFindingHex>(originHex);
//	origin->isBegin = true;
//
//	hexIndex[entity->tileIndex] = origin;
//
//
//	std::queue<e2::PathFindingHex*> queue;
//	std::unordered_set<e2::Hex> processed;
//
//	queue.push(origin);
//
//	while (!queue.empty())
//	{
//		e2::PathFindingHex* currHexEntry = queue.front();
//		queue.pop();
//
//		for (e2::Hex nextHex : currHexEntry->index.neighbours())
//		{
//			
//			if (processed.contains(nextHex))
//				continue;
//
//			glm::ivec2 coords = nextHex.offsetCoords();
//
//			if (hexIndex.contains(coords))
//				continue;
//
//			if (currHexEntry->stepsFromOrigin + 1 > e2::maxAutoMoveRange)
//				continue;
//
//			// check movepoints 
//			if (currHexEntry->stepsFromOrigin + 1 > entity->movePointsLeft)
//			{
//				// we are out of move bounds, do cheeky version 
//				e2::TileData* existingTile = grid->getExistingTileData(coords);
//				if (existingTile && !existingTile->isPassable(entity->specification->passableFlags))
//					continue;
//
//				// ignore hexes that are occupied by units (but only if they are visible)
//				e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
//				if (grid->isVisible(coords) && otherUnit)
//					continue;
//
//				e2::PathFindingHex* nextHexEntry = e2::create<e2::PathFindingHex>(nextHex);
//				nextHexEntry->towardsOrigin = currHexEntry;
//				nextHexEntry->stepsFromOrigin = currHexEntry->stepsFromOrigin + 1;
//				nextHexEntry->instantlyReachable = false;
//				hexIndex[coords] = nextHexEntry;
//				queue.push(nextHexEntry);
//			}
//			else
//			{
//				// we are within move bounds, do proper version 
//				if (!grid->isVisible(coords))
//					continue;
//
//				// ignore hexes that are occupied by unpassable biome
//				e2::TileData tile = grid->calculateTileData(coords);
//				if (!tile.isPassable(entity->specification->passableFlags))
//					continue;
//
//				// ignore hexes that are occupied by units
//				e2::GameEntity* otherUnit = game->entityAtHex(e2::EntityLayerIndex::Unit, coords);
//				if (otherUnit)
//					continue;
//
//				e2::PathFindingHex* nextHexEntry = e2::create<e2::PathFindingHex>(nextHex);
//				nextHexEntry->towardsOrigin = currHexEntry;
//				nextHexEntry->stepsFromOrigin = currHexEntry->stepsFromOrigin + 1;
//				nextHexEntry->instantlyReachable = true;
//				hexIndex[coords] = nextHexEntry;
//				queue.push(nextHexEntry);
//			}
//
//
//		}
//
//		processed.insert(currHexEntry->index);
//	}
//}

e2::PathFindingAS::PathFindingAS(e2::Game* game, e2::Hex const& start, uint64_t range, bool ignoreVisibility, e2::PassableFlags passableFlags, bool onlyWaveRelevant, e2::Hex* stopWhenFound)
{
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
			e2::TileData tile = game->hexGrid()->calculateTileData(coords);
			if (!tile.isPassable(passableFlags))
				continue;


			e2::PathFindingHex* newHex = e2::create<e2::PathFindingHex>(n);
			newHex->towardsOrigin = curr;
			newHex->stepsFromOrigin = curr->stepsFromOrigin + 1;
			hexIndex[coords] = newHex;


			queue.push(newHex);
		}

		processed.insert(curr->index);

		if (stopWhenFound && *stopWhenFound == curr->index)
			return;
	}
}

e2::PathFindingAS::~PathFindingAS()
{
	std::unordered_set<e2::PathFindingHex*> hexes;
	for (auto [coords, hex] : hexIndex)
		hexes.insert(hex);
	hexIndex.clear(); 

	while (!hexes.empty())
	{
		e2::PathFindingHex* h = *hexes.begin();
		hexes.erase(h);
		e2::destroy(h);
	}
		
}

e2::PathFindingAS::PathFindingAS()
{

}

std::vector<e2::Hex> e2::PathFindingAS::find(e2::Hex const& target)
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

e2::HitLabel::HitLabel(glm::vec3 const& worldOffset, std::string const& inText) : offset(worldOffset)
, text(inText)
{
	active = true;
	timeCreated = e2::timeNow();
	velocity = glm::vec3{ 50.0f, -200.0f, 50.0f } * 0.02f;
}
