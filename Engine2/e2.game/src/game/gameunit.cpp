
#include "game/gameunit.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

e2::GameUnit::GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile)
	: m_game(ctx->game())
	, tileIndex(tile)
{
	e2::MeshProxyConfiguration proxyConf{};
	proxyConf.mesh = game()->cursorMesh();
	m_proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
	m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), e2::Hex(tile).localCoords());
	m_proxy->modelMatrixDirty = true;
	m_position = e2::Hex(tile).localCoords();
}

void e2::GameUnit::setMeshTransform(glm::vec3 const& pos, float angle)
{
	m_targetRotation = glm::angleAxis(angle, e2::worldUp());
	m_position = pos;
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

void e2::GameUnit::updateAnimation(double seconds)
{
	
	m_rotation = glm::slerp(m_rotation, m_targetRotation, glm::min(1.0f, float(8.02 * seconds)));

	m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), m_position);
	m_proxy->modelMatrix = m_proxy->modelMatrix * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.0f), glm::vec3(2.0f, 2.0f, 2.0f));
	m_proxy->modelMatrixDirty = true;
}

e2::GameUnit::~GameUnit()
{
	e2::destroy(m_proxy);
}

