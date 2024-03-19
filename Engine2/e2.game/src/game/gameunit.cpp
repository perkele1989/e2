
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



void e2::GameEntity::updateAnimation(double seconds)
{
	m_rotation = glm::slerp(m_rotation, m_targetRotation, glm::min(1.0f, float(8.02 * seconds)));

	if (m_skinProxy)
	{
		m_mainPose->applyBindPose();

		float poseBlend = glm::clamp((float)m_lastChangePose.durationSince().seconds() / m_lerpTime, 0.0f, 1.0f);
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
			float actionCurrentTime = m_actionPose->time();
			float actionTotalTime = m_actionPose->animation()->timeSeconds();
			float blendInCoeff = glm::smoothstep(0.0f, m_actionBlendInTime, actionCurrentTime);
			float blendOutCoeff = 1.0f - glm::smoothstep(actionTotalTime - m_actionBlendOutTime, actionTotalTime, actionCurrentTime);
			float actionBlendCoeff = blendInCoeff * blendOutCoeff;
			m_mainPose->blendWith(m_actionPose, actionBlendCoeff);
		}

		m_mainPose->updateSkin();

		m_skinProxy->applyPose(m_mainPose);
	}

	if (m_proxy)
	{
		m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), m_position);
		m_proxy->modelMatrix = m_proxy->modelMatrix * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.0f), m_modelScale);
		m_proxy->modelMatrixDirty = true;
	}


	constexpr bool debugRender = false;
	if (m_skinProxy && debugRender)
	{
		glm::vec3 colors[4] = {
		{1.0, 0.0, 0.0},
		{0.0, 1.0, 0.0},
		{0.0, 0.0, 1.0},
		{1.0, 1.0, 1.0},
		};

		auto renderer = game()->gameSession()->renderer();
		auto skeleton = m_skinProxy->skeletonAsset;

		glm::mat4 modelMatrix = m_proxy->modelMatrix;

		for (uint32_t b = 0; b < m_skinProxy->skeletonAsset->numBones(); b++)
		{
			e2::Bone* bone = m_skinProxy->skeletonAsset->boneById(b);
			e2::PoseBone* poseBone = m_mainPose->poseBoneById(b);
			e2::Bone* parentBone = bone->parent;
			e2::PoseBone* parentPoseBone = parentBone ? m_mainPose->poseBoneById(parentBone->index) : nullptr;


			glm::mat4 boneTransform = modelMatrix * poseBone->cachedGlobalTransform;
			glm::vec3 boneOrigin = boneTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

			glm::vec3 boneRight = boneTransform * glm::vec4(e2::worldRight(), 0.0f);
			glm::vec3 boneUp = boneTransform * glm::vec4(e2::worldUp(), 0.0f);
			glm::vec3 boneForward = boneTransform * glm::vec4(e2::worldForward(), 0.0f);


			renderer->debugLine(colors[0], boneOrigin, boneOrigin + boneRight * 0.05f);
			renderer->debugLine(colors[1], boneOrigin, boneOrigin + boneUp * 0.05f);
			renderer->debugLine(colors[2], boneOrigin, boneOrigin + boneForward * 0.05f);


			if (parentBone)
			{
				glm::mat4 parentTransform = modelMatrix * parentPoseBone->cachedGlobalTransform;
				glm::vec3 parentOrigin = parentTransform * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
				renderer->debugLine(colors[3], parentOrigin, boneOrigin);
			}

		}
	}
}



void e2::GameUnit::updateUnitAction(double seconds)
{

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
	destroyProxy();
}

void e2::GameEntity::initialize()
{
	spreadVisibility();

	m_mesh = game()->getEntityMesh(entityType);
	m_skeleton = game()->getEntitySkeleton(entityType);

	buildProxy();

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
	if (m_mesh && !m_proxy)
	{
		e2::MeshProxyConfiguration proxyConf{};
		proxyConf.mesh = m_mesh;

		m_proxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		m_proxy->modelMatrix = glm::translate(glm::identity<glm::mat4>(), m_position);
		m_proxy->modelMatrixDirty = true;
	}

	if (m_mesh && m_skeleton && !m_skinProxy)
	{
		e2::SkinProxyConfiguration conf;
		conf.mesh = m_mesh;
		conf.skeleton = m_skeleton;
		m_skinProxy = e2::create<e2::SkinProxy>(game()->gameSession(), conf);
		m_proxy->skinProxy = m_skinProxy;
		m_proxy->invalidatePipeline();
	}
}

void e2::GameEntity::destroyProxy()
{
	if (m_skinProxy)
		e2::destroy(m_skinProxy);

	if (m_proxy)
		e2::destroy(m_proxy);

	m_proxy = nullptr;
	m_skinProxy = nullptr;
}


void e2::GameEntity::setCurrentPose(e2::Pose* pose, float lerpTime)
{
	if (!m_skinProxy)
		return;

	m_oldPose = m_currentPose;
	m_currentPose = pose;

	m_lerpTime = lerpTime;
	m_lastChangePose = e2::timeNow();
}

void e2::GameEntity::playAction(e2::AnimationPose* anim, float blendIn /*= 0.2f*/, float blendOut /*= 0.2f*/)
{
	if (!m_skinProxy)
		return;

	m_actionPose = anim;
	m_actionBlendInTime = blendIn;
	m_actionBlendOutTime = blendOut;

	m_actionPose->play(false);
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


void e2::GameEntity::onBeginMove()
{

}

void e2::GameEntity::onEndMove()
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
	{
		displayName = "Gold mine";
		entityType = e2::EntityType::Structure_GoldMine;
	}
	else if (resource == TileFlags::ResourceUranium)
	{
		displayName = "Uranium mine";
		entityType = e2::EntityType::Structure_UraniumMine;
	}
	else if (resource == TileFlags::ResourceOre)
	{
		displayName = "Ore mine";
		entityType = e2::EntityType::Structure_OreMine;
	}
	else if (resource == TileFlags::ResourceForest)
	{
		displayName = "Saw Mill";
		entityType = e2::EntityType::Structure_SawMill;
	}
	else if (resource == TileFlags::ResourceStone)
	{
		displayName = "Quarry";
		entityType = e2::EntityType::Structure_Quarry;
	}

	sightRange = 2;

	tileData->improvedResource = true;

	// refresh the chunk to remove forest we just ploinked
	e2::ChunkState* chunk = hexGrid()->getOrCreateChunk(hexGrid()->chunkIndexFromPlanarCoords(e2::Hex(tileIndex).planarCoords()));
	hexGrid()->refreshChunkForest(chunk);
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
