#pragma once 

#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/assets/sound.hpp>
#include <nlohmann/json.hpp>


namespace e2
{

	class GameContext;

	struct RandomAudioSpecification
	{
		void populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps);
		void finalize(e2::GameContext* ctx);

		std::vector<e2::Name> assetNames;
		std::vector<e2::SoundPtr> assets;

		e2::SoundPtr getRandom();

	};

	class Entity;
	
	/** @tags(arena, arenaSize=128) */
	class RandomAudioComponent : public e2::Object
	{
		ObjectDeclaration();
	public:
		RandomAudioComponent(e2::RandomAudioSpecification* specification, e2::Entity* entity);
		~RandomAudioComponent();

		void play(float volume = 1.0f, float spatiality = 1.0f);

	protected:

		e2::Entity* m_entity{};
		e2::RandomAudioSpecification* m_specification{};

	};


}


#include "audiocomponent.generated.hpp"