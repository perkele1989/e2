
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/graph/graph.hpp>

namespace e2
{
	class AnimusGraph;
	class AnimusNodeBase : public e2::GraphNode
	{
	public:
		AnimusNodeBase(e2::AnimusGraph* inAnimusGraph);
		virtual ~AnimusNodeBase();

		virtual bool isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin) override;

		e2::AnimusGraph* animusGraph{};
	};

	class AnimusOutputNode : public e2::AnimusNodeBase
	{
	public:
		AnimusOutputNode(e2::AnimusGraph* inAnimusGraph);
		virtual ~AnimusOutputNode();

		virtual bool isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin) override;

	};


	/** */
	class AnimusGraph : public e2::Graph
	{
	public:
		AnimusGraph();
		virtual ~AnimusGraph();

	};

}

#include "animusgraph.generated.hpp"