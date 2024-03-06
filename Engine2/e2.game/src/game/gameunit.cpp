
#include "game/gameunit.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

e2::GameUnit::GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile)
	: m_game(ctx->game())
	, tileIndex(tile)
{
	e2::MeshProxyConfiguration proxyConf{};
	proxyConf.mesh = game()->cursorMesh();
	m_proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
	m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), e2::Hex(tile).localCoords());
	m_proxy->modelMatrixDirty = true;
}

void e2::GameUnit::spreadVisibility()
{
	// @todo reuse vector 
	e2::Hex thisHex(tileIndex);
	std::vector<e2::Hex> hexes;
	e2::Hex::circle(thisHex, sightRange, hexes);

	for (e2::Hex h : hexes)
		hexGrid()->flagVisible(h.offsetCoords(), e2::Hex::distance(h, thisHex) == sightRange);
}

void e2::GameUnit::rollbackVisibility()
{
	// @todo reuse vector 
	e2::Hex thisHex(tileIndex);
	std::vector<e2::Hex> hexes;
	e2::Hex::circle(thisHex, sightRange - 1, hexes);

	for (e2::Hex h : hexes)
		hexGrid()->unflagVisible(h.offsetCoords());
}

e2::GameUnit::~GameUnit()
{
	e2::destroy(m_proxy);
}

