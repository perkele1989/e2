
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
	sightRange = 5;
	moveRange = 8;
	movePointsLeft = 8;

	entityType = e2::EntityType::Unit_Engineer;

}

e2::Engineer::~Engineer()
{

}

void e2::Engineer::updateAnimation(double seconds)
{
	e2::GameUnit::updateAnimation(seconds);

}

void e2::Engineer::drawUI(e2::UIContext* ui)
{

	e2::TileData* tileData = game()->hexGrid()->getTileData(tileIndex);
	if (!tileData)
		return;

	auto resStr = [](e2::TileFlags res) -> std::string {
		switch (res)
		{
		case e2::TileFlags::ResourceForest:
			return "Saw Mill";
			break;
		case e2::TileFlags::ResourceStone:
			return "Quarry";
			break;
		case e2::TileFlags::ResourceOre:
			return "Ore Mine";
			break;
		case e2::TileFlags::ResourceGold:
			return "Gold Mine";
			break;
		case e2::TileFlags::ResourceOil:
			return "Oil Well";
			break;
		case e2::TileFlags::ResourceUranium:
			return "Uranium Mine";
			break;
		}
		return "";
	};

	e2::TileFlags resource = tileData->getResource();

	ui->beginStackV("test2");

	ui->gameLabel(std::format("**{}**", displayName), 12, e2::UITextAlign::Middle);

	ui->gameLabel(std::format("Movement points left: {}", movePointsLeft), 11);

	ui->beginStackH("actions", 32.0f);

	bool hasStructure = game()->structureAtHex(tileIndex);

	if (!tileData->improvedResource && !hasStructure && resource != TileFlags::ResourceNone)
	{
		if (ui->button("build", std::format("Build {}", resStr(resource))))
		{
			game()->spawnStructure<e2::Mine>(e2::Hex(tileIndex), empireId);
			game()->destroyUnit(e2::Hex(tileIndex));
		}
	}

	ui->endStackH();

	ui->endStackV();

}
