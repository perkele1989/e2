
#include "game/entities/radionentity.hpp"
#include "game/game.hpp"
#include <e2/game/gamesession.hpp>

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
				LogWarning("pin requires name");
				continue;
			}

			if (!jsonPin.contains("type"))
			{
				LogWarning("pin type (Input or Output)");
				continue;
			}

			e2::RadionPinType pinType = jsonPin.at("type").template get<std::string>() == "Input" ? e2::RadionPinType::Input : e2::RadionPinType::Output;

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

			pins.push({pinName,pinType, pinOffset});
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
	for (e2::RadionPin& pin : radionSpecification->pins)
	{
		if(pin.type == e2::RadionPinType::Input)
		{
			disconnectInputPin(pin.name);
		}
		else
		{
			disconnectOutputPin(pin.name);
		}

	}

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
	slots.resize(numPins);
	outputRadiance.resize(radionSpecification->pins.size());
	for (uint64_t i = 0; i < numPins; i++)
	{
		slots[i] = {};
		outputRadiance[i] = 0.0f;
	}

	m_mesh = e2::create<e2::StaticMeshComponent>(&radionSpecification->mesh, this);
	m_collision = e2::create<e2::CollisionComponent>(&radionSpecification->collision, this);

	radionManager()->registerEntity(this);
	
}

void e2::RadionEntity::update(double seconds)
{
	m_collision->invalidate();

	e2::GameSession* gameSession = game()->gameSession();
	e2::Renderer* renderer = gameSession->renderer();

	int32_t i = -1;
	for (e2::RadionPin& pin : radionSpecification->pins)
	{
		i++;
		if (pin.type != e2::RadionPinType::Output)
			continue;

		e2::RadionSlot& slot = slots[i];

		for (e2::RadionConnection conn : slot.connections)
		{
			e2::RadionEntity* otherEntity = conn.otherEntity;
			if (!otherEntity)
				continue;
			glm::vec3 aWorld = getTransform()->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(pin.offset, 1.0f);

			e2::RadionEntitySpecification* otherSpec = otherEntity->radionSpecification;
			int32_t otherIndex = otherSpec->pinIndexFromName(conn.otherPin);

			glm::vec3 bWorld = otherEntity->getTransform()->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(otherSpec->pins[otherIndex].offset, 1.0f);

			glm::vec3 color = glm::mix(glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0), outputRadiance[i] / e2::radionHighRadiance);
			renderer->debugLine(color, aWorld, bWorld);
		}



	}
}

void e2::RadionEntity::updateAnimation(double seconds)
{
	m_mesh->applyTransform();
}

void e2::RadionEntity::updateVisibility()
{
	m_mesh->updateVisibility();
}

void e2::RadionEntity::connectOutputPin(e2::Name outputPinName, e2::RadionEntity* inputEntity, e2::Name inputPinName)
{
	inputEntity->disconnectInputPin(inputPinName);

	int32_t outputPinIndex = radionSpecification->pinIndexFromName(outputPinName);
	e2::RadionPin& outputPin = radionSpecification->pins[outputPinIndex];
	e2::RadionSlot& outputSlot = slots[outputPinIndex];

	if (outputPin.type != e2::RadionPinType::Output)
	{
		LogError("not output pin");
		return;
	}

	if (outputSlot.connections.full())
	{
		e2::RadionConnection lastOutputConnection = outputSlot.connections.back();
		lastOutputConnection.otherEntity->disconnectInputPin(lastOutputConnection.otherPin);
	}

	if (outputSlot.connections.full())
	{
		LogError("code flow error!!!!");
		return;
	}

	int32_t inputPinIndex = inputEntity->radionSpecification->pinIndexFromName(inputPinName);
	e2::RadionPin& inputPin = inputEntity->radionSpecification->pins[inputPinIndex];
	if (inputPin.type != e2::RadionPinType::Input)
	{
		LogError("not input pin");
		return;
	}

	e2::RadionSlot& inputSlot = inputEntity->slots[inputPinIndex];
	inputSlot.connections.push({this, outputPinName});
	outputSlot.connections.push({ inputEntity, inputPinName });


}

void e2::RadionEntity::disconnectInputPin(e2::Name pinName)
{
	int32_t inputPinIndex = radionSpecification->pinIndexFromName(pinName);
	if (inputPinIndex < 0)
	{
		LogError("no such pin");
		return;
	}

	e2::RadionPin& inputPin = radionSpecification->pins[inputPinIndex];
	if (inputPin.type != e2::RadionPinType::Input)
	{
		LogError("not an input pin");
		return;
	}

	e2::RadionSlot& inputSlot = slots[inputPinIndex];
	if (inputSlot.connections.empty())
	{
		return;
	}

	if (inputSlot.connections.size() > 1)
	{
		LogWarning("multiple connections on input pin, this is invalid, will disconnect all");
	}

	for (e2::RadionConnection& conn : inputSlot.connections)
	{
		e2::RadionEntity* outputEntity = conn.otherEntity;
		e2::Name outputPinName = conn.otherPin;

		conn.otherEntity = nullptr;
		conn.otherPin = e2::Name();

		if (outputEntity)
		{
			int32_t outputPinIndex = outputEntity->radionSpecification->pinIndexFromName(outputPinName);
			if (outputPinIndex < 0)
			{
				LogError("no such pin (invalid connection)");
				continue;
			}

			e2::RadionSlot &outputSlot = outputEntity->slots[outputPinIndex];
			outputSlot.connections.removeFirstByValueFast({this, pinName});
		}
	}

	inputSlot.connections.clear();

}

void e2::RadionEntity::disconnectOutputPin(e2::Name pinName)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex < 0)
	{
		LogError("no such pin");
		return;
	}

	e2::RadionPin& pin = radionSpecification->pins[pinIndex];
	if (pin.type != e2::RadionPinType::Output)
	{
		LogError("not an output pin");
		return;
	}

	e2::RadionSlot& slot = slots[pinIndex];
	if (slot.connections.empty())
	{
		return;
	}

	e2::StackVector<e2::RadionConnection, maxNumRadionConnections> connCopy = slot.connections;

	for (e2::RadionConnection conn : connCopy)
	{
		if (!conn.otherEntity)
			continue;
		conn.otherEntity->disconnectInputPin(conn.otherPin);
	}
}

float e2::RadionEntity::getInputRadiance(e2::Name pinName)
{
	int32_t pinIndex = radionSpecification->pinIndexFromName(pinName);
	if (pinIndex < 0)
	{
		LogError("no such pin");
		return 0.0f;
	}

	e2::RadionPin& pin = radionSpecification->pins[pinIndex];
	if (pin.type != e2::RadionPinType::Input)
	{
		LogError("not input pin");
		return 0.0f;
	}

	e2::RadionSlot& slot = slots[pinIndex];
	if (slot.connections.empty())
	{
		// not connected
		return 0.0f;
	}

	if (slot.connections.size() > 1)
	{
		LogError("multiple connections on input pin, invalid!!");
		return 0.0f;
	}

	e2::RadionConnection& connection = slot.connections[0];

	e2::RadionEntity* otherEntity = connection.otherEntity;
	if (!otherEntity)
		return 0.0f;

	return otherEntity->getOutputRadiance(connection.otherPin);
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

bool e2::RadionEntity::anyOutputConnected()
{
	int32_t i = -1;
	for (e2::RadionPin& pin : radionSpecification->pins)
	{
		i++;

		if (pin.type != e2::RadionPinType::Output)
			continue;
		
		if (!slots[i].connections.empty())
			return true;
	}
	
	return false;
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

void e2::RadionSwitch::onInteract(e2::Entity* interactor)
{
	state = !state;
}

void e2::RadionCapacitor::radionTick()
{
	float inputRadiance = getInputRadiance("Input");

	if (inputRadiance > e2::radionPowerTreshold)
	{
		setOutputRadiance("Output", e2::radionHighRadiance);
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

	setOutputRadiance("Output", inputRadiance >= e2::radionSignalTreshold ? e2::radionLowRadiance : powerRadiance * e2::radionDecay);
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

void e2::RadionSplitter::radionTick()
{
	float inputRadiance = getInputRadiance("Input");
	inputRadiance *= e2::radionDecay;
	setOutputRadiance("OutputA", inputRadiance);
	setOutputRadiance("OutputB", inputRadiance);
}
