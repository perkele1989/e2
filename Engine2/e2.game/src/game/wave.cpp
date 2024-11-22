
#include "game/wave.hpp"
#include "game/game.hpp"
#include "game/mob.hpp"

#include "game/entities/turnbasedentity.hpp"
#include <utility>


void e2::Wave::pushMobs(std::string const& prefabPath)
{
	std::string raw;
	if (!e2::readFileWithIncludes(prefabPath, raw))
	{
		return;
	}

	std::vector<std::string> lines = e2::split(raw, '\n');
	for (std::string line : lines)
	{
		line = e2::trim(line);
		if (line.empty())
			continue;
		if (line[0] == '#')
			continue;


		std::vector<std::string> parts = e2::split(line, ' ');
		if (parts.size() != 2)
			continue;

		double time = strtod(parts[0].c_str(), nullptr);
		e2::Name mobName = e2::trim(parts[1]);
		e2::MobSpecification* spec = e2::MobSpecification::specificationById(mobName);
		if (!spec)
			continue;

		queue.push(std::make_pair(time, spec));
	}
}

e2::Wave::Wave(e2::Game* game, e2::TurnbasedEntity* hiveEntity, e2::TurnbasedEntity* targetEntity)
	: game(game)
	, hive(hiveEntity)
	, target(targetEntity)
{
	//int32_t dist = e2::Hex::distance(city->entity->tileIndex, hiveEntity->tileIndex);
	e2::Hex cityHex = target->getTileIndex();
	e2::PathFindingAS *as = e2::create<e2::PathFindingAS>(game, e2::Hex(hive->getTileIndex()), 1024, true, PassableFlags::Land, true, &cityHex);
	path = as->find(cityHex);
	e2::destroy(as);
}

e2::Wave::~Wave()
{
	for(auto m : deadMobs)
		e2::destroy(m);
}

void e2::Wave::destroyMob(e2::Mob* mob)
{
	game->onMobDestroyed(mob);
	mobs.erase(mob);
	deadMobs.insert(mob);
}

void e2::Wave::update(double seconds)
{
	time += seconds;

	// spawn new mobs
	while (!queue.empty() && time > queue.front().first)
	{
		auto p = queue.front();
		queue.pop();
		time -= p.first;
		
		e2::Mob *newMob = e2::create<e2::Mob>(game, p.second, this);
		newMob->initialize();
		mobs.insert(newMob);

		game->onMobSpawned(newMob);

		hive->meshComponent()->playAction("spawn");
	}

	// update existing mobs 
	swap = mobs;
	for (e2::Mob* mob : swap)
	{
		mob->update(seconds);
		mob->updateAnimation(seconds);
	}
}

bool e2::Wave::completed()
{
	return queue.empty() && mobs.empty();
}
