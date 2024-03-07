
#include "game/game.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"

#include "game/militaryunit.hpp"

#include <glm/gtx/intersect.hpp>

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
	am->prescribeALJ(alj, "assets/SM_PalmTree001.e2a");
	am->queueWaitALJ(alj);
	m_uiTextureResources = am->get("assets/UI_ResourceIcons.e2a").cast<e2::Texture2D>();
	m_cursorMesh = am->get("assets/SM_PalmTree001.e2a").cast<e2::Mesh>();

	e2::MeshProxyConfiguration proxyConf;
	proxyConf.mesh = m_cursorMesh;
	m_cursorProxy = e2::create<e2::MeshProxy>(m_session, proxyConf);

	m_hexGrid = e2::create<e2::HexGrid>(this, m_session);
}

void e2::Game::shutdown()
{
	
	e2::destroy(m_cursorProxy);
	e2::destroy(m_hexGrid);
	e2::destroy(m_session);
}

void e2::Game::update(double seconds)
{
	m_timeDelta = seconds;

	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();



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
	
	m_cursorProxy->modelMatrix = glm::translate(glm::mat4(1.0f), m_cursorHex.localCoords());
	m_cursorProxy->modelMatrix = glm::scale(m_cursorProxy->modelMatrix, glm::vec3(2.0f));
	m_cursorProxy->modelMatrixDirty = true;

	constexpr float invisibleChunkLifetime = 3.0f;

	// debug buttons 
	if (kb.keys[int16_t(e2::Key::Num1)].pressed)
	{
		m_altView = !m_altView;
		gameSession()->window()->mouseLock(m_altView);
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


	m_hexGrid->assertChunksWithinRangeVisible(m_viewOrigin, m_viewPoints, m_viewVelocity);

	m_hexGrid->renderFogOfWar();

	// ticking session renders renderer too, and blits it to the UI, so we need to do it precisely here (after rendering fog of war and before rendering UI)
	m_session->tick(seconds);

	ui->drawTexturedQuad({}, resolution, 0xFFFFFFFF, m_hexGrid->outlineTexture());

	drawUI();

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
	}

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
		m_state = GameState::TurnPreparing;
		onTurnPreparingBegin();
	}
}

void e2::Game::updateTurn()
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
	if (leftMouse.clicked && leftMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftShift))
		{
			spawnUnit<e2::MilitaryUnit>(m_cursorHex);
			return;
		}

		auto finder = m_unitIndex.find(m_cursorHex.offsetCoords());
		if (finder != m_unitIndex.end())
			selectUnit(finder->second);
		else
			deselectUnit();
	}

	if (rightMouse.clicked && rightMouse.dragDistance <= 2.0f)
	{
		if (kb.state(e2::Key::LeftShift))
		{
			LogNotice("destroying unit at {}", m_cursorHex.offsetCoords());
			destroyUnit(m_cursorHex);
			return;
		}

		deselectUnit();
	}
}

void e2::Game::updateUnitAttack()
{

}

void e2::Game::updateUnitMove()
{

}

void e2::Game::drawUI()
{
	drawStatusUI();

	drawUnitUI();

	drawDebugUI();
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

	/*ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 1.0f), 14.0f }, std::format("Wood: {0:10} ({0:+})", m_resources.funds.wood, m_resources.profits.wood));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 2.0f), 14.0f }, std::format("Stone: {0:10} ({0:+})", m_resources.funds.stone, m_resources.profits.stone));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 3.0f), 14.0f }, std::format("Metal: {0:10} ({0:+})", m_resources.funds.metal, m_resources.profits.metal));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 4.0f), 14.0f }, std::format("Oil: {0:10} ({0:+})", m_resources.funds.oil, m_resources.profits.oil));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 5.0f), 14.0f }, std::format("Uranium: {0:10} ({0:+})", m_resources.funds.uranium, m_resources.profits.uranium));
	ui->drawRasterText(e2::FontFace::Serif, 11, 0xFFFFFFFF, { 4.0f + (160.0f * 6.0f), 14.0f }, std::format("Meteorite: {0:10} ({0:+})", m_resources.funds.meteorite, m_resources.profits.meteorite));*/



}


void e2::Game::drawUnitUI()
{
	if (!m_selectedUnit)
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
	float height = 220.0f;

	glm::vec2 offset = { winSize.x - width - 16.0f - 256.0f - 16.0f, winSize.y - height - 16.0f };

	ui->drawQuadShadow(offset, {width, height}, 8.0f, 0.9f, 4.0f);

	
	uint8_t fontSize = 12;

	std::string str = m_selectedUnit->displayName;
	ui->drawRasterText(e2::FontFace::Serif, 14, 0xFFFFFFFF, offset + glm::vec2(8.f, 14.f), str);



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
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 5.0f) }, std::format("^7Num. chunk meshes: {}", m_hexGrid->numChunkMeshes()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 6.0f) }, std::format("^8High loadtime: {:.2f}ms", m_hexGrid->highLoadTime()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 7.0f) }, std::format("^9Jobs in flight: {}", m_hexGrid->numJobsInFlight()));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 8.0f) }, std::format("^2View Origin: {}", m_viewOrigin));
	ui->drawRasterText(e2::FontFace::Monospace, 14, 0xFFFFFFFF, { xOffset, yOffset + (18.0f * 9.0f) }, std::format("^3View Velocity: {}", m_viewVelocity));

}

void e2::Game::onNewCursorHex()
{
	if (m_selectedUnit)
	{
		m_unitHoverPath = m_unitAS.find(m_cursorHex);
	}
}

void e2::Game::updateResources()
{
	
}

void e2::Game::selectUnit(e2::GameUnit* unit)
{
	if (!unit || unit == m_selectedUnit)
		return;

	m_selectedUnit = unit;
	m_unitAS = PathFindingAccelerationStructure(m_selectedUnit);

	m_hexGrid->clearOutline();

	for (auto& [coords, hexAS] : m_unitAS.hexIndex)
	{
		m_hexGrid->pushOutline(coords);
	}
	
}

void e2::Game::deselectUnit()
{
	m_selectedUnit = nullptr;
	m_unitAS = PathFindingAccelerationStructure();
	m_hexGrid->clearOutline();
}

void e2::Game::updateMainCamera(double seconds)
{
	constexpr float moveSpeed = 10.0f;
	constexpr float viewSpeed = .3f;

	e2::GameSession* session = gameSession();
	e2::Renderer* renderer = session->renderer();
	e2::UIContext* ui = session->uiContext();
	auto& kb = ui->keyboardState();
	auto& mouse = ui->mouseState();
	auto& leftMouse = mouse.buttons[uint16_t(e2::MouseButton::Left)];

	m_viewZoom -= float(mouse.scrollOffset) * 0.1f;
	m_viewZoom = glm::clamp(m_viewZoom, 0.0f, 1.0f);

	float viewFov = glm::mix(65.0f, 45.0f, m_viewZoom);
	float viewAngle = glm::mix(35.0f, 50.0f, m_viewZoom);
	float viewDistance = glm::mix(5.0f, 25.0f, m_viewZoom);

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

	if (leftMouse.pressed)
	{
		m_dragView = calculateRenderView(m_viewDragOrigin);
		// save where in world we pressed
		m_cursorDragOrigin = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc);
		m_viewDragOrigin = m_viewOrigin;
		
	}


	const float dragMultiplier = glm::mix(0.01f, 0.025f, m_viewZoom);
	if (leftMouse.held && leftMouse.dragDistance > 2.0f)
	{
		glm::vec2 newDrag = m_dragView.unprojectWorldPlane(renderer->resolution(), m_cursorNdc);
		glm::vec2 dragOffset = newDrag - m_cursorDragOrigin;
		

		m_viewOrigin = m_viewDragOrigin - dragOffset;
	}

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
	float viewFov = glm::mix(65.0f, 45.0f, m_viewZoom);
	float viewAngle = glm::mix(35.0f, 50.0f, m_viewZoom);
	float viewDistance = glm::mix(5.0f, 25.0f, m_viewZoom);

	glm::quat orientation = glm::rotate(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), glm::radians(viewAngle), { 1.0f, 0.0f, 0.0f });

	e2::RenderView newView{};
	newView.fov = viewFov;
	newView.clipPlane = { 0.01f, 1000.0f };
	newView.origin = glm::vec3(viewOrigin.x, 0.0f, viewOrigin.y) + (orientation * e2::worldForward() * -viewDistance);
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

	if (m_selectedUnit == unit)
		deselectUnit();

	m_unitIndex.erase(unit->tileIndex);
	m_units.erase(unit);
	e2::destroy(unit);
}




e2::GameUnit* e2::Game::unitAtHex(glm::ivec2 const& hex)
{
	auto finder = m_unitIndex.find(hex);
	if (finder == m_unitIndex.end())
		return nullptr;

	return finder->second;
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
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewYaw), e2::worldUp());
		altRotation = glm::rotate(altRotation, glm::radians(m_altViewPitch), e2::worldRight());

		if (kb.keys[int16_t(e2::Key::Space)].state)
		{
			m_altViewOrigin -= e2::worldUp() * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::LeftControl)].state)
		{
			m_altViewOrigin -= -e2::worldUp() * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::A)].state)
		{
			m_altViewOrigin -= altRotation * -e2::worldRight() * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::D)].state)
		{
			m_altViewOrigin -= altRotation * e2::worldRight() * moveSpeed * float(seconds);
		}

		if (kb.keys[int16_t(e2::Key::W)].state)
		{
			m_altViewOrigin -= altRotation * e2::worldForward() * moveSpeed * float(seconds);
		}
		if (kb.keys[int16_t(e2::Key::S)].state)
		{
			m_altViewOrigin -= altRotation * -e2::worldForward() * moveSpeed * float(seconds);
		}

		altRotation = glm::rotate(altRotation, glm::radians(180.0f), e2::worldUp());

		e2::RenderView newView{};
		newView.fov = 65.0f;
		newView.clipPlane = { 0.01f, 1000.0f };
		newView.origin = m_altViewOrigin;
		newView.orientation = altRotation;
		renderer->setView(newView);
	}

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
	// clear and calculate visibility
	m_hexGrid->clearVisibility();

	for (e2::GameUnit* unit : m_units)
	{
		unit->spreadVisibility();
	}
}

void e2::Game::onEndOfTurn()
{

}



void e2::Game::onTurnEndingBegin()
{

}

void e2::Game::onTurnEndingEnd()
{

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
			if (curr->stepsFromOrigin + 1 > unit->moveRange)
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
			e2::TileData* tile = grid->getTileData(coords);
			e2::TileFlags biome = tile->getBiome();
			bool walkable = (biome == e2::TileFlags::BiomeGrassland) || (biome == e2::TileFlags::BiomeForest) || (biome == e2::TileFlags::BiomeDesert) || (biome == e2::TileFlags::BiomeTundra);
			if (!walkable)
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
