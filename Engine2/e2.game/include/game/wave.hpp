

#pragma once 



#include <e2/application.hpp>
#include "game/hex.hpp"
#include "game/gamecontext.hpp"
#include "game/shared.hpp"

#include <chaiscript/chaiscript.hpp>

namespace e2
{
	struct MobSpecification;
	class Mob;

	/** @tags(arena, arenaSize=4) */
	class Wave : public e2::Object
	{
		ObjectDeclaration();
	public:
		Wave(e2::Game* game, e2::TurnbasedEntity* hiveEntity, e2::TurnbasedEntity* targetEntity);
		virtual ~Wave();

		void pushMobs(std::string const& prefabPath);

		void destroyMob(e2::Mob* mob);

		void update(double seconds);
		bool completed();

		e2::Game* game;
		e2::TurnbasedEntity* hive;
		e2::TurnbasedEntity* target;

		double time{};

		std::queue<std::pair<double, e2::MobSpecification*>> queue;

		std::vector<e2::Hex> path;

		std::unordered_set<e2::Mob*> mobs;
		std::unordered_set<e2::Mob*> deadMobs;
		std::unordered_set<e2::Mob*> swap;

	};
}

#include "wave.generated.hpp"