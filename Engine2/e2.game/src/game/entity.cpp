
#include "game/entity.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"
#include "game/wave.hpp"
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
	if (obj.contains("type"))
	{
		e2::Name typeName = obj.at("type").template get<std::string>();
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

void e2::Entity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition)
{
	m_game = ctx->game();
	m_specification = spec;
	m_transform->setTranslation(worldPosition, e2::TransformSpace::World);

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

e2::ItemSpecification::ItemSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::ItemEntity");
}

e2::ItemSpecification::~ItemSpecification()
{
	if (wieldHandler)
		e2::destroy(wieldHandler);

	if (useHandler)
		e2::destroy(useHandler);
}

void e2::ItemSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);
	if (obj.contains("stackable"))
	{
		stackable = obj.at("stackable").template get<bool>();
	}

	if (obj.contains("stackSize"))
	{
		stackSize = obj.at("stackSize").template get<uint32_t>();
	}


	if (obj.contains("equippable"))
	{
		equippable = obj.at("equippable").template get<bool>();
	}

	if (obj.contains("wieldable"))
	{
		wieldable = obj.at("wieldable").template get<bool>();
	}

	if (obj.contains("usable"))
	{
		usable = obj.at("usable").template get<bool>();
	}

	if (obj.contains("wieldHandler"))
	{
		wieldHandlerType = e2::Type::fromName(obj.at("wieldHandler").template get<std::string>());
		if (!wieldHandlerType)
		{
			LogError("unrecognized type for wieldhandler");
			return;
		}

		wieldHandler = wieldHandlerType->create()->cast<e2::WieldHandler>();
		if (!wieldHandler)
		{
			LogError("failed to instance wieldhandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("wieldHandlerData"))
		{
			wieldHandler->populate(ctx, obj.at("wieldHandlerData"), assets);
		}
	}

	if (obj.contains("useHandler"))
	{
		useHandlerType = e2::Type::fromName(obj.at("useHandler").template get<std::string>());
		if (!useHandlerType)
		{
			LogError("unrecognized type for usehandler");
			return;
		}

		useHandler = useHandlerType->create()->cast<e2::UseHandler>();
		if (!useHandler)
		{
			LogError("failed to instance usehandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("useHandlerData"))
		{
			useHandler->populate(ctx, obj.at("useHandlerData"));
		}
	}


	if (obj.contains("equipHandler"))
	{
		equipHandlerType = e2::Type::fromName(obj.at("equipHandler").template get<std::string>());
		if (!equipHandlerType)
		{
			LogError("unrecognized type for equiphandler");
			return;
		}

		equipHandler = equipHandlerType->create()->cast<e2::EquipHandler>();
		if (!equipHandler)
		{
			LogError("failed to instance equiphandler, make sure its not purevirtual");
			return;
		}

		if (obj.contains("equipHandlerData"))
		{
			equipHandler->populate(ctx, obj.at("equipHandlerData"));
		}
	}

	if (obj.contains("equipMesh"))
		equipMesh.populate(obj.at("equipMesh"), assets);

	if (obj.contains("dropMesh"))
	{
		dropMesh.populate(obj.at("dropMesh"), assets);
	}

	if (obj.contains("iconSprite"))
	{
		iconSprite = obj.at("iconSprite").template get<std::string>();
	}

	collision.radius = 0.1;

	if (obj.contains("movement"))
	{
		movement.populate(obj.at("movement"), assets);
	}

}

void e2::ItemSpecification::finalize()
{
	dropMesh.finalize(game);
	equipMesh.finalize(game);

	if (equipHandler)
		equipHandler->finalize(game);


	if (useHandler)
		useHandler->finalize(game);

	if (wieldHandler)
		wieldHandler->finalize(game);
}

e2::WieldHandler::~WieldHandler()
{
}

e2::HatchetHandler::~HatchetHandler()
{

}

void e2::HatchetHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
		m_meshSpecification.populate(obj.at("mesh"), deps);

	if(obj.contains("axeSwing"))
		m_axeSwingSpecification.populate(obj.at("axeSwing"), deps);

	if(obj.contains("woodChop"))
		m_woodChopSpecification.populate(obj.at("woodChop"), deps);
}

void e2::HatchetHandler::finalize(e2::GameContext* ctx)
{
	m_meshSpecification.finalize(ctx);
	m_axeSwingSpecification.finalize(ctx);
	m_woodChopSpecification.finalize(ctx);

}

void e2::HatchetHandler::setActive(e2::PlayerEntity* player)
{
	if (m_axeSwing)
		e2::destroy(m_axeSwing);

	m_axeSwing = nullptr;

	if (m_woodChop)
		e2::destroy(m_woodChop);

	m_woodChop = nullptr;

	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_meshSpecification, player);
	m_axeSwing = e2::create<e2::RandomAudioComponent>(&m_axeSwingSpecification, player);
	m_woodChop = e2::create<e2::RandomAudioComponent>(&m_woodChopSpecification, player);
}

void e2::HatchetHandler::setInactive(e2::PlayerEntity* player)
{
	if (m_axeSwing)
		e2::destroy(m_axeSwing);

	m_axeSwing = nullptr;

	if (m_woodChop)
		e2::destroy(m_woodChop);

	m_woodChop = nullptr;

	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;
}

void e2::HatchetHandler::onUpdate(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::TileData cursorTile = game->hexGrid()->getTileData(game->cursorHex());

	if (player->aiming() && mouse.buttonPressed(e2::MouseButton::Left) && !m_actionBusy && cursorTile.isLand())
	{
		player->getMesh()->playAction("chop");
		m_axeSwing->play();
		m_actionBusy = true;
	}

	glm::mat4 playerTransform = player->getTransform()->getTransformMatrix(e2::TransformSpace::World);
	glm::mat4 playerScale = player->getMesh()->getScaleTransform();
	glm::mat4 itemScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.8f));
	glm::mat4 handTransform = player->getMesh()->boneTransform("hand_r");
	glm::mat4 boneCalibration = glm::identity<glm::mat4>();
	boneCalibration = glm::rotate(boneCalibration, glm::radians(180.0f), e2::worldUpf());
	boneCalibration = glm::rotate(boneCalibration, glm::radians(90.0f), e2::worldRightf());
	m_mesh->applyCustomTransform(playerTransform * playerScale * handTransform * glm::inverse(playerScale) * itemScale * boneCalibration);
}

void e2::HatchetHandler::onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName)
{

	static e2::Name const triggerHit = "hit";
	if (triggerName == triggerHit)
	{
		m_actionBusy = false;

		glm::vec3 worldPosition = player->getTransform()->getTranslation(e2::TransformSpace::World);
		glm::vec2 planarPosition = glm::vec2(worldPosition.x, worldPosition.z);


		glm::vec2 const& mouse = player->game()->cursorPlane();
		glm::vec2 const playerToMouse = glm::normalize(mouse - planarPosition);

		e2::SweepResult closestResult;
		closestResult.moveDistance = std::numeric_limits<float>::max();
		uint32_t closestTree{};
		bool didHitTree = false;
		glm::ivec2 closestHex;
		for (e2::Collision& c : player->getCollisionCache())
		{
			if (c.type != e2::CollisionType::Tree)
				continue;

			e2::SweepResult result = e2::circleSweepTest(planarPosition, planarPosition + playerToMouse * 0.3f, 0.05f, c.position, c.radius);
			if (result.hit && result.moveDistance < closestResult.moveDistance)
			{
				closestResult = result;
				closestTree = c.treeIndex;
				closestHex = c.hex;
				didHitTree = true;
			}
		}

		if (didHitTree)
		{
			player->hexGrid()->damageTree(closestHex, closestTree, e2::randomFloat(0.15f, 0.27f));
			m_woodChop->play();
		}
	}
}









e2::SwordHandler::~SwordHandler()
{

}

void e2::SwordHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
		m_meshSpecification.populate(obj.at("mesh"), deps);

	if (obj.contains("swordSwing"))
		m_swordSwingSpecification.populate(obj.at("swordSwing"), deps);
}

void e2::SwordHandler::finalize(e2::GameContext* ctx)
{
	m_meshSpecification.finalize(ctx);
	m_swordSwingSpecification.finalize(ctx);
}

void e2::SwordHandler::setActive(e2::PlayerEntity* player)
{
	if (m_swordSwing)
		e2::destroy(m_swordSwing);

	m_swordSwing = nullptr;

	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_meshSpecification, player);
	m_swordSwing = e2::create<e2::RandomAudioComponent>(&m_swordSwingSpecification, player);
}

void e2::SwordHandler::setInactive(e2::PlayerEntity* player)
{
	if (m_swordSwing)
		e2::destroy(m_swordSwing);

	m_swordSwing = nullptr;

	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;
}

void e2::SwordHandler::onUpdate(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::TileData cursorTile = game->hexGrid()->getTileData(game->cursorHex());

	if (player->aiming() && mouse.buttonPressed(e2::MouseButton::Left) && !m_actionBusy && cursorTile.isLand())
	{
		player->getMesh()->playAction("sword");
		m_swordSwing->play();
		m_actionBusy = true;
	}

	glm::mat4 playerTransform = player->getTransform()->getTransformMatrix(e2::TransformSpace::World);
	glm::mat4 playerScale = player->getMesh()->getScaleTransform();
	glm::mat4 itemScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.8f));
	glm::mat4 handTransform = player->getMesh()->boneTransform("hand_r");
	glm::mat4 boneCalibration = glm::identity<glm::mat4>();
	boneCalibration = glm::rotate(boneCalibration, glm::radians(180.0f), e2::worldUpf());
	boneCalibration = glm::rotate(boneCalibration, glm::radians(90.0f), e2::worldRightf());
	m_mesh->applyCustomTransform(playerTransform * playerScale * handTransform * glm::inverse(playerScale) * itemScale * boneCalibration);
}

void e2::SwordHandler::onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName)
{

	static e2::Name const triggerHit = "hit";
	if (triggerName == triggerHit)
	{
		m_actionBusy = false;

		glm::vec3 worldPosition = player->getTransform()->getTranslation(e2::TransformSpace::World);
		glm::vec2 planarPosition = glm::vec2(worldPosition.x, worldPosition.z);


		glm::vec2 const& mouse = player->game()->cursorPlane();
		glm::vec2 const playerToMouse = glm::normalize(mouse - planarPosition);


		for (e2::Collision& c : player->getCollisionCache())
		{
			if (c.type != e2::CollisionType::Component || (c.component && c.component->entity() == player))
				continue;

			e2::OverlapResult result = e2::circleOverlapTest(planarPosition + playerToMouse * 0.15f, 0.15f, c.position, c.radius);
			if (result.overlaps)
			{
				c.component->entity()->onMeleeDamage(player, 25.0f);

			}
		}

		player->game()->hexGrid()->grassCutMask().cut({ planarPosition + playerToMouse * 0.2f , 0.3f });

	}
}








e2::ShieldHandler::~ShieldHandler()
{
}

void e2::ShieldHandler::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	if (obj.contains("knockback"))
		knockback = obj.at("knockback").template get<float>();

	if (obj.contains("stun"))
		stun = obj.at("stun").template get<float>();
}

void e2::ShieldHandler::finalize(e2::GameContext* ctx)
{
}

e2::EquipHandler::~EquipHandler()
{
}

e2::ItemEntity::ItemEntity()
	: e2::Entity()
{
}

e2::ItemEntity::~ItemEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);
}

void e2::ItemEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition);
	m_itemSpecification = getSpecificationAs<e2::ItemSpecification>();
	m_mesh = e2::create<e2::StaticMeshComponent>(&m_itemSpecification->dropMesh, this);
	m_collision = e2::create<e2::CollisionComponent>(&m_itemSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_itemSpecification->movement,this);
}

void e2::ItemEntity::writeForSave(e2::IStream& toBuffer)
{
	e2::Entity::writeForSave(toBuffer);
	toBuffer << m_time << m_rotation;
}

void e2::ItemEntity::readForSave(e2::IStream& fromBuffer)
{
	e2::Entity::readForSave(fromBuffer);
	fromBuffer >> m_time >> m_rotation;
}

void e2::ItemEntity::updateAnimation(double seconds)
{
	e2::Entity::updateAnimation(seconds);

	constexpr float rotationsPerSecond = 0.25f;
	constexpr double periodsPerSecond = 0.24;
	constexpr double maxHeight = 0.10;

	m_time += seconds; 
	double sineWave = glm::sin(glm::radians(m_time * 360.0f * periodsPerSecond));
	double sineWaveUnit = sineWave * 0.5 + 0.5;
	double height = sineWaveUnit * -maxHeight;
	m_mesh->setHeightOffset(height);

	m_rotation += seconds * 360.0f * rotationsPerSecond;

	getTransform()->setRotation(glm::rotate(glm::identity<glm::quat>(), glm::radians(m_rotation), e2::worldUpf()), e2::TransformSpace::World);

	m_mesh->applyTransform();
}

void e2::ItemEntity::update(double seconds)
{
	e2::Entity::update(seconds);

	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::Component | e2::CollisionType::Tree | e2::CollisionType::Mountain, m_collisionCache);

	m_movement->resolve(0.1f, e2::CollisionType::Component | e2::CollisionType::Tree | e2::CollisionType::Mountain, m_collisionCache);
	m_collision->invalidate();
}

void e2::ItemEntity::updateVisibility()
{
	e2::Entity::updateVisibility();
	m_mesh->updateVisibility();
}
