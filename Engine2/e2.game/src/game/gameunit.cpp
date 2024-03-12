
#include "game/gameunit.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

e2::GameUnit::GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameEntity(ctx, tile, empire)
{
	movePointsLeft = moveRange;
}



void e2::GameUnit::updateAnimation(double seconds)
{
	
	m_rotation = glm::slerp(m_rotation, m_targetRotation, glm::min(1.0f, float(8.02 * seconds)));

	if (!m_proxy)
		return;

	m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), m_position);
	m_proxy->modelMatrix = m_proxy->modelMatrix * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
	m_proxy->modelMatrixDirty = true;
}

void e2::GameUnit::drawUI(e2::UIContext* ui)
{

	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->gameLabel(std::format("Movement points left: {}", movePointsLeft), 11);

	ui->endStackV();
}

e2::GameUnit::~GameUnit()
{

}

void e2::GameUnit::onTurnEnd()
{
	
}

void e2::GameUnit::onTurnStart()
{
	movePointsLeft = moveRange;
}

e2::GameEntity::GameEntity(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empire)
	: m_game(ctx->game())
	, tileIndex(tile)
	, empireId(empire)
{
	m_position = e2::Hex(tile).localCoords();
}

e2::GameEntity::~GameEntity()
{
	destroyProxy();
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
	m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
	m_position = pos;
}

glm::vec2 e2::GameEntity::planarCoords()
{
	return glm::vec2(m_position.x, m_position.z);
}

void e2::GameEntity::buildProxy()
{
	if (m_proxy)
		return;

	e2::MeshProxyConfiguration proxyConf{};
	proxyConf.mesh = m_mesh;

	m_proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
	m_proxy->modelMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
	m_proxy->modelMatrixDirty = true;
}

void e2::GameEntity::destroyProxy()
{
	if (m_proxy)
		e2::destroy(m_proxy);

	m_proxy = nullptr;
}



void e2::GameEntity::spreadVisibility()
{
	::cleanTmpHex();
	
	e2::Hex thisHex(tileIndex);
	e2::Hex::circle(thisHex, sightRange, ::tmpHex);

	for (e2::Hex h : ::tmpHex)
		hexGrid()->flagVisible(h.offsetCoords(), e2::Hex::distance(h, thisHex) == sightRange);
}

void e2::GameEntity::rollbackVisibility()
{
	::cleanTmpHex();

	e2::Hex thisHex(tileIndex);
	e2::Hex::circle(thisHex, sightRange - 1, ::tmpHex);

	for (e2::Hex h : ::tmpHex)
		hexGrid()->unflagVisible(h.offsetCoords());
}

void e2::GameEntity::onTurnEnd()
{

}

void e2::GameEntity::onTurnStart()
{

}

void e2::GameEntity::updateAnimation(double seconds)
{

}

e2::GameStructure::GameStructure(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId)
	: e2::GameEntity(ctx, tile, empireId)
{

}

e2::GameStructure::~GameStructure()
{

}

void e2::GameStructure::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->endStackV();
}

e2::Mine::Mine(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId)
	: e2::GameStructure(ctx, tile, empireId)
{
	e2::TileData* tileData = game()->hexGrid()->getTileData(tile);
	if (!tileData)
		return;

	e2::TileFlags resource = tileData->getResource();
	tileData->improvedResource = true;

	if (resource == TileFlags::ResourceGold)
		displayName = "Gold mine";
	else if (resource == TileFlags::ResourceUranium)
		displayName = "Uranium mine";
	else if (resource == TileFlags::ResourceOre)
		displayName = "Ore mine";
	else if (resource == TileFlags::ResourceForest)
		displayName = "Saw Mill";
	else if (resource == TileFlags::ResourceStone)
		displayName = "Quarry";

	sightRange = 2;
}

e2::Mine::~Mine()
{

}

void e2::Mine::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();

}

void e2::Mine::onTurnStart()
{
	e2::GameStructure::onTurnStart();

}
