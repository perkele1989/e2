
#include "game/playerstate.hpp"

#include "game/entity.hpp"
#include "game/game.hpp"

#include "game/entities/player.hpp"
#include "game/entities/itementity.hpp"
#include "game/entities/throwableentity.hpp"
#include "game/entities/radionentity.hpp"
#include "game/components/staticmeshcomponent.hpp"

#include <e2/renderer/meshproxy.hpp>
#include <e2/game/gamesession.hpp>
#include <e2/managers/audiomanager.hpp>
#include <e2/game/gamesession.hpp>
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


e2::PlayerState::PlayerState(e2::Game* g)
	: game(g)
{
}

bool e2::PlayerState::give(e2::Name itemIdentifier)
{
	e2::ItemSpecification* item = game->getItemSpecification(itemIdentifier);
	if (!item)
		return false;

	if (item->stackable)
	{
		for (uint32_t i = 0; i < inventory.size(); i++)
		{
			e2::InventorySlot& slot = inventory[i];
			if (slot.item == item && slot.count < item->stackSize)
			{

				if (i == activeSlot && slot.item->wieldable && slot.item->wieldHandler && slot.count == 0 && entity)
				{
					slot.item->wieldHandler->setActive(entity);
				}

				slot.count++;

				entity->playSound("S_Item_Pickup.e2a", 1.0, 1.0);

				return true;
			}
		}
	}
	for (uint32_t i = 0; i < inventory.size(); i++)
	{
		e2::InventorySlot& slot = inventory[i];
		if (slot.item == nullptr || slot.count == 0)
		{
			slot.item = item;
			slot.count = 1;

			if (i == activeSlot && slot.item->wieldable && slot.item->wieldHandler && entity)
			{
				slot.item->wieldHandler->setActive(entity);
			}

			entity->playSound("S_Item_Pickup.e2a", 1.0, 1.0);

			return true;
		}
	}

	return false;
}

void e2::PlayerState::drop(uint32_t slotIndex, uint32_t num)
{
	e2::InventorySlot& slot = inventory[slotIndex];
	if (!slot.item || slot.count == 0)
		return;

	num = glm::clamp(num, (uint32_t)1, slot.count);
	slot.count -= num;

	if (slotIndex == activeSlot && slot.count == 0 && entity)
	{
		if(slot.item->wieldable && slot.item->wieldHandler)
			slot.item->wieldHandler->setInactive(entity);
	}

	if (entity && num > 0)
	{
		for (uint32_t i = 0; i < num; i++)
			game->spawnEntity(slot.item->id, entity->getTransform()->getTranslation(e2::TransformSpace::World));

		entity->playSound("S_Item_Drop.e2a", 1.0, 1.0);
	}

	if (slot.count == 0)
	{
		slot.item = nullptr;
	}
}

void e2::PlayerState::take(uint32_t slotIndex, uint32_t num)
{
	e2::InventorySlot& slot = inventory[slotIndex];
	if (!slot.item || slot.count == 0)
		return;

	num = glm::clamp(num, (uint32_t)1, slot.count);
	slot.count -= num;

	if (slotIndex == activeSlot && slot.count == 0 && entity)
	{
		if (slot.item->wieldable && slot.item->wieldHandler)
			slot.item->wieldHandler->setInactive(entity);
	}

	if (slot.count == 0)
	{
		slot.item = nullptr;
	}
}

bool e2::PlayerState::takeById(e2::Name itemName, uint32_t num)
{
	struct _slotFuck
	{
		uint32_t slotIndex;
		uint32_t num;
	};

	e2::StackVector<_slotFuck, 32> slotsToFuck;

	for (uint32_t slotIndex = 0; slotIndex < 32; slotIndex++)
	{
		if (num == 0)
		{
			break;
		}

		if (inventory[slotIndex].item && inventory[slotIndex].item->id == itemName)
		{
			uint32_t toFuck = glm::min(num, inventory[slotIndex].count);
			slotsToFuck.push({ slotIndex, toFuck});
			num -= toFuck;
		}
	}

	if (num > 0)
	{
		return false;
	}

	for (_slotFuck& fuck : slotsToFuck)
	{
		take(fuck.slotIndex, fuck.num);
	}

	return true;
}

void e2::PlayerState::setActiveSlot(uint8_t newSlotIndex)
{
	e2::InventorySlot& slot = inventory[activeSlot];
	if (slot.item)
	{
		if (slot.item->wieldable && slot.item->wieldHandler)
		{
			slot.item->wieldHandler->setInactive(entity);
		}
	}

	activeSlot = newSlotIndex;
	e2::InventorySlot& newSlot = inventory[activeSlot];
	if (newSlot.item)
	{
		if (newSlot.item->wieldable && newSlot.item->wieldHandler)
		{
			newSlot.item->wieldHandler->setActive(entity);
		}
	}


}

void e2::PlayerState::update(double seconds)
{
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();

	if (entity)
	{
		glm::vec2 inputVector{};

		if (!game->scriptRunning())
		{
			if (!entity->getMesh()->isAnyActionPlaying())
			{
				if (kb.pressed(e2::Key::Num1))
				{
					setActiveSlot(0);
				}
				if (kb.pressed(e2::Key::Num2))
				{
					setActiveSlot(1);
				}
				if (kb.pressed(e2::Key::Num3))
				{
					setActiveSlot(2);
				}
				if (kb.pressed(e2::Key::Num4))
				{
					setActiveSlot(3);
				}
				if (kb.pressed(e2::Key::Num5))
				{
					setActiveSlot(4);
				}
				if (kb.pressed(e2::Key::Num6))
				{
					setActiveSlot(5);
				}
				if (kb.pressed(e2::Key::Num7))
				{
					setActiveSlot(6);
				}
				if (kb.pressed(e2::Key::Num8))
				{
					setActiveSlot(7);
				}

				if (kb.pressed(e2::Key::G))
				{
					drop(activeSlot, 1);
				}
				//if (kb.pressed(e2::Key::Q))
				//{
				//	give("ironhatchet");
				//}
				//if (kb.pressed(e2::Key::E))
				//{
				//	give("bomb");
				//	give("redbomb");
				//}
			}


			if (kb.state(e2::Key::A))
			{
				inputVector.x -= 1.0f;
			}
			if (kb.state(e2::Key::D))
			{
				inputVector.x += 1.0f;
			}
			if (kb.state(e2::Key::W))
			{
				inputVector.y -= 1.0f;
			}
			if (kb.state(e2::Key::S))
			{
				inputVector.y += 1.0f;
			}
		}

		entity->setInputVector(inputVector);
	}

	if (!entity->isCaptain())
	{
		e2::InventorySlot& slot = inventory[activeSlot];
		if (slot.item)
		{
			if (slot.item->wieldable && slot.item->wieldHandler)
			{
				slot.item->wieldHandler->onUpdate(entity, seconds);
			}
		}
	}

}

void e2::PlayerState::renderInventory(double seconds)
{
	e2::GameSession* session = entity->gameSession();
	e2::UIContext* ui = session->uiContext();

	e2::IWindow* wnd = session->window();
	e2::Renderer* renderer = session->renderer();

	glm::vec2 res = wnd->size();
	glm::vec2 iconSize{ 48.0f, 48.0f };
	float iconPadding{ 8.0f };
	glm::vec2 cursor{ res.x / 2.0f - (iconSize.x * 4.f) - (iconPadding * 4.5f), res.y - iconSize.y - iconPadding - 48.0f };

	for (uint32_t i = 0; i < 8; i++)
	{
		ui->drawGamePanel(cursor, iconSize, i == activeSlot, inventory[i].item ? 1.0f : 0.25f);


		if (inventory[i].item)
		{
			float textWidth = ui->calculateTextWidth(e2::FontFace::Sans, 9, inventory[i].item->displayName);
			ui->drawRasterText(e2::FontFace::Sans, 9, e2::UIColor(0xFFFFFFFF), cursor + glm::vec2(iconSize.x / 2.0f - textWidth / 2.0f, iconSize.y / 2.0f), inventory[i].item->displayName);

			if (inventory[i].count > 1)
			{
				float numWidth = ui->calculateTextWidth(e2::FontFace::Sans, 9, std::format("{}", inventory[i].count));
				ui->drawRasterText(e2::FontFace::Sans, 9, e2::UIColor(0xFFFFFFFF), cursor + glm::vec2(iconSize.x / 2.0f - textWidth / 2.0f, iconSize.y / 2.0f + 14.0f), std::format("{}", inventory[i].count));
			}
		}
		else
		{
			float textWidth = ui->calculateTextWidth(e2::FontFace::Sans, 9, "empty");
			ui->drawRasterText(e2::FontFace::Sans, 9, e2::UIColor(0xFFFFFFFF), cursor + glm::vec2(iconSize.x / 2.0f - textWidth / 2.0f, iconSize.y / 2.0f), "empty");
		}

		cursor.x += iconSize.x + iconPadding;

	}

	e2::InventorySlot& slot = inventory[activeSlot];
	if (slot.item)
	{
		if (slot.item->wieldable && slot.item->wieldHandler)
		{
			slot.item->wieldHandler->onRenderInventory(entity, seconds);
		}
	}

}



e2::WieldHandler::~WieldHandler()
{
}

void e2::WieldHandler::onNuke()
{
}

void e2::WieldHandler::onSetup()
{
}

e2::HatchetHandler::~HatchetHandler()
{

}

void e2::HatchetHandler::onNuke()
{
	e2::WieldHandler::onNuke();
}

void e2::HatchetHandler::onSetup()
{
	e2::WieldHandler::onSetup();
}

void e2::HatchetHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
		m_meshSpecification.populate(obj.at("mesh"), deps);

	if (obj.contains("axeSwing"))
		m_axeSwingSpecification.populate(obj.at("axeSwing"), deps);

	if (obj.contains("woodChop"))
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


		player->game()->hexGrid()->grassCutMask().push({ planarPosition + playerToMouse * 0.2f , 0.5f });

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

void e2::SwordHandler::onNuke()
{
	e2::WieldHandler::onNuke();
}

void e2::SwordHandler::onSetup()
{
	e2::WieldHandler::onSetup();
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

		player->game()->hexGrid()->grassCutMask().push({ planarPosition + playerToMouse * 0.2f , 0.5f });
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



e2::ThrowableHandler::~ThrowableHandler()
{
}

void e2::ThrowableHandler::onNuke()
{
	e2::WieldHandler::onNuke();
}

void e2::ThrowableHandler::onSetup()
{
	e2::WieldHandler::onSetup();
}

void e2::ThrowableHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	std::string entityName = obj.at("entity").template get<std::string>();
	if (obj.contains("entity"))
		m_entityName = entityName;

}

void e2::ThrowableHandler::finalize(e2::GameContext* ctx)
{
}

void e2::ThrowableHandler::setActive(e2::PlayerEntity* player)
{
}

void e2::ThrowableHandler::setInactive(e2::PlayerEntity* player)
{
}

void e2::ThrowableHandler::onUpdate(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::PlayerState& playerState = game->playerState();

	e2::TileData cursorTile = game->hexGrid()->getTileData(game->cursorHex());

	if (player->aiming() && mouse.buttonPressed(e2::MouseButton::Left))
	{
		e2::Entity* newEntity = game->spawnEntity(m_entityName, player->getTransform()->getTranslation(e2::TransformSpace::World) + e2::worldUpf() * 0.5f + player->getTransform()->getForward(e2::TransformSpace::World) * 0.25f);
		e2::ThrowableEntity* asThrowable = newEntity->cast<e2::ThrowableEntity>();
		if (asThrowable)
		{
			asThrowable->setVelocity(player->getTransform()->getForward(e2::TransformSpace::World) * 1.0f);
			playerState.take(playerState.activeSlot, 1);
		}
	}
}

void e2::ThrowableHandler::onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName)
{
}




e2::IonizerHandler::~IonizerHandler()
{
}

void e2::IonizerHandler::onNuke()
{
	m_previewEntity = nullptr;
	e2::WieldHandler::onNuke();
}

void e2::IonizerHandler::onSetup()
{
	e2::WieldHandler::onSetup();
}

void e2::IonizerHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
		m_meshSpecification.populate(obj.at("mesh"), deps);
}

void e2::IonizerHandler::finalize(e2::GameContext* ctx)
{
	m_meshSpecification.finalize(ctx);
}

void e2::IonizerHandler::setActive(e2::PlayerEntity* player)
{
	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_meshSpecification, player);
}

void e2::IonizerHandler::setInactive(e2::PlayerEntity* player)
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_previewEntity)
	{
		player->game()->destroyEntity(m_previewEntity);
		m_previewEntity = nullptr;
	}

	m_mesh = nullptr;
}

void e2::IonizerHandler::onUpdate(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::TileData cursorTile = game->hexGrid()->getTileData(game->cursorHex());

	e2::RadionManager* rm = game->radionManager();
	int32_t numEntities = rm->numDiscoveredEntities();


	glm::vec2 cursorPlane = game->cursorPlane();
	glm::vec3 cursorWorld{ cursorPlane.x, 0.0f, cursorPlane.y };

	e2::RadionEntity* hoveredEntity{};
	float hoveredDistance{9999.0f};
	m_collisions.clear();
	game->populateCollisions(game->cursorHex(), e2::CollisionType::Component, m_collisions, true);

	for (e2::Collision& col : m_collisions)
	{
		e2::RadionEntity* currEntity = col.component->entity()->cast<e2::RadionEntity>();
		if (!currEntity)
			continue;

		glm::vec2 currCoords = currEntity->planarCoords();
		float currDistance = glm::distance(currCoords, cursorPlane);

		if (currDistance < hoveredDistance && currDistance < currEntity->radionSpecification->collision.radius)
		{
			hoveredDistance = currDistance;
			hoveredEntity = currEntity;
		}
	}

	if (!m_holding)
	{
		if (glm::abs(mouse.scrollOffset) > 0.0f && numEntities > 0)
		{
			int32_t scroll = glm::ceil(mouse.scrollOffset);
			m_currentEntityIndex += scroll;
			m_currentEntityIndex = glm::clamp(m_currentEntityIndex, 0, numEntities - 1);
		}

		if (numEntities == 0)
		{
			m_currentEntityIndex = 0;
		}

		if (mouse.buttonState(e2::MouseButton::Left))
		{
			m_holding = true;
		}
	}
	else if(numEntities > 0)
	{
		if (!mouse.buttonState(e2::MouseButton::Left))
		{
			e2::Name spawnId = rm->discoveredEntity(m_currentEntityIndex);
			e2::RadionEntitySpecification *spec = game->getEntitySpecification(spawnId)->cast<e2::RadionEntitySpecification>();
			if (!spec)
			{
				LogError("Invalid radion entity specification");
			}
			else
			{
				if (game->playerState().takeById("radion_cube", spec->radionCost))
				{
					game->spawnEntity( 
						spawnId,
						m_previewEntity->getTransform()->getTranslation(e2::TransformSpace::World),
						m_previewEntity->getTransform()->getRotation(e2::TransformSpace::World)
					);
				}
			}


			m_holding = false;
		}
	}



	if (((m_previewIndex != m_currentEntityIndex) || !m_previewEntity )  && numEntities > 0)
	{
		if (m_previewEntity)
		{
			game->destroyEntity(m_previewEntity);
			m_previewEntity = nullptr;
		}

		m_previewIndex = m_currentEntityIndex;


		m_previewEntity = game->spawnCustomEntity(rm->discoveredEntity(m_previewIndex), "e2::RadionPreviewEntity", cursorWorld)->cast<e2::RadionPreviewEntity>();
		m_previewEntity->mesh->updateVisibility();
		m_previewEntity->mesh->applyTransform();
	}


	if (m_holding)
	{
		if (m_previewEntity)
		{
			glm::vec2 previewPlane = m_previewEntity->planarCoords();

			float angle = e2::radiansBetween(cursorPlane, previewPlane);
			glm::quat rot = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
			m_previewEntity->getTransform()->setRotation(rot, e2::TransformSpace::World);
			m_previewEntity->mesh->updateVisibility();
			m_previewEntity->mesh->applyTransform();
		}
	}
	else
	{
		if (m_previewEntity)
		{
			m_previewEntity->getTransform()->setTranslation(cursorWorld, e2::TransformSpace::World);
			m_previewEntity->mesh->updateVisibility();
			m_previewEntity->mesh->applyTransform();
		}
	}

	if (hoveredEntity && mouse.buttonPressed(e2::MouseButton::Right))
	{
		for (uint32_t i = 0; i < hoveredEntity->radionSpecification->radionCost; i++)
		{
			e2::Entity *cube = game->spawnEntity("radion_cube", hoveredEntity->getTransform()->getTranslation(e2::TransformSpace::World));
			e2::ItemEntity* cubeAsItem = cube->cast<e2::ItemEntity>();
			cubeAsItem->setLifetime(4.0);
		}
		game->destroyEntity(hoveredEntity);
	}

	//if (player->aiming() && mouse.buttonPressed(e2::MouseButton::Left) && cursorTile.isLand())
	//{
	//	player->getMesh()->playAction("chop");
	//	// todo place 
	//}

	glm::mat4 playerTransform = player->getTransform()->getTransformMatrix(e2::TransformSpace::World);
	glm::mat4 playerScale = player->getMesh()->getScaleTransform();
	glm::mat4 itemScale = glm::scale(glm::identity<glm::mat4>(), glm::vec3(0.8f));
	glm::mat4 handTransform = player->getMesh()->boneTransform("hand_r");
	glm::mat4 boneCalibration = glm::identity<glm::mat4>();
	boneCalibration = glm::rotate(boneCalibration, glm::radians(180.0f), e2::worldUpf());
	boneCalibration = glm::rotate(boneCalibration, glm::radians(90.0f), e2::worldRightf());
	m_mesh->applyCustomTransform(playerTransform * playerScale * handTransform * glm::inverse(playerScale) * itemScale * boneCalibration);
}

void e2::IonizerHandler::onRenderInventory(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::RadionManager* rm = game->radionManager();
	int32_t numDiscovered = rm->numDiscoveredEntities();
	if (numDiscovered == 0)
	{
		return;
	}

	e2::IWindow* wnd = session->window();
	e2::Renderer* renderer = session->renderer();

	glm::vec2 res = wnd->size();
	glm::vec2 iconSize{ 48.0f, 48.0f };
	float iconPadding{ 8.0f };
	glm::vec2 cursor{ res.x / 2.0f - (iconSize.x * 4.f) - (iconPadding * 4.5f), res.y - iconSize.y - iconPadding - 48.0f - iconSize.y - iconPadding };

	int32_t startIndex = m_currentEntityIndex - 3;
	int32_t endIndex = m_currentEntityIndex + 4;
	for (int32_t i = startIndex; i < endIndex; i++)
	{
		if (i < 0 || i >= numDiscovered)
		{
			cursor.x += iconSize.x + iconPadding;
			continue;
		}
			

		ui->drawGamePanel(cursor, iconSize, i == m_currentEntityIndex, 1.0f);

		e2::Name currentName = rm->discoveredEntity(i);


		float textWidth = ui->calculateTextWidth(e2::FontFace::Sans, 6, currentName.string());
		ui->drawRasterText(e2::FontFace::Sans, 6, e2::UIColor(0xFFFFFFFF), cursor + glm::vec2(iconSize.x / 2.0f - textWidth / 2.0f, iconSize.y / 2.0f), currentName.string());

		cursor.x += iconSize.x + iconPadding;

	}


}

void e2::IonizerHandler::onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName)
{
}

e2::LinkerHandler::~LinkerHandler()
{
}

void e2::LinkerHandler::onNuke()
{
	
	e2::WieldHandler::onNuke();
}

void e2::LinkerHandler::onSetup()
{
	e2::WieldHandler::onSetup();
}

void e2::LinkerHandler::populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
		m_meshSpecification.populate(obj.at("mesh"), deps);
}

void e2::LinkerHandler::finalize(e2::GameContext* ctx)
{
	m_meshSpecification.finalize(ctx);
}

void e2::LinkerHandler::setActive(e2::PlayerEntity* player)
{
	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_meshSpecification, player);
}

void e2::LinkerHandler::setInactive(e2::PlayerEntity* player)
{
	if (m_mesh)
		e2::destroy(m_mesh);

	m_mesh = nullptr;
}

void e2::LinkerHandler::onUpdate(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::TileData cursorTile = game->hexGrid()->getTileData(game->cursorHex());

	e2::RadionManager* rm = game->radionManager();
	int32_t numEntities = rm->numDiscoveredEntities();


	e2::IWindow* wnd = session->window();
	e2::Renderer* renderer = session->renderer();
	glm::vec2 cursorPlane = game->cursorPlane();
	glm::vec3 cursorWorld{ cursorPlane.x, 0.0f, cursorPlane.y };


	m_worldPins.clear();
	rm->populatePins(game->cursorHex(), m_connecting ? e2::RadionPinType::Input : e2::RadionPinType::Output, m_worldPins);

	glm::vec2 mousePos = mouse.relativePosition;

	m_hoveredPin = nullptr;
	float hoveredDistance = 9999999.9f;
	for (e2::RadionWorldPin& pin : m_worldPins)
	{
		glm::vec2 pinScreen = renderer->view().unprojectWorld(renderer->resolution(), pin.worldOffset);

		float dist = glm::distance(pinScreen, mousePos);
		if (dist < hoveredDistance && dist < 6.0f)
		{
			hoveredDistance = dist;
			m_hoveredPin = &pin;
		}


		
	}


	if (m_hoveredPin && mouse.buttonPressed(e2::MouseButton::Left))
	{
		if (m_connecting)
		{
			m_connectingPin.entity->connectOutputPin(m_connectingPin.name, m_hoveredPin->entity, m_hoveredPin->name);
		}
		else
		{
			m_connectingPin = *m_hoveredPin;
		}
		m_connecting = !m_connecting;
	}

	if (!m_connecting && m_hoveredPin && mouse.buttonPressed(e2::MouseButton::Right))
	{
		m_hoveredPin->entity->disconnectOutputPin(m_hoveredPin->name);
	}


}

void e2::LinkerHandler::onRenderInventory(e2::PlayerEntity* player, double seconds)
{
	e2::Game* game = player->game();
	e2::GameSession* session = game->gameSession();
	e2::UIContext* ui = session->uiContext();
	e2::UIKeyboardState& kb = ui->keyboardState();
	e2::UIMouseState& mouse = ui->mouseState();

	e2::RadionManager* rm = game->radionManager();

	e2::IWindow* wnd = session->window();
	e2::Renderer* renderer = session->renderer();

	glm::vec2 res = wnd->size();

	for (e2::RadionWorldPin& pin : m_worldPins)
	{
		glm::vec2 pinScreen = renderer->view().unprojectWorld(renderer->resolution(), pin.worldOffset);
		ui->drawRasterText(e2::FontFace::Monospace, 9, &pin == m_hoveredPin ? e2::UIColor(0x00FF00FF) :  e2::UIColor::white(), pinScreen, pin.name.string());

		session->renderer()->debugSphere(&pin == m_hoveredPin ? glm::vec3(0.0, 1.0, 0.0) : (m_connecting ? glm::vec3(1.0, 0.0, 1.0) : glm::vec3(0.0, 1.0, 1.0)), pin.worldOffset, 0.05f);
	}

}

void e2::LinkerHandler::onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName)
{
}
