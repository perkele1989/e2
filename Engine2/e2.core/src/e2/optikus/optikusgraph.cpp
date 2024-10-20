
#include "e2/optikus/optikusgraph.hpp"

e2::OptikusOutputNode::OptikusOutputNode(e2::OptikusGraph* inOptikusGraph)
	: e2::OptikusNodeBase(inOptikusGraph)
{
	// outputColor
	createInputPin("outputColor", GraphPinDataType::Optikus_Vector4);
}

e2::OptikusOutputNode::~OptikusOutputNode()
{

}

bool e2::OptikusOutputNode::isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin)
{
	return true;
}

e2::OptikusNodeBase::OptikusNodeBase(e2::OptikusGraph* inOptikusGraph)
	: e2::GraphNode(inOptikusGraph)
{

}

e2::OptikusNodeBase::~OptikusNodeBase()
{

}


e2::OptikusGraph::~OptikusGraph()
{

}

void e2::OptikusGraph::write(e2::IStream& destination) const
{

}

bool e2::OptikusGraph::read(e2::IStream& source)
{
	return true;
}

bool e2::OptikusGraph::finalize()
{
	return true;
}
