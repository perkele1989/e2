
#include "game/mob.hpp"
#include "game/game.hpp"
#include "game/builderunit.hpp"

e2::MainOperatingBase::MainOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Main Operating Base";
	sightRange = 8;
	
	entityType = e2::EntityType::Structure_MainOperatingBase;

	m_buildActionEngineer.buildTurns = 3;
	m_buildActionEngineer.buildTurnsLeft = 3;
	m_buildActionEngineer.buildType = EntityType::Unit_Engineer;
	m_buildActionEngineer.displayName = "Engineer";
}

e2::MainOperatingBase::~MainOperatingBase()
{

}

void e2::MainOperatingBase::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	if (m_currentlyBuilding)
	{
		ui->gameLabel(std::format("Building {}, {} turns left", m_currentlyBuilding->displayName, m_currentlyBuilding->buildTurnsLeft), 12);
	}
	else if (m_buildMessage.size() > 0)
	{
		ui->gameLabel(m_buildMessage, 12);
	}
	

 	ui->beginStackH("actions", 32.0f);

	if (m_currentlyBuilding)
	{
		if (ui->button("cancel", "Cancel"))
		{
			m_currentlyBuilding = nullptr;
		}
	}
	else
	{
		if (ui->button("buildEngineer", "Build Engineer"))
		{
			m_currentlyBuilding = &m_buildActionEngineer;
		}
	}



	ui->endStackH();

	ui->endStackV();
}

void e2::MainOperatingBase::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::MainOperatingBase::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";


	if (m_currentlyBuilding)
	{
		if(m_currentlyBuilding->buildTurnsLeft > 0)
			m_currentlyBuilding->buildTurnsLeft--;

		if (m_currentlyBuilding->buildTurnsLeft <= 0)
		{

			bool hasSlot = false;
			glm::ivec2 slotToUse = tileIndex;
			bool mainSlotFree = game()->unitAtHex(slotToUse) == nullptr;
			if (mainSlotFree)
				hasSlot = true;
			else
			{
				for (e2::Hex& n : e2::Hex(tileIndex).neighbours())
				{
					slotToUse = n.offsetCoords();
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && e2::HexGrid::calculateTileDataForHex(n).isWalkable();

					if (extraSlotFree)
					{
						hasSlot = true;
						break;
					}
				}
			}

			if (!hasSlot)
			{
				m_buildMessage = "Units blocking completed build action, can't spawn this turn.";
			}
			else
			{
				m_currentlyBuilding->buildTurnsLeft = m_currentlyBuilding->buildTurns;
				// spawn the build action here

				switch (m_currentlyBuilding->buildType)
				{
				case e2::EntityType::Unit_Engineer:
					game()->spawnUnit<e2::Engineer>(e2::Hex(slotToUse), empireId);
					break;
				}


				m_buildMessage = std::format("Just finished building {}.", m_currentlyBuilding->displayName);
				m_currentlyBuilding = nullptr;
			}
		}
	}
}

