
#include "game/builderunit.hpp"
#include "game/game.hpp"
#include "game/mob.hpp"
#include <e2/game/gamesession.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/renderer.hpp>


#include <e2/e2.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

e2::Engineer::Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameUnit(ctx, tile, empire)

{
	displayName = "Engineer";
	sightRange = 3;
	moveRange = 3;
	movePointsLeft = 3;

	entityType = e2::EntityType::Unit_Engineer;
	m_modelScale = glm::vec3(1.0f, -1.0f, -1.0f) / 200.0f;
}

e2::Engineer::~Engineer()
{
	if (m_idlePose)
		e2::destroy(m_idlePose);

	if (m_runPose)
		e2::destroy(m_runPose);

	if (m_hitPose)
		e2::destroy(m_hitPose);

	if (m_diePose)
		e2::destroy(m_diePose);

	if (m_buildPose)
		e2::destroy(m_buildPose);
}

void e2::Engineer::updateAnimation(double seconds)
{
	E2_BEGIN_SCOPE_CTX(game());

	e2::Hex hex = e2::Hex(tileIndex);
	e2::Viewpoints2D viewpoints = game()->viewPoints();
	bool inView = viewpoints.isWithin(hex.planarCoords(), 1.0f);


	if (dying && (!m_diePose->playing() || m_diePose->time() > m_diePose->animation()->timeSeconds() - 0.05f))
	{
		game()->queueDestroyUnit(this);
		E2_END_SCOPE_CTX(game());
		return;
	}

	if (inView && !m_proxy->enabled())
		m_proxy->enable();

	if (!inView && m_proxy->enabled())
		m_proxy->disable();


	if (inView)
	{
		m_idlePose->updateAnimation(seconds, false);
		m_runPose->updateAnimation(seconds, false);
	}

	m_buildPose->updateAnimation(seconds*4.0f, !inView);
	m_hitPose->updateAnimation(seconds, !inView);
	m_diePose->updateAnimation(seconds, !inView);

	if(inView)
		e2::GameUnit::updateAnimation(seconds);


	E2_END_SCOPE_CTX(game());

}

void e2::Engineer::drawUI(e2::UIContext* ui)
{

	e2::TileData* tileData = game()->hexGrid()->getTileData(tileIndex);
	if (!tileData)
		return;

	e2::TileFlags resource = tileData->getResource();

	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->gameLabel(std::format("Movement points left: {}", movePointsLeft), 11);
	ui->gameLabel(std::format("Build points left: {}", m_buildPointsLeft), 11);

	//ui->beginStackV("actions");

	bool hasStructure = game()->structureAtHex(tileIndex);
	bool canBuild =!hasStructure && m_buildPointsLeft > 0;
	float abundance = tileData->getAbundanceAsFloat();
	float woodAbundance = tileData->getWoodAbundanceAsFloat();
	
	bool isForested = (tileData->flags & e2::TileFlags::FeatureForest) != e2::TileFlags::FeatureNone;
	bool hasResource = (tileData->flags & e2::TileFlags::ResourceMask) != e2::TileFlags::ResourceNone;
	bool hasWaterTile = false;
	for (e2::Hex nb : e2::Hex(tileIndex).neighbours())
	{
		e2::TileData* nbData = game()->hexGrid()->getTileData(nb.offsetCoords());
		if (!nbData)
			continue;

		if ((nbData->flags & e2::TileFlags::WaterMask) == e2::TileFlags::WaterShallow)
		{
			if (game()->structureAtHex(nb.offsetCoords()))
				continue;

			m_waterHex = nb;
			hasWaterTile = true;
			break;
		}
	}


	if (canBuild)
	{
		if (!isForested && !hasResource)
		{
			if (ui->button("wf", "Build War Factory"))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_Factory;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}

			if (ui->button("brc", "Build Barracks"))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_Barracks;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}

			if (ui->button("arb", "Build Airbase"))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_Airbase;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (hasWaterTile && ui->button("navb", "Build Naval Base"))
		{
			m_buildType = e2::EngineerBuildType::SpawnWaterEntity;
			m_spawnEntityType = e2::EntityType::Structure_NavalBase;
			game()->beginCustomEntityAction();
			playAction(m_buildPose, 0.5f, 1.0f);
			m_buildPointsLeft--;
		}


		if (isForested)
		{
			// harvest 
			if (ui->button("hrvst", std::format("Harvest Wood on tile ({:.0f} Wood)", woodAbundance * 14.0f)))
			{
				m_buildType = e2::EngineerBuildType::HarvestWood;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}

			// sawmill
			if (ui->button("sawmill", std::format("Build Saw Mill (+{:.0f} Wood per turn)", woodAbundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_SawMill;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (resource == e2::TileFlags::ResourceGold)
		{
			if (ui->button("goldmine", std::format("Build Gold Mine (+{:.0f} Gold per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_GoldMine;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (resource == e2::TileFlags::ResourceOil)
		{
			if (ui->button("oilwell", std::format("Build Oil Well (+{:.0f} Oil per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_OilWell;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (resource == e2::TileFlags::ResourceOre)
		{
			if (ui->button("oremine", std::format("Build Ore Mine (+{:.0f} Metal per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_OreMine;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (resource == e2::TileFlags::ResourceStone)
		{
			if (ui->button("qwarry", std::format("Build Quarry (+{:.0f} Stone per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_Quarry;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}

		if (resource == e2::TileFlags::ResourceUranium)
		{
			if (ui->button("urine", std::format("Build Uranium Mine (+{:.0f} Uranium per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::SpawnEntity;
				m_spawnEntityType = e2::EntityType::Structure_UraniumMine;
				game()->beginCustomEntityAction();
				playAction(m_buildPose, 0.5f, 1.0f);
				m_buildPointsLeft--;
			}
		}
	}

	//ui->endStackV();

	ui->endStackV();

}

void e2::Engineer::initialize()
{
	e2::GameUnit::initialize();
	m_idlePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::EngineerIdle), true);
	m_runPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::EngineerRun), true);
	m_buildPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::EngineerBuild), false);
	m_hitPose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::SoldierHit), false); // @todo 
	m_diePose = e2::create<e2::AnimationPose>(m_skeleton, game()->getAnimationByIndex(AnimationIndex::EngineerDie), false);

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;
}

void e2::Engineer::updateEntityAction(double seconds)
{
	if (!m_buildPose->playing())
	{
		if (m_buildType == e2::EngineerBuildType::HarvestWood)
		{
			game()->harvestWood(e2::Hex(tileIndex), empireId);
		}
		else if (m_buildType == e2::EngineerBuildType::SpawnEntity)
		{
			switch (m_spawnEntityType)
			{
			case EntityType::Structure_Factory:
				game()->spawnStructure<e2::WarFactory>(e2::Hex(tileIndex), empireId);
				break;
			case EntityType::Structure_Barracks:
				game()->spawnStructure<e2::Barracks>(e2::Hex(tileIndex), empireId);
				break;
			case EntityType::Structure_Airbase:
				game()->spawnStructure<e2::AirBase>(e2::Hex(tileIndex), empireId);
				break;
			case EntityType::Structure_OilWell:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			case EntityType::Structure_GoldMine:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			case EntityType::Structure_OreMine:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			case EntityType::Structure_UraniumMine:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			case EntityType::Structure_Quarry:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			case EntityType::Structure_SawMill:
				game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, m_spawnEntityType);
				break;
			}
			
		}
		else if (m_buildType == e2::EngineerBuildType::SpawnWaterEntity)
		{
			switch (m_spawnEntityType)
			{
			case EntityType::Structure_NavalBase:
				game()->spawnStructure<e2::NavalBase>(m_waterHex, empireId);
				break;
			}
		}
		 
		game()->endCustomEntityAction();
	}

}

void e2::Engineer::onBeginMove()
{
	setCurrentPose(m_runPose, 0.2f);
}

void e2::Engineer::onEndMove()
{
	setCurrentPose(m_idlePose, 0.2f);
}

void e2::Engineer::onTurnEnd()
{
	e2::GameUnit::onTurnEnd();
	m_buildPointsLeft = m_buildPoints;
}

void e2::Engineer::onTurnStart()
{
	e2::GameUnit::onTurnStart();
}

void e2::Engineer::onHit(e2::GameEntity* instigator, float dmg)
{
	e2::GameUnit::onHit(instigator, dmg);

	if (health <= 0.0f)
	{
		kill();
	}
	else
	{
		playAction(m_hitPose, 0.2f, 0.2f);
	}

}

void e2::Engineer::kill()
{
	e2::GameUnit::kill();

	playAction(m_diePose, 0.2f, 0.0f);
}
