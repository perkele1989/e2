
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

	labels.resize(3);
	for (HitLabel& label : labels)
		label.active = false;
}


void e2::GameUnit::kill()
{
	dying = true;
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
		m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), m_position + glm::vec3(e2::worldUp()) * m_heightOffset);
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



void e2::GameEntity::updateEntityAction(double seconds)
{

}

void e2::GameUnit::drawUI(e2::UIContext* ui)
{


	for (HitLabel& label : labels)
	{
		if (!label.active)
			continue;

		if (label.timeCreated.durationSince().seconds() > 1.0f)
		{
			label.active = false;
			continue;
		}

		label.velocity.y += 1000.0f * (float)game()->timeDelta();

		label.offset += label.velocity * (float)game()->timeDelta();

		float fontSize = 14.0f + glm::smoothstep(0.0f, 1.0f, (float)label.timeCreated.durationSince().seconds()) * 10.0f;

		float alpha = (1.0f - glm::smoothstep(0.4f, 1.0f, (float)label.timeCreated.durationSince().seconds())) * 255.f;

		ui->drawSDFText(e2::FontFace::Sans, fontSize, e2::UIColor(255, 255, 255, (uint8_t)alpha), label.offset, label.text);

	}

}

e2::GameUnit::~GameUnit()
{
	destroyProxy();

	if (m_mainPose)
		e2::destroy(m_mainPose);
}

void e2::GameEntity::collectRevenue(ResourceTable& outRevenueTable)
{

}

void e2::GameEntity::collectExpenditure(ResourceTable& outExpenditureTable)
{

}

void e2::GameEntity::initialize()
{
	spreadVisibility();

	m_mesh = game()->getEntityMesh(entityType);
	m_skeleton = game()->getEntitySkeleton(entityType);

	m_targetRotation = glm::angleAxis(0.0f, glm::vec3(e2::worldUp()));
	m_rotation = m_targetRotation;

	buildProxy();

	if(m_skinProxy)
		m_mainPose = e2::create<e2::Pose>(m_skeleton);

}

void e2::GameEntity::onHit(e2::GameEntity* instigator, float damage)
{

}

void e2::GameEntity::onEntityTargetChanged(e2::Hex const& location)
{

}

void e2::GameEntity::onEntityTargetClicked()
{

}

void e2::GameUnit::onTurnEnd()
{
	
}

void e2::GameUnit::onTurnStart()
{
	movePointsLeft = moveRange;
}

void e2::GameUnit::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::GameEntity::onHit(instigator, dmg);

	health -= dmg;
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
		m_proxy->modelMatrix = glm::translate(glm::mat4(1.0), m_position + glm::vec3(e2::worldUp()) * m_heightOffset);
		m_proxy->modelMatrix = m_proxy->modelMatrix * glm::toMat4(m_rotation) * glm::scale(glm::mat4(1.0f), m_modelScale);

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

e2::Mine::Mine(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empireId, e2::EntityType type)
	: e2::GameStructure(ctx, tile, empireId)
{
	e2::TileData* tileData = game()->hexGrid()->getTileData(tile);
	if (!tileData)
		return;

	e2::TileFlags resource = tileData->getResource();
	
	entityType = type;
	if (entityType == e2::EntityType::Structure_GoldMine)
	{
		displayName = "Gold mine";
		;
	}
	else if (entityType == e2::EntityType::Structure_UraniumMine)
	{
		displayName = "Uranium mine";
		;
	}
	else if (entityType == e2::EntityType::Structure_OreMine)
	{
		displayName = "Ore mine";
		;
	}
	else if (entityType == e2::EntityType::Structure_SawMill)
	{
		displayName = "Saw Mill";
		;
	}
	else if (entityType == e2::EntityType::Structure_Quarry)
	{
		displayName = "Quarry";
	}

	sightRange = 2;

	// refresh the chunk to remove forest we just ploinked
	e2::ChunkState* chunk = hexGrid()->getOrCreateChunk(hexGrid()->chunkIndexFromPlanarCoords(e2::Hex(tileIndex).planarCoords()));
	hexGrid()->refreshChunkMeshes(chunk);
}

e2::Mine::~Mine()
{

}

void e2::Mine::collectRevenue(ResourceTable& outRevenueTable)
{
	e2::TileData* tileData = game()->hexGrid()->getTileData(tileIndex);
	float abundance = tileData->getAbundanceAsFloat();

	switch (entityType)
	{
	case EntityType::Structure_GoldMine:
		outRevenueTable.gold += abundance;
		break;
	case EntityType::Structure_OilWell:
		outRevenueTable.oil += abundance;
		break;
	case EntityType::Structure_OreMine:
		outRevenueTable.metal += abundance;
		break;
	case EntityType::Structure_Quarry:
		outRevenueTable.stone += abundance;
		break;
	case EntityType::Structure_UraniumMine:
		outRevenueTable.uranium += abundance;
		break;
	case EntityType::Structure_SawMill:
		outRevenueTable.wood += abundance;
		break;
	}
}

void e2::Mine::collectExpenditure(ResourceTable& outExpenditureTable)
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
