
#include "game/mob.hpp"
#include "game/game.hpp"
#include "game/builderunit.hpp"

#include "game/militaryunit.hpp"

e2::MainOperatingBase::MainOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Main Operating Base";
	sightRange = 2;
	
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
	
	if (m_buildMessage.size() > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::Land);

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

e2::Barracks::Barracks(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Barracks";
	sightRange = 2;

	entityType = e2::EntityType::Structure_Barracks;

	m_buildActionGrunt.buildTurns = 3;
	m_buildActionGrunt.buildTurnsLeft = 3;
	m_buildActionGrunt.buildType = EntityType::Unit_Grunt;
	m_buildActionGrunt.displayName = "Grunt";
}

e2::Barracks::~Barracks()
{

}

void e2::Barracks::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	if (m_currentlyBuilding)
	{
		ui->gameLabel(std::format("Building {}, {} turns left", m_currentlyBuilding->displayName, m_currentlyBuilding->buildTurnsLeft), 12);
	}

	if (m_buildMessage.size() > 0)
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
		if (ui->button("buildGrunt", "Build Grunt"))
		{
			m_currentlyBuilding = &m_buildActionGrunt;
		}
	}



	ui->endStackH();

	ui->endStackV();
}

void e2::Barracks::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::Barracks::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";

	if (m_currentlyBuilding)
	{
		if (m_currentlyBuilding->buildTurnsLeft > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::Land);

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
				case e2::EntityType::Unit_Grunt:
					game()->spawnUnit<e2::Grunt>(e2::Hex(slotToUse), empireId);
					break;
				}


				m_buildMessage = std::format("Just finished building {}.", m_currentlyBuilding->displayName);
				m_currentlyBuilding = nullptr;
			}
		}
	}
}

e2::WarFactory::WarFactory(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "War Factory";
	sightRange = 2;

	entityType = e2::EntityType::Structure_Factory;

	m_buildActionTank.buildTurns = 5;
	m_buildActionTank.buildTurnsLeft = 5;
	m_buildActionTank.buildType = EntityType::Unit_Tank;
	m_buildActionTank.displayName = "Stridsvagn 122";

	m_modelScale = glm::vec3(1.0f, 1.0f, 1.0f) / 1.0f;
}

e2::WarFactory::~WarFactory()
{

}

void e2::WarFactory::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	if (m_currentlyBuilding)
	{
		ui->gameLabel(std::format("Building {}, {} turns left", m_currentlyBuilding->displayName, m_currentlyBuilding->buildTurnsLeft), 12);
	}

	if (m_buildMessage.size() > 0)
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
		if (ui->button("buildTank", "Build Stridsvagn 122"))
		{
			m_currentlyBuilding = &m_buildActionTank;
		}
	}



	ui->endStackH();

	ui->endStackV();
}

void e2::WarFactory::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::WarFactory::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";

	if (m_currentlyBuilding)
	{
		if (m_currentlyBuilding->buildTurnsLeft > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::Land);

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
				case e2::EntityType::Unit_Tank:
					game()->spawnUnit<e2::Tank>(e2::Hex(slotToUse), empireId);
					break;
				}


				m_buildMessage = std::format("Just finished building {}.", m_currentlyBuilding->displayName);
				m_currentlyBuilding = nullptr;
			}
		}
	}
}





e2::NavalBase::NavalBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Naval Base";
	sightRange = 2;

	entityType = e2::EntityType::Structure_NavalBase;

	m_buildActionCombatBoat.buildTurns = 4;
	m_buildActionCombatBoat.buildTurnsLeft = 4;
	m_buildActionCombatBoat.buildType = EntityType::Unit_AssaultCraft;
	m_buildActionCombatBoat.displayName = "CB-90";
}

e2::NavalBase::~NavalBase()
{

}

void e2::NavalBase::drawUI(e2::UIContext* ui)
{
	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	if (m_currentlyBuilding)
	{
		ui->gameLabel(std::format("Building {}, {} turns left", m_currentlyBuilding->displayName, m_currentlyBuilding->buildTurnsLeft), 12);
	}

	if (m_buildMessage.size() > 0)
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
		if (ui->button("buildCombatBoat", "Build CB-90"))
		{
			m_currentlyBuilding = &m_buildActionCombatBoat;
		}
	}



	ui->endStackH();

	ui->endStackV();
}

void e2::NavalBase::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::NavalBase::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";

	if (m_currentlyBuilding)
	{
		if (m_currentlyBuilding->buildTurnsLeft > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::WaterShallow);

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
				case e2::EntityType::Unit_AssaultCraft:
					game()->spawnUnit<e2::CombatBoat>(e2::Hex(slotToUse), empireId);
					break;
				}


				m_buildMessage = std::format("Just finished building {}.", m_currentlyBuilding->displayName);
				m_currentlyBuilding = nullptr;
			}
		}
	}
}













e2::AirBase::AirBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Air Base";
	sightRange = 2;

	entityType = e2::EntityType::Structure_Airbase;

	m_buildActionJetFighter.buildTurns = 4;
	m_buildActionJetFighter.buildTurnsLeft = 4;
	m_buildActionJetFighter.buildType = EntityType::Unit_Fighter;
	m_buildActionJetFighter.displayName = "Fighter Jet";
}

e2::AirBase::~AirBase()
{

}

void e2::AirBase::drawUI(e2::UIContext* ui)
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
		if (ui->button("buildFighterJet", "Build Fighter Jet"))
		{
			m_currentlyBuilding = &m_buildActionJetFighter;
		}
	}



	ui->endStackH();

	ui->endStackV();
}

void e2::AirBase::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::AirBase::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";

	if (m_currentlyBuilding)
	{
		if (m_currentlyBuilding->buildTurnsLeft > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::Land);

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

				/*
				switch (m_currentlyBuilding->buildType)
				{
				case e2::EntityType::Unit_Fighter:
					game()->spawnUnit<e2::CombatBoat>(e2::Hex(slotToUse), empireId);
					break;
				}*/


				m_buildMessage = std::format("Just finished building {}.", m_currentlyBuilding->displayName);
				m_currentlyBuilding = nullptr;
			}
		}
	}
}
















e2::ForwardOperatingBase::ForwardOperatingBase(e2::GameContext* gameCtx, glm::ivec2 const& tile, uint8_t empire)
	: e2::GameStructure(gameCtx, tile, empire)
{
	displayName = "Forward Operating Base";
	sightRange = 2;

	entityType = e2::EntityType::Structure_ForwardOperatingBase;

	m_buildActionEngineer.buildTurns = 3;
	m_buildActionEngineer.buildTurnsLeft = 3;
	m_buildActionEngineer.buildType = EntityType::Unit_Engineer;
	m_buildActionEngineer.displayName = "Engineer";
}

e2::ForwardOperatingBase::~ForwardOperatingBase()
{

}

void e2::ForwardOperatingBase::drawUI(e2::UIContext* ui)
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

void e2::ForwardOperatingBase::onTurnEnd()
{
	e2::GameStructure::onTurnEnd();
}

void e2::ForwardOperatingBase::onTurnStart()
{
	e2::GameStructure::onTurnStart();

	if (m_buildMessage.size() != 0)
		m_buildMessage = "";


	if (m_currentlyBuilding)
	{
		if (m_currentlyBuilding->buildTurnsLeft > 0)
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
					bool extraSlotFree = game()->unitAtHex(n.offsetCoords()) == nullptr && game()->hexGrid()->calculateTileDataForHex(n).isPassable(e2::PassableFlags::Land);

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