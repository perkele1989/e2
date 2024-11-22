
#include "game/components/audiocomponent.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"
#include <e2/managers/audiomanager.hpp>

e2::RandomAudioComponent::RandomAudioComponent(e2::RandomAudioSpecification* specification, e2::Entity* entity)
	: m_specification(specification)
	, m_entity(entity)
{
	
}

e2::RandomAudioComponent::~RandomAudioComponent()
{

}

void e2::RandomAudioComponent::play(float volume, float spatiality)
{
	m_entity->game()->audioManager()->playOneShot(m_specification->getRandom(), volume, spatiality, m_entity->getTransform()->getTranslation(e2::TransformSpace::World));
}


void e2::RandomAudioSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("assets"))
	{
		for (nlohmann::json& objAsset : obj.at("assets"))
		{
			e2::Name assetName = objAsset.template get<std::string>();
			assetNames.push_back(assetName);
			deps.insert(assetName);
		}
	}


}

void e2::RandomAudioSpecification::finalize(e2::GameContext* ctx)
{

	e2::AssetManager* am = ctx->game()->assetManager();

	for (e2::Name assetName : assetNames)
	{
		assets.push_back(am->get(assetName).cast<e2::Sound>());
	}
}

e2::SoundPtr e2::RandomAudioSpecification::getRandom()
{
	return assets[e2::randomInt(0, assets.size() - 1)];
}
