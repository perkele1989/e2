
#include "game/builderunit.hpp"
#include "game/game.hpp"
#include <e2/game/gamesession.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/renderer.hpp>


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

}

e2::Engineer::~Engineer()
{

}

void e2::Engineer::updateAnimation(double seconds)
{
	m_buildPose->updateAnimation(seconds*4.0f);
	m_idlePose->updateAnimation(seconds);
	m_runPose->updateAnimation(seconds);

	e2::GameUnit::updateAnimation(seconds);

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

	if (canBuild)
	{
		if (isForested)
		{
			// harvest 
			if (ui->button("hrvst", std::format("Harvest Wood on tile ({:.0f} Wood)", woodAbundance * 14.0f)))
			{
				m_buildType = e2::EngineerBuildType::HarvestWood;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}

			// sawmill
			if (ui->button("sawmill", std::format("Build Saw Mill (+{:.0f} Wood per turn)", woodAbundance)))
			{
				m_buildType = e2::EngineerBuildType::SawMill;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}
		}

		if (resource == e2::TileFlags::ResourceGold)
		{
			if (ui->button("goldmine", std::format("Build Gold Mine (+{:.0f} Gold per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::GoldMine;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}
		}

		if (resource == e2::TileFlags::ResourceOil)
		{
			if (ui->button("oilwell", std::format("Build Oil Well (+{:.0f} Oil per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::OilWell;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}
		}

		if (resource == e2::TileFlags::ResourceOre)
		{
			if (ui->button("oremine", std::format("Build Ore Mine (+{:.0f} Metal per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::OreMine;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}
		}

		if (resource == e2::TileFlags::ResourceStone)
		{
			if (ui->button("qwarry", std::format("Build Quarry (+{:.0f} Stone per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::Quarry;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
			}
		}

		if (resource == e2::TileFlags::ResourceUranium)
		{
			if (ui->button("urine", std::format("Build Uranium Mine (+{:.0f} Uranium per turn)", abundance)))
			{
				m_buildType = e2::EngineerBuildType::UraniumMine;
				game()->beginCustomUnitAction();
				playAction(m_buildPose, 0.5f, 1.0f);
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

	m_currentPose = m_idlePose;
	m_oldPose = m_idlePose;
}

void e2::Engineer::updateUnitAction(double seconds)
{
	if (!m_buildPose->playing())
	{
		if (m_buildType == e2::EngineerBuildType::HarvestWood)
		{
			game()->harvestWood(e2::Hex(tileIndex), empireId);
		}
		else if (m_buildType == e2::EngineerBuildType::SawMill)
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, e2::EntityType::Structure_SawMill);
		}
		else if (m_buildType == e2::EngineerBuildType::GoldMine)
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, e2::EntityType::Structure_GoldMine);
		}
		else if (m_buildType == e2::EngineerBuildType::OreMine)
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, e2::EntityType::Structure_OreMine);
		}
		else if (m_buildType == e2::EngineerBuildType::UraniumMine)
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, e2::EntityType::Structure_UraniumMine);
		}
		else if (m_buildType == e2::EngineerBuildType::Quarry)
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId, e2::EntityType::Structure_Quarry);
		}



		game()->endCustomUnitAction();
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
