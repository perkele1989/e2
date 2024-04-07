
#include "game/gameunit.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

void e2::GameUnit::setupConfig()
{
	movePointsLeft = movePoints;

	labels.resize(3);
	for (HitLabel& label : labels)
		label.active = false;
}

e2::GameUnit::GameUnit(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameEntity(ctx, tile, empire)
{
	setupConfig();
}


e2::GameUnit::GameUnit()
	: e2::GameEntity()
{
	setupConfig();
}

void e2::GameUnit::kill()
{
	dying = true;
}

void e2::GameEntity::updateAnimation(double seconds)
{
	m_rotation = glm::slerp(m_rotation, m_targetRotation, glm::min(1.0f, float(m_rotationLerpSpeed * seconds)));

	if (m_skinProxy)
	{
		m_mainPose->applyBindPose();

		double poseBlend = glm::clamp(m_lastChangePose.durationSince().seconds() / m_lerpTime, 0.0, 1.0);
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
			double actionCurrentTime = m_actionPose->time();
			double actionTotalTime = m_actionPose->animation()->timeSeconds();
			double blendInCoeff = glm::smoothstep(0.0, m_actionBlendInTime, actionCurrentTime);
			double blendOutCoeff = 1.0 - glm::smoothstep(actionTotalTime - m_actionBlendOutTime, actionTotalTime, actionCurrentTime);
			double actionBlendCoeff = blendInCoeff * blendOutCoeff;
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

void e2::GameUnit::writeForSave(e2::Buffer& toBuffer)
{
	e2::GameEntity::writeForSave(toBuffer);

	toBuffer << health;
	toBuffer << movePoints;
	toBuffer << movePointsLeft;
}

void e2::GameUnit::readForSave(e2::Buffer& fromBuffer)
{
	e2::GameEntity::readForSave(fromBuffer);
	fromBuffer >> health;
	fromBuffer >> movePoints;
	fromBuffer >> movePointsLeft;
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
	if(isLocal())
		spreadVisibility();

	m_mesh = game()->getEntityMesh(entityType);
	m_skeleton = game()->getEntitySkeleton(entityType);


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
	movePointsLeft = movePoints;
}

void e2::GameUnit::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::GameEntity::onHit(instigator, dmg);

	health -= dmg;
}

e2::GameEntity::GameEntity(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empire)
	: e2::GameEntity()
{
	postConstruct(ctx, tile, empire);
}

e2::GameEntity::GameEntity()
{

	m_targetRotation = glm::angleAxis(0.0f, glm::vec3(e2::worldUp()));
	m_rotation = m_targetRotation;

}

e2::GameEntity::~GameEntity()
{
	destroyProxy();

}

void e2::GameEntity::postConstruct(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empire)
{
	m_game = ctx->game();
	tileIndex = tile;
	empireId = empire;
	m_position = e2::Hex(tileIndex).localCoords();
}

void e2::GameEntity::writeForSave(e2::Buffer& toBuffer)
{
	toBuffer << displayName;
	toBuffer << sightRange;
	toBuffer << m_rotation;
	toBuffer << m_targetRotation;
	toBuffer << m_position;

}

void e2::GameEntity::readForSave(e2::Buffer& fromBuffer)
{
	fromBuffer >> displayName;
	fromBuffer >> sightRange;
	fromBuffer >> m_rotation;
	fromBuffer >> m_targetRotation;
	fromBuffer >> m_position;
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

bool e2::GameEntity::isLocal()
{
	return empireId == game()->localEmpire()->id;
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


void e2::GameEntity::setCurrentPose(e2::Pose* pose, double lerpTime)
{
	if (!m_skinProxy)
		return;

	m_oldPose = m_currentPose;
	m_currentPose = pose;

	m_lerpTime = lerpTime;
	m_lastChangePose = e2::timeNow();
}

void e2::GameEntity::playAction(e2::AnimationPose* anim, double blendIn /*= 0.2f*/, double blendOut /*= 0.2f*/)
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

e2::GameStructure::GameStructure(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empireId)
	: e2::GameEntity(ctx, tile, empireId)
{

}

e2::GameStructure::GameStructure()
	: e2::GameEntity()
{

}

e2::GameStructure::~GameStructure()
{

}

void e2::GameStructure::writeForSave(e2::Buffer& toBuffer)
{
	toBuffer << health;
}

void e2::GameStructure::readForSave(e2::Buffer& fromBuffer)
{
	fromBuffer >> health;
}

void e2::GameStructure::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->endStackV();
}

void e2::Mine::setupConfig()
{
	sightRange = 2;
}

e2::Mine::Mine(e2::GameContext* ctx, glm::ivec2 const& tile, EmpireId empireId, e2::EntityType type)
	: e2::GameStructure(ctx, tile, empireId)
{
	entityType = type;
	setupConfig();
}

e2::Mine::Mine()
	: e2::GameStructure()
{
	setupConfig();
}

e2::Mine::~Mine()
{

}

void e2::Mine::writeForSave(e2::Buffer& toBuffer)
{
	e2::GameStructure::writeForSave(toBuffer);

	toBuffer << (uint32_t)entityType;
}

void e2::Mine::readForSave(e2::Buffer& fromBuffer)
{
	e2::GameStructure::readForSave(fromBuffer);

	uint32_t newType;
	fromBuffer >> newType;
	entityType = (EntityType)newType;
}

void e2::Mine::initialize()
{
	// refresh the chunk to remove forest we just ploinked @todo this doesnt always actually refresh! why? (chunkindexfromplanarcoords wrong? should we use fromoffsetcoords?)
	e2::ChunkState* chunk = hexGrid()->getOrCreateChunk(hexGrid()->chunkIndexFromPlanarCoords(e2::Hex(tileIndex).planarCoords()));
	hexGrid()->refreshChunkMeshes(chunk);

	if (entityType == e2::EntityType::Structure_GoldMine)
		displayName = "Gold mine";
	else if (entityType == e2::EntityType::Structure_UraniumMine)
		displayName = "Uranium mine";
	else if (entityType == e2::EntityType::Structure_OreMine)
		displayName = "Ore mine";
	else if (entityType == e2::EntityType::Structure_SawMill)
		displayName = "Saw Mill";
	else if (entityType == e2::EntityType::Structure_Quarry)
		displayName = "Quarry";

	e2::GameStructure::initialize();
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
