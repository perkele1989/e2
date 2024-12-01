
#include "game/entities/radionentity.hpp"
#include "game/game.hpp"

e2::RadionEntitySpecification::RadionEntitySpecification()
	: e2::EntitySpecification()
{
}

e2::RadionEntitySpecification::~RadionEntitySpecification()
{
}

void e2::RadionEntitySpecification::populate(e2::GameContext* ctx, nlohmann::json& obj)
{
	e2::EntitySpecification::populate(ctx, obj);

	if (obj.contains("pins"))
	{
		nlohmann::json& jsonPins = obj.at("pins");
		for (nlohmann::json& jsonPin : jsonPins)
		{
			if (!jsonPin.contains("name"))
			{
				LogWarning("pins contain invalid object");
				continue;
			}

			e2::Name pinName = jsonPin.at("name").template get<std::string>();
			glm::vec3 pinOffset{};
			if (jsonPin.contains("offset"))
			{
				nlohmann::json& jsonPinOffset = jsonPin.at("offset");
				if (!jsonPinOffset.is_array() || jsonPinOffset.size() != 3)
				{
					LogWarning("invalid pin offset");
				}
				else
				{
					pinOffset.x = jsonPinOffset[0].template get<float>();
					pinOffset.y = jsonPinOffset[1].template get<float>();
					pinOffset.z = jsonPinOffset[2].template get<float>();
				}
			}

			pins.push({pinName, pinOffset});
		}
	}

	if (obj.contains("radionCost"))
	{
		radionCost = obj.at("radionCost").template get<uint32_t>();
	}

	if (obj.contains("mesh"))
		mesh.populate(obj.at("mesh"), assets);

	if (obj.contains("collision"))
		collision.populate(obj.at("collision"), assets);

}

void e2::RadionEntitySpecification::finalize()
{
	mesh.finalize(game);
	collision.finalize(game);
}

int32_t e2::RadionEntitySpecification::pinIndexFromName(e2::Name name)
{
	if (name.index() == 0)
		return -1;
	for (int32_t i = 0; i < pins.size(); i++)
	{
		if (pins[i].name == name)
		{
			return i;
		}
	}
	return -1;
}

e2::RadionEntity::~RadionEntity()
{
	radionManager()->unregisterEntity(this);
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);
}

void e2::RadionEntity::radionTick()
{
}

void e2::RadionEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);

	radionSpecification = m_specification->cast<e2::RadionEntitySpecification>();
	
	uint64_t numPins = radionSpecification->pins.size();
	connections.resize(numPins);
	outputRadiance.resize(radionSpecification->pins.size());
	for (uint64_t i = 0; i < numPins; i++)
	{
		connections[i] = {};
		outputRadiance[i] = 0.0f;
	}

	m_mesh = e2::create<e2::StaticMeshComponent>(&radionSpecification->mesh, this);
	m_collision = e2::create<e2::CollisionComponent>(&radionSpecification->collision, this);

	radionManager()->registerEntity(this);
	
}

void e2::RadionEntity::update(double seconds)
{
	m_collision->invalidate();
}

void e2::RadionEntity::updateAnimation(double seconds)
{
	m_mesh->applyTransform();
}

void e2::RadionEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::RadionEntity::connectPin(e2::Name pinName, e2::RadionEntity* otherEntity, e2::Name otherPinName)
{
	disconnectPin(pinName);

	if (!otherEntity)
	{
		LogError("otherEntity null");
		return;
	}

	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	int32_t otherPinIndex = otherEntity->radionSpecification->pinIndexFromName(otherPinName);

	if (pinIndex < 0 || otherPinIndex < 0)
	{
		LogError("no such pin");
		return;
	}

	connections[pinIndex].otherEntity = otherEntity;
	connections[pinIndex].otherPin = otherPinName;

	otherEntity->connections[otherPinIndex].otherEntity = this;
	otherEntity->connections[otherPinIndex].otherPin = pinName;

}

void e2::RadionEntity::disconnectPin(e2::Name pinName)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex < 0)
	{
		LogError("invalid pin name");
		return;
	}

	e2::RadionEntity* otherEntity = connections[pinIndex].otherEntity;
	if (!otherEntity)
	{
		LogError("broken connection! WTF");
		return;
	}

	int32_t otherPinIndex = otherEntity->radionSpecification->pinIndexFromName(connections[pinIndex].otherPin);

	connections[pinIndex].otherEntity = nullptr;
	connections[pinIndex].otherPin = e2::Name();

	otherEntity->connections[otherPinIndex].otherEntity = nullptr;
	otherEntity->connections[otherPinIndex].otherPin = e2::Name();
}

float e2::RadionEntity::getInputRadiance(e2::Name pinName)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex < 0)
		return 0.0f;

	e2::RadionEntity* otherEntity = connections[pinIndex].otherEntity;
	if (!otherEntity)
		return 0.0f;

	return otherEntity->getOutputRadiance(connections[pinIndex].otherPin);
}

float e2::RadionEntity::getOutputRadiance(e2::Name pinName)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex >= 0)
		return outputRadiance[pinIndex];

	return 0.0f;
}

void e2::RadionEntity::setOutputRadiance(e2::Name pinName, float newRadiance)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex < 0)
		return;

	outputRadiance[pinIndex] = newRadiance;
}


e2::RadionPreviewEntity::~RadionPreviewEntity()
{
	if (mesh)
		e2::destroy(mesh);
}

void e2::RadionPreviewEntity::postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation)
{
	e2::Entity::postConstruct(ctx, spec, worldPosition, worldRotation);
	radionSpecification = m_specification->cast<e2::RadionEntitySpecification>();
	mesh = e2::create<e2::StaticMeshComponent>(&radionSpecification->mesh, this);
}




void e2::RadionCrystal::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	setOutputRadiance("Output", state ? inputRadiance * e2::radionDecay : e2::radionLowRadiance);

	state = !state;
}

void e2::RadionSwitch::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	setOutputRadiance("Output", state ? inputRadiance * e2::radionDecay : e2::radionLowRadiance);
}

void e2::RadionCapacitor::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	charge += inputRadiance;
	charge = glm::clamp(charge, e2::radionLowRadiance, e2::radionHighRadiance);

	if (charge > e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", charge);
		charge *= e2::radionDecay;
	}
	else
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
	}
}

void e2::RadionWirePost::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	setOutputRadiance("Output", inputRadiance * e2::radionDecay);
}

void e2::RadionPowerSource::radionTick()
{
	setOutputRadiance("Output", e2::radionHighRadiance);
}

void e2::RadionGateNOT::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output", inputRadiance >= e2::radionSignalTreshold ? powerRadiance * e2::radionDecay : e2::radionLowRadiance);
}

void e2::RadionGateAND::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		(inputRadianceA >= e2::radionSignalTreshold && inputRadianceB >= e2::radionSignalTreshold) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}

void e2::RadionGateOR::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		(inputRadianceA >= e2::radionSignalTreshold || inputRadianceB >= e2::radionSignalTreshold) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}

void e2::RadionGateXOR::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		((inputRadianceA >= e2::radionSignalTreshold) != (inputRadianceB >= e2::radionSignalTreshold)) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}

void e2::RadionGateNAND::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		!(inputRadianceA >= e2::radionSignalTreshold && inputRadianceB >= e2::radionSignalTreshold) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}

void e2::RadionGateNOR::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		!(inputRadianceA >= e2::radionSignalTreshold || inputRadianceB >= e2::radionSignalTreshold) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}

void e2::RadionGateXNOR::radionTick()
{
	float inputRadianceA = getInputRadiance("InputA");
	float inputRadianceB = getInputRadiance("InputB");
	float powerRadiance = getInputRadiance("Power");

	if (powerRadiance < e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionLowRadiance);
		return;
	}

	setOutputRadiance("Output",
		((inputRadianceA >= e2::radionSignalTreshold) == (inputRadianceB >= e2::radionSignalTreshold)) ?
		powerRadiance * e2::radionDecay :
		e2::radionLowRadiance);
}
