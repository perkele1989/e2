
#include "game/entities/player.hpp"

#include "game/game.hpp"

#include <e2/game/gamesession.hpp>
#include <e2/e2.hpp>

#include <e2/utils.hpp>

e2::PlayerSpecification::PlayerSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::PlayerEntity");
}

e2::PlayerSpecification::~PlayerSpecification()
{
}

void e2::PlayerSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	if (obj.contains("movement"))
		movement.populate(obj.at("movement"), assets);

	if(obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);

	if (obj.contains("boatMesh"))
		boatMesh.populate(obj.at("boatMesh"), assets);

	if (obj.contains("moveSpeed"))
		moveSpeed = obj.at("moveSpeed").template get<float>();

	if (obj.contains("footstepGrass"))
		footstepGrass.populate(obj.at("footstepGrass"), assets);

	if (obj.contains("footstepDesert"))
		footstepDesert.populate(obj.at("footstepDesert"), assets);

	if (obj.contains("footstepSnow"))
		footstepSnow.populate(obj.at("footstepSnow"), assets);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);
}

void e2::PlayerSpecification::finalize()
{
	mesh.finalize(game);
	boatMesh.finalize(game);
	footstepGrass.finalize(game);
	footstepDesert.finalize(game);
	footstepSnow.finalize(game);
	collision.finalize(game);
}

e2::PlayerEntity::PlayerEntity()
	: e2::Entity()
	, m_fog{this, 6, 1}
{
	m_targetRotation = glm::angleAxis(0.0f, e2::worldUpf());
}

e2::PlayerEntity::~PlayerEntity()
{
	if (m_boatMesh)
		e2::destroy(m_boatMesh);

	if (m_mesh)
		e2::destroy(m_mesh);

	if(m_footstepGrass)
		e2::destroy(m_footstepGrass);

	if(m_footstepDesert)
		e2::destroy(m_footstepDesert);

	if(m_footstepSnow)
		e2::destroy(m_footstepSnow);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);

}

void e2::PlayerEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition);
	m_playerSpecification = m_specification->cast<e2::PlayerSpecification>();

	m_mesh = e2::create<e2::SkeletalMeshComponent>(&m_playerSpecification->mesh, this);
	m_mesh->setTriggerListener(this);

	m_boatMesh = e2::create<e2::StaticMeshComponent>(&m_playerSpecification->boatMesh, this);
	m_boatMesh->meshProxy()->disable();

	m_footstepGrass = e2::create<e2::RandomAudioComponent>(&m_playerSpecification->footstepGrass, this);
	m_footstepDesert = e2::create<e2::RandomAudioComponent>(&m_playerSpecification->footstepDesert, this);
	m_footstepSnow = e2::create<e2::RandomAudioComponent>(&m_playerSpecification->footstepSnow, this);
	m_collision = e2::create<e2::CollisionComponent>(&m_playerSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_playerSpecification->movement, this);
}

void e2::PlayerEntity::writeForSave(e2::IStream& toBuffer)
{
}

void e2::PlayerEntity::readForSave(e2::IStream& fromBuffer)
{
}

void e2::PlayerEntity::updateAnimation(double seconds)
{
	// slerp the rotation towards target
	m_currentRotation = glm::slerp(m_currentRotation, m_targetRotation, glm::clamp(float(seconds) * 10.0f, 0.01f, 1.0f));
	m_transform->setRotation(m_currentRotation * m_tilt, e2::TransformSpace::World);

	m_mesh->updateAnimation(seconds);

	m_fog.refresh();
}

glm::vec2 rhash(glm::vec2 uv) {
	glm::mat2 myt = glm::mat2(.12121212f, .13131313f, -.13131313f, .12121212f);
	glm::vec2 mys = glm::vec2(1e4, 1e6);
	uv = uv * myt;
	uv = uv * mys;
	return glm::fract(glm::fract(uv / mys) * uv);
}

float voronoi2d(glm::vec2 const& point)
{
	glm::vec2 p = glm::floor(point);
	glm::vec2 f = glm::fract(point);
	float res = 0.0;
	for (int j = -1; j <= 1; j++) {
		for (int i = -1; i <= 1; i++) {
			glm::vec2 b = glm::vec2(i, j);
			glm::vec2 r = glm::vec2(b) - f + rhash(p + b);
			res += 1. / glm::pow(glm::dot(r, r), 8.f);
		}
	}
	return glm::pow(1. / res, 0.0625f);
}

static float sampleHeight(glm::vec2 position)
{
	return glm::pow(voronoi2d(position * 0.8f), 2.0f);
}

static float generateWaveHeight(
	glm::vec2 position,
	float time,
	float scale1,
	float scale2,
	float scale3,
	float variance,
	float varianceSpeed,
	float speed,
	glm::vec2 dir)
{
	float varianceOffset = 1.0 - variance;
	float waveCoeff = (sin(time * varianceSpeed) * 0.5f + 0.5f) * variance + varianceOffset;

	dir = normalize(dir);
	glm::vec2 pos = position + (dir * speed * time);
	return sampleHeight(pos * scale3 + scale1 * scale2) * waveCoeff;
}

static float sampleWaterHeight(glm::vec2 const& position, float time)
{
	float heightSmallOffset = generateWaveHeight(position, time,
		0.0f, 1.6f, 3.5f,
		0.2f, 1.0f,
		0.1f, glm::vec2(0.2f, 0.4f));

	float heightSmall = generateWaveHeight(position, time,
		heightSmallOffset * 0.314f, 1.0f, 2.5f,
		0.5f, 0.5f,
		0.15f, glm::vec2(-0.22f, -0.3f));

	float heightBigOffset = generateWaveHeight(position, time,
		0.0f, 1.6f, 1.5f,
		0.2f, 1.0f,
		0.2f, glm::vec2(0.2f, 0.4f));

	float heightBig = generateWaveHeight(position, time,
		heightBigOffset * 0.314f, 1.0f, 1.0f,
		0.5f, 0.5f,
		0.3f, glm::vec2(-0.22f, -0.3f));


	return heightSmall * 0.2 + heightBig * 0.8;
}

static float getWaterHeight(e2::GameContext* ctx, glm::vec2 const& position)
{
	constexpr float heightScale = 0.15f;
	e2::GameSession *session = ctx->game()->gameSession();
	e2::RendererData const& rendererData = session->renderer()->rendererData();

	float time = rendererData.time.x * 0.67f * 2.0f;
	float wh = sampleWaterHeight(position, time);
	float y = 0.0f;
	y -= wh * heightScale;
	y += 0.15f;
	return y;
}

void e2::PlayerEntity::update(double seconds)
{

	e2::UIContext* ui = gameSession()->uiContext();
	e2::UIMouseState& mouse = ui->mouseState();
	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), m_captain ? e2::CollisionType::Land |  e2::CollisionType::Component : e2::CollisionType::Mountain | e2::CollisionType::Tree | e2::CollisionType::Water | e2::CollisionType::Component, m_collisionCache);


	/*e2::TileData tile = game()->hexGrid()->getTileData(hex().offsetCoords());
	if (tile.isShallowWater() || tile.isDeepWater())
	{

	}
	else
	{

	}*/

	m_aiming = mouse.buttonState(e2::MouseButton::Right);

	if (m_captain)
	{
		updateWater(seconds);
		float height = getWaterHeight(game(), planarCoords());

		glm::vec3 worldCoords = m_transform->getTranslation(e2::TransformSpace::World);
		glm::vec3 bckCoords = worldCoords + (m_currentRotation * e2::worldForwardf() * -0.34104f * 0.5f);
		glm::vec3 fwdCoords = worldCoords + (m_currentRotation * e2::worldForwardf() * 0.34104f) * 0.5f;
		float dist = glm::distance(bckCoords, fwdCoords); // x
		float fwdHeight = getWaterHeight(game(), { fwdCoords.x, fwdCoords.z});
		float bckHeight = getWaterHeight(game(), { bckCoords.x, bckCoords.z });

		gameSession()->renderer()->debugSphere({ 0.0f, 1.0f, 0.0f }, { bckCoords.x, bckHeight, bckCoords.z }, 0.05f);
		gameSession()->renderer()->debugSphere({ 1.0f, 0.0f, 0.0f }, { fwdCoords.x, fwdHeight, fwdCoords.z }, 0.05f);
		
		glm::vec3 cA{ bckHeight, 0.0f, 0.0f};
		glm::vec3 cB{ fwdHeight, 0.0f, dist };

		float tilt = e2::radiansBetween(cA, cB); 
		m_tilt = glm::rotate(glm::identity<glm::quat>(), tilt, -e2::worldRightf());

		glm::mat4 heightOffset = glm::translate(glm::identity<glm::mat4>(), { 0.0f, height, 0.0f });
		
		m_mesh->setHeightOffset(height);
		m_boatMesh->setHeightOffset(height);
	}
	else
	{
		updateLand(seconds);
		m_mesh->setHeightOffset(0.0f);
	}

	m_collision->invalidate();

	m_mesh->applyTransform();
	m_boatMesh->applyTransform();

}

void e2::PlayerEntity::updateLand(double seconds)
{
	e2::UIContext* ui = gameSession()->uiContext();
	e2::UIMouseState& mouse = ui->mouseState();

	game()->setZoom(0.0f);

	if (mouse.buttonPressed(e2::MouseButton::Left) && !m_aiming)
	{
		game()->spawnEntity("teardrop", getTransform()->getTranslation(e2::TransformSpace::World));
	}

	glm::vec2 cursor = game()->cursorPlane();

	glm::vec2 planarPosition = planarCoords();

	float moveSpeed = m_playerSpecification->moveSpeed;
	/*if (m_aiming)
		moveSpeed *= 0.5f;*/

	m_lastPosition = planarPosition;
	if (glm::length(m_inputVector) > 0.01f)
	{
		m_inputVector = glm::normalize(m_inputVector);

		m_movement->move(m_collision->radius(), m_inputVector, float(seconds) * moveSpeed, e2::CollisionType::Mountain | e2::CollisionType::Tree | e2::CollisionType::Water, m_collisionCache);
	}

	planarPosition = planarCoords();

	glm::vec2 moveOffset = planarPosition - m_lastPosition;
	float distance = glm::length(moveOffset);
	float speed = distance / float(seconds);
	if (speed >= 0.3f)
	{
		glm::vec2 moveDirection = glm::normalize(moveOffset);
		if (!m_aiming)
		{
			float angle = e2::radiansBetween(glm::vec3(planarPosition.x, 0.0f, planarPosition.y) + glm::vec3(m_inputVector.x, 0.0f, m_inputVector.y), glm::vec3(planarPosition.x, 0.0f, planarPosition.y));
			m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
		}
		m_mesh->setPose("run");
		m_mesh->setPoseSpeed(glm::clamp(speed / m_playerSpecification->moveSpeed, 0.5f, 1.0f));
	}
	else
	{
		m_mesh->setPose("idle");
		m_mesh->setPoseSpeed(1.0);
	}


	if (m_aiming)
	{
		float angle = e2::radiansBetween(glm::vec3(cursor.x, 0.0f, cursor.y), glm::vec3(planarPosition.x, 0.0f, planarPosition.y));
		m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));

		if (!m_mesh->isAnyActionPlaying() && mouse.buttonPressed(e2::MouseButton::Left))
		{
			e2::TileData cursorTile = game()->hexGrid()->getTileData(game()->cursorHex());
			if (cursorTile.isDeepWater() || cursorTile.isShallowWater())
			{
				glm::vec2 cursorPlane = game()->cursorPlane();
				if (glm::distance(cursorPlane, planarPosition) < 2.0f)
				{
					m_captain = true;
					m_boatMesh->meshProxy()->enable();
					m_collision->setRadius(0.3f);
					m_boatVelocity = 0.0f;
					m_fog.setRange(8, 1);
					glm::vec3 translation = getTransform()->getTranslation(e2::TransformSpace::World);
					getTransform()->setTranslation({ cursorPlane.x, translation.y, cursorPlane.y }, e2::TransformSpace::World);
				}

			}
		}
	}

}

void e2::PlayerEntity::updateWater(double seconds)
{
	e2::UIContext* ui = gameSession()->uiContext();
	e2::UIMouseState& mouse = ui->mouseState();

	m_mesh->setPose("boat");

	glm::vec2 planarPosition = planarCoords();

	constexpr float boatMoveSpeed = 2.0f;
	constexpr float boatAccel = 0.56f;
	constexpr float boatDeccel = 1.5f;
	constexpr float boatTurnBrake = 0.7f;
	constexpr float boatTurnSpeedMin = 0.4f;
	constexpr float boatTurnSpeedMax = 3.0f;

	game()->setZoom(0.15f);

	m_lastPosition = planarPosition;
	if (glm::length(m_inputVector) > 0.01f)
	{

		m_inputVector = glm::normalize(m_inputVector);
		float angle = e2::radiansBetween(glm::vec3(planarPosition.x, 0.0f, planarPosition.y) + glm::vec3(m_inputVector.x, 0.0f, m_inputVector.y), glm::vec3(planarPosition.x, 0.0f, planarPosition.y));

		glm::vec3 boatFwd = m_targetRotation * e2::worldForwardf();
		glm::vec3 inputWrld = { m_inputVector.x, 0.0f, m_inputVector.y };

		if (glm::dot(boatFwd, inputWrld) < 0.0f)
		{
			m_boatVelocity -= seconds * boatTurnBrake;
		}
		else
		{
			m_boatVelocity += seconds * boatAccel;
		}
		m_boatVelocity = glm::clamp(m_boatVelocity, 0.0f, boatMoveSpeed);


		float rotateSpeed = glm::mix(boatTurnSpeedMin, boatTurnSpeedMax, m_boatVelocity / boatMoveSpeed);
		m_targetRotation = glm::slerp(
			m_targetRotation,
			glm::angleAxis(angle, glm::vec3(e2::worldUp())),
			glm::min(0.01f, float(seconds) * rotateSpeed)
		);

	}
	else
	{
		m_boatVelocity -= seconds * boatDeccel;
		m_boatVelocity = glm::clamp(m_boatVelocity, 0.0f, boatMoveSpeed);
	}




	float moveDist = float(seconds) * m_boatVelocity;
	if (moveDist > 0.0f)
	{
		glm::vec3 boatFwd = m_targetRotation * e2::worldForwardf();

		glm::vec3 preMove = getTransform()->getTranslation(e2::TransformSpace::World);

		float energyLeft = m_movement->move(m_collision->radius(), { boatFwd.x, boatFwd.z }, moveDist, e2::CollisionType::Land, m_collisionCache);
		glm::vec3 postMove = getTransform()->getTranslation(e2::TransformSpace::World);

		float expendedFactor = 1.0f - (energyLeft / moveDist);
		if (expendedFactor < 1.0f)
		{
			m_boatVelocity = m_boatVelocity * expendedFactor;
		}
	}

	if (m_aiming)
	{
		if (mouse.buttonPressed(e2::MouseButton::Left))
		{
			e2::TileData cursorTile = game()->hexGrid()->getTileData(game()->cursorHex());
			if (cursorTile.isLand() && !cursorTile.isMountain())
			{
				glm::vec2 cursorPlane = game()->cursorPlane();
				if (glm::distance(cursorPlane, planarPosition) < 2.0f)
				{
					m_captain = false;
					m_collision->setRadius(0.05f);
					m_fog.setRange(6, 1);
					m_boatMesh->meshProxy()->disable();
					glm::vec3 translation = getTransform()->getTranslation(e2::TransformSpace::World);
					getTransform()->setTranslation({ cursorPlane.x, translation.y, cursorPlane.y }, e2::TransformSpace::World);
				}

			}
		}
	}

}

void e2::PlayerEntity::setInputVector(glm::vec2 const& inputVector)
{
	m_inputVector = inputVector;
}

namespace
{
	std::vector<e2::Hex> tmpHex;
	void cleanTmpHex()
	{
		tmpHex.clear();
		tmpHex.reserve(16);
	}
}

void e2::PlayerEntity::onTrigger(e2::Name action, e2::Name trigger)
{
	static e2::Name const triggerStep1 = "step1";
	static e2::Name const triggerStep2 = "step2";

	if (trigger == triggerStep1 || trigger == triggerStep2)
	{
		e2::TileData tile = game()->hexGrid()->getTileData(hex().offsetCoords());
		e2::TileFlags biome = tile.getBiome();
		if(biome == e2::TileFlags::BiomeGrassland)
			m_footstepGrass->play(0.15f);
		else if (biome == e2::TileFlags::BiomeDesert)
			m_footstepDesert->play(0.15f);
		else if (biome == e2::TileFlags::BiomeTundra)
			m_footstepSnow->play(0.15f);
	}

	e2::PlayerState& playerState = game()->playerState();
	e2::InventorySlot& activeSlot = playerState.inventory[playerState.activeSlot];
	if (activeSlot.item && activeSlot.item->wieldable && activeSlot.item->wieldHandler)
	{
		activeSlot.item->wieldHandler->onTrigger(this, action, trigger);
	}

}

void e2::PlayerEntity::onHitEntity(e2::Entity* otherEntity)
{
	e2::ItemEntity* item = otherEntity->cast<e2::ItemEntity>();
	e2::ItemSpecification* itemSpec = otherEntity->getSpecificationAs<e2::ItemSpecification>();
	if (item && itemSpec)
	{
		if(item->getLifetime() > 4.0 && game()->playerState().give(itemSpec->id))
			game()->queueDestroyEntity(otherEntity);
	}
}

void e2::PlayerEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}








































e2::TeardropSpecification::TeardropSpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::TeardropEntity");
}

e2::TeardropSpecification::~TeardropSpecification()
{
}

void e2::TeardropSpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	assets.insert("S_Teardrop_Jump.e2a");
	assets.insert("S_Teardrop_Land.e2a");
	assets.insert("S_Teardrop_Hit.e2a");
	assets.insert("S_Teardrop_Die.e2a");

	if (obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);

	if (obj.contains("movement"))
		movement.populate(obj.at("movement"), assets);
}

void e2::TeardropSpecification::finalize()
{
	mesh.finalize(game);
	collision.finalize(game);

}

e2::TeardropEntity::TeardropEntity()
	: e2::Entity()
{
	m_targetRotation = glm::angleAxis(0.0f, e2::worldUpf());
}

e2::TeardropEntity::~TeardropEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

	if (m_movement)
		e2::destroy(m_movement);

}

void e2::TeardropEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition);
	m_teardropSpecification = m_specification->cast<e2::TeardropSpecification>();

	m_mesh = e2::create<e2::SkeletalMeshComponent>(&m_teardropSpecification->mesh, this);
	m_mesh->setTriggerListener(this);


	m_collision = e2::create<e2::CollisionComponent>(&m_teardropSpecification->collision, this);
	m_movement = e2::create<e2::MovementComponent>(&m_teardropSpecification->movement, this);
}

void e2::TeardropEntity::writeForSave(e2::IStream& toBuffer)
{
}

void e2::TeardropEntity::readForSave(e2::IStream& fromBuffer)
{
}

void e2::TeardropEntity::updateAnimation(double seconds)
{
	glm::quat currentRotation = m_transform->getRotation(e2::TransformSpace::World);
	currentRotation = glm::slerp(currentRotation, m_targetRotation, glm::clamp(float(seconds) * 10.0f, 0.01f, 1.0f));
	m_transform->setRotation(currentRotation, e2::TransformSpace::World);

	m_mesh->updateAnimation(seconds);
}

void e2::TeardropEntity::update(double seconds)
{
	m_collisionCache.clear();
	game()->populateCollisions(hex().offsetCoords(), e2::CollisionType::All, m_collisionCache);

	e2::PlayerEntity* player = game()->playerEntity();
	glm::vec2 playerCoords = player->planarCoords();
	glm::vec2 thisCoords = planarCoords();

	constexpr float jumpInterval = 3.0f;
	constexpr float jumpTime = 0.5f;
	constexpr float rotateTime = 2.3f;
	constexpr float jumpHeight = 0.3f;

	if (m_mesh->isActionPlaying("die") || m_health <= 0.0f)
	{

	}
	else if (m_mesh->isActionPlaying("hit"))
	{
		m_movement->move(m_collision->radius(), m_hitDirection, float(seconds) * 1.0f, e2::CollisionType::All, m_collisionCache);
		float angle = e2::radiansBetween(glm::vec3(thisCoords.x, 0.0f, thisCoords.y) - glm::vec3(m_hitDirection.x, 0.0f, m_hitDirection.y), glm::vec3(thisCoords.x, 0.0f, thisCoords.y));
		m_targetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
		m_jumping = false;
		m_timeSinceJump = 0.0f;
	}
	else if (m_jumping)
	{
		m_mesh->setPose("air");

		m_jumpFactor += float(seconds);

		float jumpCoefficient = glm::clamp(m_jumpFactor / jumpTime, 0.0f, 1.0f);
		float heightCoefficient = glm::sin(jumpCoefficient * glm::pi<float>());


		m_mesh->setHeightOffset(heightCoefficient * -jumpHeight);

		glm::vec3 fwd = m_targetRotation * e2::worldForwardf();
		
		m_movement->move(m_collision->radius(), glm::normalize(glm::vec2{fwd.x, fwd.z}), float(seconds) * 1.0f, e2::CollisionType::All, m_collisionCache);

		if (m_jumpFactor >= jumpTime)
		{
			m_jumping = false;
			m_timeSinceJump = 0.0f;

			m_mesh->setPose("idle");
			m_mesh->playAction("land");
			playSound("S_Teardrop_Land.e2a", 1.0f, 1.0f);
			
		}
	}
	else
	{
		m_movement->resolve(m_collision->radius(), e2::CollisionType::All, m_collisionCache);

		m_mesh->setHeightOffset(0.0f);
		if (!m_mesh->isActionPlaying("jump"))
		{
			m_mesh->setPose("idle");
			float oldTimeSinceJump = m_timeSinceJump;
			m_timeSinceJump += float(seconds);

			if (oldTimeSinceJump < rotateTime && m_timeSinceJump >= rotateTime)
			{
				m_targetRotation = glm::angleAxis(glm::radians(e2::randomFloat(0.0f, 360.0f)), e2::worldUpf());
			}

			if (m_timeSinceJump > jumpInterval)
			{
				m_mesh->playAction("jump");
			}
		}
	}

	
	m_collision->invalidate();
	m_mesh->applyTransform();
	


}

void e2::TeardropEntity::onTrigger(e2::Name action, e2::Name trigger)
{
	static const e2::Name triggerJump{ "jump" };
	if (trigger == triggerJump)
	{
		m_jumping = true;
		m_jumpFactor = 0.0f;
	}

	static const e2::Name triggerJumpSound{ "jump_sound"};
	if (trigger == triggerJumpSound)
	{
		playSound("S_Teardrop_Jump.e2a", 1.0f, 1.0f);
	}
}

void e2::TeardropEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::TeardropEntity::onMeleeDamage(e2::Entity* instigator, float dmg)
{
	if (m_health <= 0.0f)
		return;

	glm::vec2 pos = planarCoords();

	if (m_jumping)
	{
		game()->spawnHitLabel({ pos.x, -0.25f, pos.y }, "Miss!");
		return;
	}

	
	glm::vec2 instPos = instigator->planarCoords();
	m_hitDirection = glm::normalize(pos - instPos);

	game()->spawnHitLabel({ pos.x, -0.25f, pos.y }, std::format("{}", (uint32_t)dmg));
	m_health -= dmg;
	if (m_health <= 0.0f)
	{
		playSound("S_Teardrop_Die.e2a", 1.0, 0.5);
		m_mesh->playAction("die");
		game()->queueDestroyEntity(this);
	}
	else
	{
		playSound("S_Teardrop_Hit.e2a", 1.0, 1.0);
		m_mesh->playAction("hit");
	}
}

bool e2::TeardropEntity::canBeDestroyed()
{
	return !m_mesh->isActionPlaying("die");
}
