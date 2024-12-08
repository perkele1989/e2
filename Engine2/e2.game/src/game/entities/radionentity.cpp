
#include "game/entities/radionentity.hpp"
#include "game/game.hpp"
#include <e2/game/gamesession.hpp>

#include <glm/gtx/spline.hpp>

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

	destroyConnectionMeshes();

	radionManager()->unregisterEntity(this);
	if (m_mesh)
		e2::destroy(m_mesh);

	if (m_collision)
		e2::destroy(m_collision);
}

void e2::RadionEntity::radionTick()
{
}

void e2::RadionEntity::writeForSave(e2::IStream& toBuffer)
{
	e2::Entity::writeForSave(toBuffer);
	
	for (float rad : outputRadiance)
	{
		toBuffer << rad;
	}

	for (e2::RadionSlot& slot : slots)
	{
		toBuffer << (uint64_t)slot.connections.size();
		for (e2::RadionConnection& conn : slot.connections)
		{
			toBuffer << (conn.otherEntity ? conn.otherEntity->uniqueId : (uint64_t)0);
			toBuffer << conn.otherPin;
		}
	}
}

void e2::RadionEntity::readForSave(e2::IStream& fromBuffer)
{
	e2::Entity::readForSave(fromBuffer);

	for (float& rad : outputRadiance)
	{
		fromBuffer >> rad;
	}

	for (e2::RadionSlot& slot : slots)
	{
		slot.connections.clear();
		uint64_t numConnections = 0;
		fromBuffer >> numConnections;
		for (uint64_t i = 0; i < numConnections; i++)
		{
			uint64_t entityId{};
			std::string pinName;
			fromBuffer >> entityId;
			fromBuffer >> pinName;
			e2::Entity* otherEntity = game()->entityFromId(entityId);
			if (!otherEntity)
			{
				LogError("no such entity, refusing connection");
				continue;
			}

			e2::RadionEntity* otherRadionEntity = otherEntity->cast<e2::RadionEntity>();
			if (!otherRadionEntity)
			{
				LogError("entity wasnt a radion entity wtf, refusing connection");
				continue;
			}

			slot.connections.push({ otherRadionEntity, pinName });
		}
	}

	updateConnectionMeshes();
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

	for (e2::ConnectionMesh& mesh : connectionMeshes)
	{
		float radiance = outputRadiance[mesh.pinIndex];
		bool powerOn = radiance >= e2::radionPowerTreshold;
		bool signalHigh = radiance >= e2::radionSignalTreshold;

		e2::MaterialProxy *glowProxy = game()->radionManager()->glowProxy();
		e2::MaterialProxy *unglowProxy = game()->radionManager()->unglowProxy();
		e2::MaterialProxy* signalProxy = game()->radionManager()->signalProxy();

		mesh.meshProxy->setMaterial(0, 0, powerOn ? glowProxy : ( signalHigh ? signalProxy :  unglowProxy));
	}

}

void e2::RadionEntity::updateAnimation(double seconds)
{
	m_mesh->applyTransform();
}

void e2::RadionEntity::updateVisibility()
{
	m_mesh->updateVisibility();


	for (e2::ConnectionMesh& mesh : connectionMeshes)
	{
		bool proxyEnabled = mesh.meshProxy->enabled();
		if (m_inView && !proxyEnabled)
		{
			mesh.meshProxy->enable();
		}
		else if (!m_inView && proxyEnabled)
		{
			mesh.meshProxy->disable();		
		}
	}
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

	updateConnectionMeshes();
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

			outputEntity->updateConnectionMeshes();
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

	updateConnectionMeshes();
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

e2::MeshPtr generateConnectionMesh(glm::vec3 a, glm::vec3 aO, glm::vec3 b, glm::vec3 bO, e2::Game* game)
{

	constexpr float halfWidth = 0.01f;
	glm::vec3 const normal = e2::worldUpf();

	float aHeight = a.y;
	float bHeight = b.y;

	a.y = 0.0f;
	b.y = 0.0f;
	aO.y = 0.0f;
	bO.y = 0.0f;

	glm::vec3 offsetA = glm::distance(aO, a) < 0.001f ? glm::normalize(b -a) : glm::normalize(aO - a);
	glm::vec3 offsetB = glm::distance(bO, b) < 0.001f ? glm::normalize(a - b) : glm::normalize(bO - b);

	glm::vec3 aO2 = a + offsetA * 0.5f;
	glm::vec3 bO2 = b + offsetB * 0.5f;
	constexpr int32_t numDivisions = 8;


	e2::DynamicMesh dynMesh;
	dynMesh.reserve(4 * (numDivisions), 2 * (numDivisions));
	float distance = 0.0f;
	for (int32_t currDivision = 0; currDivision < numDivisions; currDivision++)
	{
		int32_t nextDivision = currDivision + 1;

		float pastAlpha = (float)glm::max(currDivision - 1, 0) / numDivisions;
		float currAlpha = (float)currDivision / numDivisions;
		float nextAlpha = (float)nextDivision / numDivisions;
		float futureAlpha = (float)glm::min(nextDivision + 1, numDivisions+1) / numDivisions;

		float currHeight = glm::mix(aHeight, bHeight, currAlpha);
		float nextHeight = glm::mix(aHeight, bHeight, nextAlpha);

		glm::vec3 pastBegin = glm::catmullRom(aO2, a, b, bO2, pastAlpha);
		glm::vec3 begin = glm::catmullRom(aO2, a, b, bO2, currAlpha);
		glm::vec3 end = glm::catmullRom(aO2, a, b, bO2, nextAlpha);
		glm::vec3 futureEnd = glm::catmullRom(aO2, a, b, bO2, futureAlpha);

		float prevDistance = distance;
		distance += glm::distance(begin, end);

		glm::vec3 beginToEnd = end - begin;
		beginToEnd.y = 0.0f;

		glm::vec3 endToFuture = futureEnd - end;
		endToFuture.y = 0.0f;

		glm::vec3 beginToEndNorm = glm::normalize(beginToEnd);
		glm::vec3 tangentBegin{ -beginToEndNorm.z, 0.0f, beginToEndNorm.x };


		glm::vec3 endToFutureNorm = glm::normalize(endToFuture);
		glm::vec3 tangentEnd{ -endToFutureNorm.z, 0.0f, endToFutureNorm.x };


		glm::vec3 startRight = begin + tangentBegin * halfWidth;
		glm::vec3 startCenter = begin;
		glm::vec3 startLeft = begin - tangentBegin * halfWidth;

		glm::vec3 endRight = end + tangentEnd * halfWidth;
		glm::vec3 endCenter = end;
		glm::vec3 endLeft = end - tangentEnd * halfWidth;

		e2::Vertex tl, tc, tr, bl, bc, br;
		tl.position = startLeft + glm::vec3{0.0f, currHeight, 0.0f};
		tl.normal = normal;
		tl.tangent = tangentBegin;
		tl.uv01 = {-1.0, currAlpha, prevDistance, 0.0};

		tc.position = startCenter + glm::vec3{ 0.0f, currHeight, 0.0f };
		tc.normal = normal;
		tc.tangent = tangentBegin;
		tc.uv01 = { 0.0, currAlpha, prevDistance, 0.0 };

		tr.position = startRight + glm::vec3{0.0f, currHeight, 0.0f};
		tr.normal = normal;
		tr.tangent = tangentBegin;
		tr.uv01 = { 1.0, currAlpha, prevDistance, 0.0 };

		bl.position = endLeft + glm::vec3{0.0f, nextHeight, 0.0f};
		bl.normal = normal;
		bl.tangent = tangentEnd;
		bl.uv01 = { -1.0, nextAlpha, distance, 0.0 };

		bc.position = endCenter + glm::vec3{ 0.0f, nextHeight, 0.0f };
		bc.normal = normal;
		bc.tangent = tangentBegin;
		bc.uv01 = { 0.0, nextAlpha, distance, 0.0 };

		br.position = endRight + glm::vec3{0.0f, nextHeight, 0.0f};
		br.normal = normal;
		br.tangent = tangentEnd;
		br.uv01 = { 1.0, nextAlpha, distance, 0.0 };

		dynMesh.addVertex(tl);
		dynMesh.addVertex(tc);
		dynMesh.addVertex(tr);
		dynMesh.addVertex(bl);
		dynMesh.addVertex(bc);
		dynMesh.addVertex(br);

		e2::Triangle triA, triB, triC, triD;
		triA.a = 0 + (currDivision * 6);
		triA.b = 1 + (currDivision * 6);
		triA.c = 3 + (currDivision * 6);

		triB.a = 1 + (currDivision * 6);
		triB.b = 4 + (currDivision * 6);
		triB.c = 3 + (currDivision * 6);


		triC.a = 1 + (currDivision * 6);
		triC.b = 2 + (currDivision * 6);
		triC.c = 4 + (currDivision * 6);


		triD.a = 2 + (currDivision * 6);
		triD.b = 5 + (currDivision * 6);
		triD.c = 4 + (currDivision * 6);

		dynMesh.addTriangle(triA);
		dynMesh.addTriangle(triB);
		dynMesh.addTriangle(triC);
		dynMesh.addTriangle(triD);
	}




	e2::MaterialPtr mat = game->assetManager()->get("M_Connection_Off.e2a").cast<e2::Material>();

	return dynMesh.bake(mat, e2::VertexAttributeFlags::Normal |  e2::VertexAttributeFlags::TexCoords01);
}

void e2::RadionEntity::destroyConnectionMeshes()
{
	for (e2::ConnectionMesh& mesh : connectionMeshes)
	{
		if (mesh.meshProxy)
			e2::destroy(mesh.meshProxy);

		mesh.generatedMesh = nullptr;
	}

	connectionMeshes.clear();
}

void e2::RadionEntity::updateConnectionMeshes()
{

	destroyConnectionMeshes();
	int32_t i = -1;
	for (e2::RadionPin& pin : radionSpecification->pins)
	{
		i++;

		if (pin.type != e2::RadionPinType::Output)
			continue;

		for (e2::RadionConnection& conn : slots[i].connections)
		{



			e2::RadionEntity* otherEntity = conn.otherEntity;
			if (!otherEntity)
				continue;

			glm::vec3 aO = getTransform()->getTranslation(e2::TransformSpace::World);
			glm::vec3 aWorld = getTransform()->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(pin.offset, 1.0f);

			e2::RadionEntitySpecification* otherSpec = otherEntity->radionSpecification;
			int32_t otherIndex = otherSpec->pinIndexFromName(conn.otherPin);

			glm::vec3 bO = otherEntity->getTransform()->getTranslation(e2::TransformSpace::World);
			glm::vec3 bWorld = otherEntity->getTransform()->getTransformMatrix(e2::TransformSpace::World) * glm::vec4(otherSpec->pins[otherIndex].offset, 1.0f);

			e2::ConnectionMesh newMesh;
			newMesh.generatedMesh = generateConnectionMesh(aWorld, aO, bWorld, bO, game());

			e2::MeshProxyConfiguration cfg;
			e2::MeshLodConfiguration lod;
			lod.mesh = newMesh.generatedMesh;
			cfg.lods.push(lod);
			newMesh.meshProxy = e2::create<e2::MeshProxy>(gameSession(), cfg);

			newMesh.pinIndex = i;

			connectionMeshes.push(newMesh);
		}
	}
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

void e2::RadionSwitch::writeForSave(e2::IStream& toBuffer)
{
	e2::RadionEntity::writeForSave(toBuffer);

	toBuffer << (uint8_t)state;
}

void e2::RadionSwitch::readForSave(e2::IStream& fromBuffer)
{
	e2::RadionEntity::readForSave(fromBuffer);
	uint8_t s{};
	fromBuffer >> s;
	state = s;
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
