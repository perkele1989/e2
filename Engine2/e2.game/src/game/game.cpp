
#include "game/game.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"
#include "game/mob.hpp"

#include "game/militaryunit.hpp"

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>

e2::Game::Game(e2::Context* ctx)
	: e2::Application(ctx)
{

}

e2::Game::~Game()
{

}

void e2::Game::initialize()
{
	m_session = e2::create<e2::GameSession>(this);

	auto am = assetManager();

	e2::ALJDescription alj;
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

	am->queueWaitALJ(alj);
	m_uiTextureResources = am->get("assets/UI_ResourceIcons.e2a").cast<e2::Texture2D>();
	m_cursorMesh = am->get("assets/environment/trees/SM_PalmTree001.e2a").cast<e2::Mesh>();

	m_irradianceMap = am->get("assets/kloofendal_irr.e2a").cast<e2::Texture2D>();
	m_radianceMap = am->get("assets/kloofendal_rad.e2a").cast<e2::Texture2D>();

	m_session->renderer()->setEnvironment(m_irradianceMap->handle(), m_radianceMap->handle());


	for (uint8_t i = 0; i < (uint8_t)e2::GameStructureType::Count; i++)
	{
		m_structureMeshes[i] = m_cursorMesh;
	}

	e2::MeshPtr sawMillMesh = am->get("assets/environment/SM_SawMill.e2a").cast<e2::Mesh>();
	e2::MeshPtr quarryMesh = am->get("assets/environment/SM_Quarry.e2a").cast<e2::Mesh>();
	e2::MeshPtr mineMesh = am->get("assets/environment/SM_Mine.e2a").cast<e2::Mesh>();
	e2::MeshPtr goldMineMesh = am->get("assets/environment/SM_GoldMine.e2a").cast<e2::Mesh>();
	e2::MeshPtr uraniumMineMesh = am->get("assets/environment/SM_UraniumMine.e2a").cast<e2::Mesh>();

	m_structureMeshes[(uint8_t)e2::GameStructureType::OreMine] = mineMesh;
	m_structureMeshes[(uint8_t)e2::GameStructureType::GoldMine] = goldMineMesh;
	m_structureMeshes[(uint8_t)e2::GameStructureType::UraniumMine] = uraniumMineMesh;
	m_structureMeshes[(uint8_t)e2::GameStructureType::Quarry] = quarryMesh;
	m_structureMeshes[(uint8_t)e2::GameStructureType::SawMill] = sawMillMesh;


	for (uint8_t i = 0; i < (uint8_t)e2::GameUnitType::Count; i++)
	{
		m_unitMeshes[i] = m_cursorMesh;
	}

	m_empires.resize(e2::maxNumEmpires);
	for (EmpireId i = 0; uint64_t(i) < e2::maxNumEmpires - 1; i++)
	{
		m_empires[i] = nullptr;
	}

	m_hexGrid = e2::create<e2::HexGrid>(this);

	m_startViewOrigin = glm::vec2(528.97f, 587.02f);
	m_viewOrigin = m_startViewOrigin;
	m_hexGrid->initializeWorldBounds(m_viewOrigin);

}

void e2::Game::shutdown()
{
	std::unordered_set<e2::GameUnit*> unitsCopy = m_units;
	for (e2::GameUnit* unit : unitsCopy)
		destroyUnit(unit->tileIndex);

	std::unordered_set<e2::GameStructure*> structuresCopy = m_structures;
	for (e2::GameStructure* structure : structuresCopy)
		destroyStructure(structure->tileIndex);


	if (m_unitAS)
		e2::destroy(m_unitAS);
	e2::destroy(m_hexGrid);
	e2::destroy(m_session);
}

void e2::Game::update(double seconds)
{

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();

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
	bool hexChanged = newHex != m_prevCursorHex;
	m_prevCursorHex = m_cursorHex;
	m_cursorHex = newHex;

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

	updateCamera(seconds);

	if (hexChanged)
	{
		onNewCursorHex();
	}

	updateGameState();

	updateAnimation(seconds);

	//m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateStreaming(m_viewOrigin, m_viewPoints, m_viewVelocity);
	m_hexGrid->updateWorldBounds();
	m_hexGrid->renderFogOfWar();

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->outlineTexture(renderManager()->frameIndex()));

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
	ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->outlineTexture(renderManager()->frameIndex()));


	float globalMenuFade = m_haveBegunStart ? 1.0 - glm::smoothstep(0.0, 2.0, m_beginStartTime.durationSince().seconds()) : 1.0f;

	// tinted block
	float blockTimer1 = glm::smoothstep(12.0f, 15.0f, float(timer));
	float blockAlpha = 0.9f - blockTimer1 * 0.9;
	ui->drawQuad({}, resolution, e2::UIColor(102, 88, 66, uint8_t(blockAlpha * 255.0f)));

	// black screen
	float blockAlpha2 = 1.0f - glm::smoothstep(8.0, 10.0, timer);
	ui->drawQuad({}, resolution, e2::UIColor(0, 0, 0, uint8_t(blockAlpha2 * 255.0f)));


	float authorWidth = ui->calculateSDFTextWidth(FontFace::Serif, 36.0f, "Fredrik Haikarainen");
	float authorWidth2 = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "A game by");
	float authorAlpha = glm::smoothstep(3.0, 4.0, timer);
	float authorAlpha2 = glm::smoothstep(2.0, 3.0, timer);

	float authorFadeOut = 1.0 - glm::smoothstep(6.0, 7.0, timer);
	authorAlpha *= authorFadeOut;
	authorAlpha2 *= authorFadeOut;

	e2::UIColor authorColor(255, 255, 255, uint8_t(authorAlpha * 255.0f));
	e2::UIColor authorColor2(255, 255, 255, uint8_t(authorAlpha2 * 255.0f));
	ui->drawSDFText(FontFace::Serif, 36.0f, authorColor, glm::vec2(resolution.x / 2.0f - authorWidth / 2.0f, resolution.y / 2.0f), "Fredrik Haikarainen");
	ui->drawSDFText(FontFace::Serif, 14.0f, authorColor2, glm::vec2(resolution.x / 2.0f - authorWidth2 / 2.0f, (resolution.y / 2.0f) - 36.0f), "A game by");

	float width = ui->calculateSDFTextWidth(FontFace::Serif, 42.0f, "Reveal & Annihilate");

	float menuHeight = 280.0f;
	float menuOffset = resolution.y / 2.0f - menuHeight / 2.0f;

	float cursorY = menuOffset;
	float xOffset = resolution.x / 2.0f - width / 2.0f;


	float blockAlpha3 = glm::smoothstep(10.0, 12.0, timer);

	e2::UIColor textColor(0, 0, 0, uint8_t(blockAlpha3 * 255.0f * globalMenuFade));
	e2::UIColor textColor2(0, 0, 0, uint8_t(blockTimer1 * 170.0f * globalMenuFade));
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor, glm::vec2(xOffset, cursorY), "Reveal & Annihilate");

	static float a = 0.0f;
	a += seconds;

	// outwards 
	float sa = glm::simplex(glm::vec2(a*2.0f, 0.0f));
	// rotation
	float sb = glm::simplex(glm::vec2(a*50.0f + 21.f, 0.0f));

	sa = glm::pow(sa, 4.0f);
	sb = glm::pow(sb, 2.0f);

	if (glm::abs(sa) < 0.2f)
		sa = 0.0f;

	if (glm::abs(sb) < 0.5f)
		sb = 0.0f;

	glm::vec2 ori(0.0f, -sa * 10.0f);
	glm::vec2 offset1 = e2::rotate2d(ori, sb * 360.0f);

	glm::vec2 ori2(0.0f, -sb * 5.f);
	glm::vec2 offset2 = e2::rotate2d(ori2, sb * 360.0f);



	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset1, "Reveal & Annihilate");
	ui->drawSDFText(FontFace::Serif, 42.0f, textColor2, glm::vec2(xOffset, cursorY) + offset2, "Reveal & Annihilate");

	cursorY += 64.0f;

	float blockAlphaNewGame = glm::smoothstep(15.0, 15.25, timer);
	e2::UIColor textColorNewGame(0, 0, 0, uint8_t(blockAlphaNewGame * 170.0f * globalMenuFade));
	float newGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "New Game");
	bool newGameHovered = !m_haveBegunStart && timer > 15.25&& mouse.relativePosition.x > xOffset & mouse.relativePosition.x < xOffset + newGameWidth
		&& mouse.relativePosition.y > cursorY + 20.0f && mouse.relativePosition.y < cursorY + 60.0f;
	ui->drawSDFText(FontFace::Serif, 24.0f, newGameHovered ? 0x000000FF : textColorNewGame, glm::vec2(xOffset, cursorY + 40.0f), "New Game");
	if (!m_haveBegunStart && newGameHovered && leftMouse.clicked)
	{
		beginStartGame();
	}

	float blockAlphaLoadGame = glm::smoothstep(15.25, 15.5, timer);
	e2::UIColor textColorLoadGame(0, 0, 0, uint8_t(blockAlphaLoadGame * 170.0f * globalMenuFade));
	float loadGameWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Load Game");
	bool loadGameHovered = !m_haveBegunStart && timer > 15.5 && mouse.relativePosition.x > xOffset & mouse.relativePosition.x < xOffset + loadGameWidth
		&& mouse.relativePosition.y > cursorY + 60.0f && mouse.relativePosition.y < cursorY + 100.0f;
	ui->drawSDFText(FontFace::Serif, 24.0f, loadGameHovered ? 0x000000FF : textColorLoadGame, glm::vec2(xOffset, cursorY + 40.0f * 2), "Load Game");

	float blockAlphaOptions = glm::smoothstep(15.5, 15.75, timer);
	e2::UIColor textColorOptions(0, 0, 0, uint8_t(blockAlphaOptions * 170.0f * globalMenuFade));
	float optionsWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Options");
	bool optionsHovered = !m_haveBegunStart && timer > 15.75 && mouse.relativePosition.x > xOffset & mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 100.0f && mouse.relativePosition.y < cursorY + 140.0f;
	ui->drawSDFText(FontFace::Serif, 24.0f, optionsHovered ? 0x000000FF : textColorOptions, glm::vec2(xOffset, cursorY + 40.0f * 3), "Options");

	float blockAlphaQuit = glm::smoothstep(15.75, 16.0, timer);
	e2::UIColor textColorQuit(0, 0, 0, uint8_t(blockAlphaQuit * 170.0f * globalMenuFade));
	float quitWidth = ui->calculateSDFTextWidth(FontFace::Serif, 24.0f, "Quit");
	bool quitHovered = !m_haveBegunStart && timer > 16.0 && mouse.relativePosition.x > xOffset & mouse.relativePosition.x < xOffset + optionsWidth
		&& mouse.relativePosition.y > cursorY + 140.0f && mouse.relativePosition.y < cursorY + 180.0f;
	ui->drawSDFText(FontFace::Serif, 24.0f, quitHovered ? 0x000000FF : textColorQuit, glm::vec2(xOffset, cursorY + 40.0f * 4), "Quit");
	if (quitHovered && leftMouse.clicked)
	{
		engine()->shutdown();
	}

	if (m_haveBegunStart && !m_haveStreamedStart && m_hexGrid->numJobsInFlight() == 0 && m_beginStartTime.durationSince().seconds() > 2.1f)
	{
		m_haveStreamedStart = true;
		m_beginStreamTime = e2::timeNow();

		glm::vec2 startCoords = e2::Hex(m_startLocation).planarCoords();
		m_hexGrid->initializeWorldBounds(startCoords);

		// spawn local empire
		m_localEmpireId = spawnEmpire();
		m_localEmpire = m_empires[m_localEmpireId];
		spawnStructure<e2::MainOperatingBase>(m_startLocation, m_localEmpireId);

	}
	if (m_haveStreamedStart)
	{
		float sec = m_beginStreamTime.durationSince().seconds();
		float a = glm::smoothstep(0.0f, 1.0f, sec);
		m_viewOrigin = glm::mix(m_startViewOrigin, e2::Hex(m_startLocation).planarCoords(), a);
	}

	if (m_haveStreamedStart && m_beginStreamTime.durationSince().seconds() > 1.1f)
	{
		
		resumeWorldStreaming();
		startGame();
		m_haveBegunStart = false;
	}

}

void e2::Game::pauseWorldStreaming()
{
	m_hexGrid->m_streamingPaused = true;
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
	findStartLocation();
	pauseWorldStreaming();
	forceStreamLocation(e2::Hex(m_startLocation).planarCoords());


}

void e2::Game::findStartLocation()
{
	// plop us down somehwere nice 
	std::unordered_set<glm::ivec2> attemptedStartLocations;
	bool foundStartLocation{};
	while (!foundStartLocation)
	{
		glm::ivec2 startLocation = e2::randomIvec2({ -512, -512 }, { 512, 512 });
		if (attemptedStartLocations.contains(startLocation))
			continue;

		attemptedStartLocations.insert(startLocation);

		e2::Hex startHex(startLocation);
		e2::TileData startTile = e2::HexGrid::calculateTileDataForHex(startHex);
		if (!startTile.isWalkable())
			continue;

		constexpr bool ignoreVisibility = true;
		auto as = e2::create<e2::PathFindingAccelerationStructure>(this, startHex, 64, ignoreVisibility);
		uint64_t numWalkableHexes = as->hexIndex.size();
		e2::destroy(as);

		if (numWalkableHexes < 64)
			continue;


		m_startLocation = startLocation;
		foundStartLocation = true;
	}

}

void e2::Game::startGame()
{
	if (m_globalState != GlobalState::Menu)
		return;
	m_globalState = GlobalState::Game;

	// kick off gameloop
	onStartOfTurn();


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
		else if (m_turnState == TurnState::UnitAction_Attack)
			updateUnitAttack();
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
			spawnUnit<e2::MilitaryUnit>(m_cursorHex, 0);
			return;
		}
		e2::GameUnit* unitAtHex = nullptr;
		e2::GameStructure* structureAtHex = nullptr;

		auto finder = m_unitIndex.find(m_cursorHex.offsetCoords());
		if (finder != m_unitIndex.end())
			unitAtHex = finder->second;
		
		auto finder2 = m_structureIndex.find(m_cursorHex.offsetCoords());
		if (finder2 != m_structureIndex.end())
			structureAtHex = finder2->second;

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
	if (m_empireTurn == 0)
	{
		// clear and calculate visibility
		m_hexGrid->clearVisibility();

		for (e2::GameUnit* unit : m_units)
		{
			unit->spreadVisibility();
		}

		for (e2::GameStructure* structure : m_structures)
		{
			structure->spreadVisibility();
		}
	}

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


void e2::Game::updateUnitAttack()
{

}

void e2::Game::updateUnitMove()
{
	// how many hexes per second it moves
	constexpr float unitMoveSpeed = 2.4f;
	m_unitMoveDelta += m_timeDelta * unitMoveSpeed;

	auto radiansBetween = [](glm::vec3 const& a, glm::vec3 const & b) -> float {

		glm::vec2 a2 = { a.x, a.z };
		glm::vec2 b2 = { b.x, b.z };
		glm::vec2 na = glm::normalize(a2 - b2);

		glm::vec2 nb = glm::normalize(glm::vec2(0.0f, -1.0f));

		return glm::orientedAngle(nb, na);
	};

	while (m_unitMoveDelta > 1.0f)
	{
		m_unitMoveDelta -= 1.0;
		m_unitMoveIndex++;

		if (m_unitMoveIndex >= m_unitMovePath.size() - 1)
		{
			e2::Hex prevHex = m_unitMovePath[m_unitMovePath.size() - 2];
			e2::Hex finalHex = m_unitMovePath[m_unitMovePath.size() - 1];
			// we are done
			m_selectedUnit->rollbackVisibility();
			m_unitIndex.erase(m_selectedUnit->tileIndex);
			m_selectedUnit->tileIndex = finalHex.offsetCoords();
			m_unitIndex[m_selectedUnit->tileIndex] = m_selectedUnit;
			m_selectedUnit->spreadVisibility();

			

			float angle = radiansBetween(finalHex.localCoords(), prevHex.localCoords());
			m_selectedUnit->setMeshTransform(finalHex.localCoords(), angle);
			if (m_unitAS)
				e2::destroy(m_unitAS);
			m_unitAS = e2::create<PathFindingAccelerationStructure>(m_selectedUnit);
			m_hexGrid->clearOutline();
			for (auto& [coords, hexAS] : m_unitAS->hexIndex)
			{
				m_hexGrid->pushOutline(coords);
			}

			m_turnState = TurnState::Unlocked;

			return;
		}
	}

	e2::Hex currHex = m_unitMovePath[m_unitMoveIndex];
	e2::Hex nextHex = m_unitMovePath[m_unitMoveIndex +1];

	glm::vec3 currHexPos = currHex.localCoords();
	glm::vec3 nextHexPos = nextHex.localCoords();

	glm::vec3 newPos = glm::mix(currHexPos, nextHexPos, m_unitMoveDelta);


	float angle = radiansBetween(nextHexPos, currHexPos);
	//m_session->uiContext()->drawRasterText(e2::FontFace::Sans, 16, 0xFFFF00FF, { 4.0f, 450.0f }, std::format("Angle: {}", angle));
	//LogNotice("Angle: {}", angle);
	//glm::quat rotation = glm::angleAxis(angle, e2::worldUp());

	m_selectedUnit->setMeshTransform(newPos, angle);
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
	}

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

	// Gold
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 0.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.gold, m_resources.profits.gold);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f}, str);
	xCursor += strWidth;

	// Wood
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 1.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.wood, m_resources.profits.wood);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Stone
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 2.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.stone, m_resources.profits.stone);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Metal
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 3.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.metal, m_resources.profits.metal);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Oil
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 4.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.oil, m_resources.profits.oil);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Uranium
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 5.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.uranium, m_resources.profits.uranium);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;

	// Meteorite
	xCursor += 2.0f;
	ui->drawTexturedQuad({ xCursor, 2.0f }, { 24.0f, 24.0f }, 0xFFFFFFFF, m_uiTextureResources->handle(), { (1.0f / 7.0f) * 6.0f, 0.0f }, { 1.0f / 7.0f, 1.0f });
	xCursor += 26.0f;
	str = std::format("{0:} ({0:+})", m_resources.funds.meteorite, m_resources.profits.meteorite);
	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	ui->drawRasterText(e2::FontFace::Serif, fontSize, 0xFFFFFFFF, { xCursor, 14.0f }, str);
	xCursor += strWidth;


	strWidth = ui->calculateTextWidth(e2::FontFace::Serif, fontSize, str);
	str = std::format("Turn {}", m_turn);
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

	float width = miniSize.x;
	float height = miniSize.y;

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

void e2::Game::updateResources()
{
	
}

e2::EmpireId e2::Game::spawnEmpire()
{
	for (e2::EmpireId i = 0; i < m_empires.size(); i++)
	{
		if (!m_empires[i])
		{
			m_empires[i] = e2::create<e2::GameEmpire>(this, i);
			return i;
		}
	}

	LogError("failed to spawn empire, we full rn");
	return 0;
}

void e2::Game::destroyEmpire(EmpireId empireId)
{
	if (!m_empires[empireId])
		return;

	e2::destroy(m_empires[empireId]);
	m_empires[empireId] = nullptr;
}

void e2::Game::deselect()
{
	deselectUnit();
	deselectStructure();
}

void e2::Game::selectUnit(e2::GameUnit* unit)
{
	if (!unit || unit == m_selectedUnit)
		return;


	deselectStructure();

	m_selectedUnit = unit;

	if (m_unitAS)
		e2::destroy(m_unitAS);
	m_unitAS = e2::create<PathFindingAccelerationStructure>(m_selectedUnit);

	m_hexGrid->clearOutline();

	for (auto& [coords, hexAS] : m_unitAS->hexIndex)
	{
		m_hexGrid->pushOutline(coords);
	}
}

void e2::Game::deselectUnit()
{
	if (!m_selectedUnit)
		return;

	m_selectedUnit = nullptr;
	if (m_unitAS)
		e2::destroy(m_unitAS);
	m_unitAS = nullptr;
	m_hexGrid->clearOutline();
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

	m_selectedUnit->movePointsLeft -= m_unitMovePath.size() - 1;

	m_turnState = TurnState::UnitAction_Move;
	m_unitMoveIndex = 0;
	m_unitMoveDelta = 0.0f;
	
}

void e2::Game::updateMainCamera(double seconds)
{
	constexpr float moveSpeed = 50.0f;
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
		m_viewOrigin.x -= moveSpeed * seconds;
	}
	if (kb.keys[int16_t(e2::Key::Right)].state)
	{
		m_viewOrigin.x += moveSpeed * seconds;
	}

	if (kb.keys[int16_t(e2::Key::Up)].state)
	{
		m_viewOrigin.y -= moveSpeed * seconds;
	}
	if (kb.keys[int16_t(e2::Key::Down)].state)
	{
		m_viewOrigin.y += moveSpeed * seconds;
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
	unit->rollbackVisibility();

	if (m_selectedUnit && m_selectedUnit == unit)
		deselectUnit();

	m_unitIndex.erase(unit->tileIndex);
	m_units.erase(unit);

	if (m_empires[unit->empireId])
		m_empires[unit->empireId]->units.erase(unit);

	e2::destroy(unit);
}


namespace
{
	std::vector<e2::Hex> tmpHex;
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

void e2::Game::destroyStructure(e2::Hex const& location)
{
	auto finder = m_structureIndex.find(location.offsetCoords());
	if (finder == m_structureIndex.end())
		return;

	e2::GameStructure* structure = finder->second;
	structure->rollbackVisibility();

	if (m_selectedStructure && m_selectedStructure == structure)
		deselectStructure();

	m_structureIndex.erase(structure->tileIndex);
	m_structures.erase(structure);

	if (m_empires[structure->empireId])
		m_empires[structure->empireId]->structures.erase(structure);

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

e2::MeshPtr e2::Game::getUnitMesh(e2::GameUnitType type)
{
	return m_unitMeshes[uint8_t(type)];
}

e2::MeshPtr e2::Game::getStructureMesh(e2::GameStructureType type)
{
	return m_structureMeshes[uint8_t(type)];
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
	for (e2::GameUnit* unit : m_units)
	{
		unit->updateAnimation(seconds);
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
			e2::TileData tile = e2::HexGrid::calculateTileDataForHex(n);
			if (!tile.isWalkable())
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

e2::PathFindingAccelerationStructure::PathFindingAccelerationStructure(e2::GameContext* ctx, e2::Hex const& start, uint64_t range, bool ignoreVisibility)
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
			e2::TileData tile = e2::HexGrid::calculateTileDataForHex(n);
			if (!tile.isWalkable())
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
