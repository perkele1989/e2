#include "e2/animus/animusgraph.hpp"

e2::AnimusNodeBase::AnimusNodeBase(e2::AnimusGraph* inAnimusGraph)
	: e2::GraphNode(inAnimusGraph)
{

}

e2::AnimusNodeBase::~AnimusNodeBase()
{

}

bool e2::AnimusNodeBase::isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin)
{
	return true;
}

e2::AnimusOutputNode::AnimusOutputNode(e2::AnimusGraph* inAnimusGraph)
	: e2::AnimusNodeBase(inAnimusGraph)
{

}

e2::AnimusOutputNode::~AnimusOutputNode()
{

}

bool e2::AnimusOutputNode::isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin)
{
	return true;
}

e2::AnimusGraph::AnimusGraph()
	: e2::Graph()
{

}

e2::AnimusGraph::~AnimusGraph()
{

}
