
#include "e2/graph/graph.hpp"

e2::GraphNode::GraphNode(e2::Graph* inGraph) : graph(inGraph)
{
}

e2::GraphNode::~GraphNode()
{
}

e2::GraphPin* e2::GraphNode::createInputPin(e2::Name newDisplayName, e2::GraphPinDataType newDataType)
{
	e2::GraphPin* newPin = e2::create<e2::GraphPin>(this, e2::GraphPinIO::Input, newDisplayName, newDataType);
	inputPins.push(newPin);
	return newPin;
}

e2::GraphPin* e2::GraphNode::createOutputPin(e2::Name newDisplayName, e2::GraphPinDataType newDataType)
{
	e2::GraphPin* newPin = e2::create<e2::GraphPin>(this, e2::GraphPinIO::Output, newDisplayName, newDataType);
	outputPins.push(newPin);
	return newPin;
}

e2::GraphConnection* e2::Graph::connect(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin)
{
	// Make sure both pins are good
	if (!sourcePin || !destinationPin)
		return nullptr;

	// Make sure destination pin does not have a pin already, if so, disconnect it first
	// we are working from the fact that we can only have max 1 connection at a time, so we only disconnect the first one
	if (destinationPin->connections.size() > 0)
		disconnect(destinationPin->connections[0]->sourcePin, destinationPin);

	// check graph compatibility
	if (!isCompatible(sourcePin->dataType, destinationPin->dataType))
		return nullptr;

	e2::GraphConnection* existingConnection = getConnection(sourcePin, destinationPin);

	if (existingConnection)
		return existingConnection;

	e2::GraphConnection* newConnection = e2::create<e2::GraphConnection>(sourcePin, destinationPin);
	sourcePin->connections.push(newConnection);
	destinationPin->connections.push(newConnection);
	m_connections.push(newConnection);

	return newConnection;
}

void e2::Graph::disconnect(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin)
{
	e2::GraphConnection* connection{};
	for (e2::GraphConnection* conn : m_connections)
	{
		if (conn->sourcePin == sourcePin && conn->destinationPin == destinationPin)
		{
			connection = conn;
			break;
		}
	}
	if (!connection)
		return;

	connection->sourcePin->connections.removeFirstByValueFast(connection);
	connection->destinationPin->connections.removeFirstByValueFast(connection);
	m_connections.removeFirstByValueFast(connection);
	e2::destroy(connection);

	return;
}

e2::GraphConnection* e2::Graph::getConnection(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin)
{
	for (e2::GraphConnection* conn : m_connections)
	{
		if (conn->sourcePin == sourcePin && conn->destinationPin == destinationPin)
			return conn;
	}

	return nullptr;
}

e2::GraphConnection::GraphConnection(e2::GraphPin* inSourcePin, e2::GraphPin* inDestinationPin)
	: sourcePin(inSourcePin)
	, destinationPin(inDestinationPin)
{

}

e2::GraphConnection::~GraphConnection()
{

}

e2::GraphPin::GraphPin(e2::GraphNode* parent, e2::GraphPinIO newIO, e2::Name newDisplayName, e2::GraphPinDataType newDataType)
	: node(parent)
	, io(newIO)
	, displayName(newDisplayName)
	, dataType(newDataType)
{

}

e2::GraphPin::~GraphPin()
{

}
