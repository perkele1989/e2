
#include "game/entity.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"
#include "game/entities/player.hpp"
#include "game/components/staticmeshcomponent.hpp"
#include "e2/managers/audiomanager.hpp"

#include <e2/e2.hpp>
#include <e2/utils.hpp>
#include <e2/transform.hpp>

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <nlohmann/json.hpp>

#include <fstream>


using json = nlohmann::json;





e2::EntitySpecification::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::Entity");
}

e2::EntitySpecification::~EntitySpecification()
{
}

void e2::EntitySpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	if (obj.contains("entityType"))
	{
		e2::Name typeName = obj.at("entityType").template get<std::string>();
		entityType = e2::Type::fromName(typeName);

		if (!entityType)
		{
			LogError("no such type: {}", typeName);
		}
		else if (!entityType->inherits("e2::Entity"))
		{
			LogError("invalid type: {}", typeName);
			entityType = e2::Type::fromName("e2::Entity");
		}
	}
	
	if (obj.contains("displayName"))
	{
		displayName = obj.at("displayName").template get<std::string>();
	}

	if (obj.contains("dependencies"))
	{
		for (nlohmann::json& d : obj.at("dependencies"))
			assets.insert(d.template get<std::string>());
	}
}

e2::Entity::Entity()
{
	m_transform = e2::create<e2::Transform>();
}

e2::Entity::~Entity()
{
	e2::destroy(m_transform);
}

void e2::Entity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	m_game = ctx->game();
	m_specification = spec;
	m_transform->setTranslation(worldPosition, e2::TransformSpace::World);
	m_transform->setRotation(worldRotation, e2::TransformSpace::World);

}

void e2::Entity::writeForSave(e2::IStream& toBuffer)
{
	toBuffer << m_transform->getTranslation(e2::TransformSpace::World);
	toBuffer << m_transform->getRotation(e2::TransformSpace::World);
	toBuffer << m_transform->getScale(e2::TransformSpace::World);
}

void e2::Entity::readForSave(e2::IStream& fromBuffer)
{
	glm::vec3 pos, scale;
	glm::quat rot;
	fromBuffer >> pos >> rot >> scale;
	m_transform->setTranslation(pos, e2::TransformSpace::World);
	m_transform->setRotation(rot, e2::TransformSpace::World);
	m_transform->setScale(scale, e2::TransformSpace::World);
}

e2::Game* e2::Entity::game()
{
	return m_game;
}



void e2::Entity::playSound(e2::Name assetName, float volume, float spatiality)
{
	e2::AssetManager* assetManager = m_game->assetManager();
	e2::AudioManager* audioManager = m_game->audioManager();
	glm::vec3 entityPosition = m_transform->getTranslation(e2::TransformSpace::World);
	e2::SoundPtr soundAsset = assetManager->get(assetName).unsafeCast<e2::Sound>();
	if (!soundAsset)
		return;

	audioManager->playOneShot(soundAsset, volume, spatiality, entityPosition);
}

glm::vec2 e2::Entity::planarCoords()
{
	glm::vec3 pos = m_transform->getTranslation(e2::TransformSpace::World);
	return glm::vec2(pos.x, pos.z);
}

e2::Hex e2::Entity::hex()
{
	return e2::Hex(planarCoords());
}

