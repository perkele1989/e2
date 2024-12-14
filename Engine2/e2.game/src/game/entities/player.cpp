
#include "game/entities/player.hpp"

#include "game/game.hpp"
#include "game/entities/itementity.hpp"

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

void e2::PlayerEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
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

void e2::PlayerEntity::beCaptain()
{
	m_captain = true;
	m_boatMesh->meshProxy()->enable();
	m_collision->setRadius(0.3f);
	m_boatVelocity = 0.0f;
	m_fog.setRange(8, 1);
}

void e2::PlayerEntity::beLandcrab()
{
	m_captain = false;
	m_collision->setRadius(0.05f);
	m_fog.setRange(6, 1);
	m_boatMesh->meshProxy()->disable();
}

void e2::PlayerEntity::renderUi()
{
	auto ui = gameSession()->uiContext();
	if (m_hintText.size() > 0)
	{
		ui->drawRasterTextShadow(e2::FontFace::Sans, 12, m_hintTextPos, m_hintText);
		ui->drawRasterText(e2::FontFace::Sans, 12, e2::UIColor::white(), m_hintTextPos, m_hintText);
	}
		
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
	return glm::pow(voronoi2d(position * (0.8f/0.75f)), 2.0f);
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
	time *= 0.75f;
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
	game()->populateCollisions(hex().offsetCoords(), m_captain ? e2::CollisionType::Land |  e2::CollisionType::Component | e2::CollisionType::Mountain | e2::CollisionType::Tree : e2::CollisionType::Mountain | e2::CollisionType::Tree | e2::CollisionType::Water | e2::CollisionType::Component, m_collisionCache);



	/*e2::TileData tile = game()->hexGrid()->getTileData(hex().offsetCoords());
	if (tile.isShallowWater() || tile.isDeepWater())
	{

	}
	else
	{

	}*/

	m_aiming = !game()->scriptRunning() && mouse.buttonState(e2::MouseButton::Right);

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


	std::set<e2::Entity*, e2::RadialCompare> interactables; // @todo fix alloc

	e2::Transform* playerTransform = getTransform();
	glm::vec3 playerFwd = playerTransform->getForward(e2::TransformSpace::World);
	glm::vec3 playerPosition = playerTransform->getTranslation(e2::TransformSpace::World);
	glm::vec2 playerCoords{ playerPosition.x, playerPosition.z };
	glm::vec2 playerFwdPlanar = glm::normalize(glm::vec2{ playerFwd.x, playerFwd.z });

	bool overWater = false;
	glm::vec2 waterPlanar{};

	bool blocksLand = false;
	bool foundLand = false;
	glm::vec2 landPlanar = playerCoords + playerFwdPlanar * 0.5f;
	for (e2::Collision& collision : m_collisionCache)
	{
		if (collision.type == e2::CollisionType::Component && collision.component)
		{
			e2::Entity* otherEntity = collision.component->entity();
			e2::ItemEntity* item = otherEntity->cast<e2::ItemEntity>();
			e2::ItemSpecification* itemSpec = otherEntity->getSpecificationAs<e2::ItemSpecification>();
			if (item && itemSpec)
			{
				if (item->getCooldown() <= 0.0 && glm::distance(planarCoords(), item->planarCoords()) < 0.3f && game()->playerState().give(itemSpec->id))
					game()->queueDestroyEntity(item);
			}
			else
			{
				if (otherEntity->interactable() && !otherEntity->pendingDestroy)
				{

					glm::vec2 playerToEntity = otherEntity->planarCoords() - planarCoords();
					glm::vec2 playerToEntityDir = glm::normalize(playerToEntity);
					float playerToEntityDist = glm::length(playerToEntity);

					if (glm::dot(playerToEntityDir, playerFwdPlanar) > glm::cos(glm::radians(45.0f)) && playerToEntityDist <= 0.5f)
					{
						interactables.insert(otherEntity);
					}
				}
			}
		}

		if (collision.type == e2::CollisionType::Water)
		{
			glm::vec2 potentiallyWaterA = playerCoords + playerFwdPlanar * 0.2f;
			glm::vec2 potentiallyWaterB = playerCoords + playerFwdPlanar * 0.7f;
			bool overlapsA = e2::circleOverlapTest(potentiallyWaterA, 0.02, collision.position, collision.radius).overlaps;
			bool overlapsB = e2::circleOverlapTest(potentiallyWaterB, 0.02, collision.position, collision.radius).overlaps;
			if (overlapsA && overlapsB)
			{
				overWater = true;
				waterPlanar = potentiallyWaterB;
			}
		}

		bool overlapsThis = e2::circleOverlapTest(landPlanar, 0.02, collision.position, collision.radius).overlaps;
		if (collision.type == e2::CollisionType::Land && overlapsThis)
		{
			foundLand = true;
		}


		if (overlapsThis && collision.type != e2::CollisionType::Land)
		{
			blocksLand = true;
		}
	}
	m_interactable = interactables.empty() ? nullptr : *interactables.begin();

	auto renderer = gameSession()->renderer();
	glm::vec3 translation = getTransform()->getTranslation(e2::TransformSpace::World);
	glm::vec2 uiPos = renderer->view().unprojectWorld(renderer->resolution(), translation + e2::worldUpf() * 0.25f);

	if (!game()->scriptRunning() && m_interactable)
	{
		m_hintText = std::format("*{}*\n**{}** (E)", m_interactable->getSpecification()->displayName, m_interactable->interactText());
		m_hintTextPos = renderer->view().unprojectWorld(renderer->resolution(), m_interactable->getTransform()->getTranslation(e2::TransformSpace::World) + e2::worldUpf() * 0.2f);

		if (ui->keyboardState().pressed(e2::Key::E))
		{
			m_interactable->onInteract(this);
		}
	}
	else if (!m_interactable && !m_captain && overWater)
	{
		m_hintText = "*Boat*\n**Embark** (E)";
		m_hintTextPos = uiPos;// renderer->view().unprojectWorld(renderer->resolution(), glm::vec3{ waterPlanar.x, 0.0f, waterPlanar.y } + e2::worldUpf() * 0.1f);

		if (ui->keyboardState().pressed(e2::Key::E))
		{
			beCaptain();

			getTransform()->setTranslation({ waterPlanar.x, translation.y, waterPlanar.y }, e2::TransformSpace::World);
		}
	}
	else if (!m_interactable && m_captain && !blocksLand && foundLand)
	{
		m_hintText = "*Boat*\n**Disembark** (E)";
		m_hintTextPos = uiPos;

		if (ui->keyboardState().pressed(e2::Key::E))
		{
			beLandcrab();

			getTransform()->setTranslation({ landPlanar.x, translation.y, landPlanar.y }, e2::TransformSpace::World);
		}
	}
	else
	{
		m_hintText.clear();
	}
}

void e2::PlayerEntity::updateLand(double seconds)
{
	e2::UIContext* ui = gameSession()->uiContext();
	e2::UIMouseState& mouse = ui->mouseState();

	game()->setZoom(0.0f);

	//if (mouse.buttonPressed(e2::MouseButton::Left) && !m_aiming)
	//{
	//	game()->spawnEntity("teardrop", getTransform()->getTranslation(e2::TransformSpace::World));
	//}

	glm::vec2 cursor = game()->cursorPlane();

	glm::vec2 planarPosition = planarCoords();

	float moveSpeed = m_playerSpecification->moveSpeed;
	/*if (m_aiming)
		moveSpeed *= 0.5f;*/

	m_lastPosition = planarPosition;
	if (glm::length(m_inputVector) > 0.01f)
	{
		m_inputVector = glm::normalize(m_inputVector);

		m_movement->move(m_collision->radius(), m_inputVector, float(seconds) * moveSpeed, e2::CollisionType::Mountain | e2::CollisionType::Tree | e2::CollisionType::Water | e2::CollisionType::Component, m_collisionCache);
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

	//if (m_aiming)
	//{
	//	if (mouse.buttonPressed(e2::MouseButton::Left))
	//	{
	//		e2::TileData cursorTile = game()->hexGrid()->getTileData(game()->cursorHex());
	//		if (cursorTile.isLand() && !cursorTile.isMountain())
	//		{
	//			glm::vec2 cursorPlane = game()->cursorPlane();
	//			if (glm::distance(cursorPlane, planarPosition) < 2.0f)
	//			{
	//				beLandcrab();
	//				glm::vec3 translation = getTransform()->getTranslation(e2::TransformSpace::World);
	//				getTransform()->setTranslation({ cursorPlane.x, translation.y, cursorPlane.y }, e2::TransformSpace::World);
	//			}

	//		}
	//	}
	//}

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
			m_footstepGrass->play(0.05f);
		else if (biome == e2::TileFlags::BiomeDesert)
			m_footstepDesert->play(0.05f);
		else if (biome == e2::TileFlags::BiomeTundra)
			m_footstepSnow->play(0.05f);
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

}

void e2::PlayerEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

bool e2::RadialCompare::operator()(e2::Entity* a, e2::Entity* b) const
{
	e2::Game* game = a->game();
	e2::PlayerEntity* player = game->playerEntity();

	if (!player)
		return false;

	
	e2::Transform* playerTransform = player->getTransform();
	glm::vec3 playerFwd = playerTransform->getForward(e2::TransformSpace::World);
	glm::vec3 playerPosition = playerTransform->getTranslation(e2::TransformSpace::World);
	glm::vec2 playerCoords{ playerPosition.x, playerPosition.z };
	glm::vec2 playerFwdPlanar = glm::normalize(glm::vec2{playerFwd.x, playerFwd.z});

	glm::vec2 dirA = glm::normalize(a->planarCoords() - playerCoords);
	glm::vec2 dirB = glm::normalize(b->planarCoords() - playerCoords);

	float radiansA = e2::radiansBetween(dirA, playerFwdPlanar);
	float radiansB = e2::radiansBetween(dirB, playerFwdPlanar);
	return radiansA < radiansB;
}









e2::OldGuySpecification::OldGuySpecification()
	: e2::EntitySpecification()
{
	entityType = e2::Type::fromName("e2::OldGuyEntity");
}

e2::OldGuySpecification::~OldGuySpecification()
{
}

void e2::OldGuySpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	if (obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);
	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);
}

void e2::OldGuySpecification::finalize()
{
	mesh.finalize(game);
	collision.finalize(game);
}













e2::OldGuyEntity::OldGuyEntity()
{
}

e2::OldGuyEntity::~OldGuyEntity()
{
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);

}

void e2::OldGuyEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	m_oldGuySpecification = m_specification->cast<e2::OldGuySpecification>();

	m_mesh = e2::create<e2::StaticMeshComponent>(&m_oldGuySpecification->mesh, this);
	
	m_collision = e2::create<e2::CollisionComponent>(&m_oldGuySpecification->collision, this);
}

void e2::OldGuyEntity::writeForSave(e2::IStream& toBuffer)
{
}

void e2::OldGuyEntity::readForSave(e2::IStream& fromBuffer)
{
}

void e2::OldGuyEntity::updateAnimation(double seconds)
{

}

void e2::OldGuyEntity::update(double seconds)
{
	m_mesh->applyTransform();
}

void e2::OldGuyEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::OldGuyEntity::onInteract(e2::Entity* interactor)
{
	game()->runScriptGraph("oldguy_poppy");
}
