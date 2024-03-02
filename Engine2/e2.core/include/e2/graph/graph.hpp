
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <glm/glm.hpp>

namespace e2
{

	class GraphNode;
	class GraphPin;
	class GraphConnection;
	class Graph;

	// Optikus - Shader graph system (compiled to GLSL) 
	// Animus - Animation graph system (nativeJIT compiled)
	// Echo - Audio graph system (nativeJIT compiled)

	enum class GraphPinDataType : uint32_t
	{
		Undefined = 0,

		Optikus_Scalar,
		Optikus_Vector2,
		Optikus_Vector3,
		Optikus_Vector4,
		Optikus_Matrix3,
		Optikus_Matrix4,
		Optikus_Quaternion,
		Optikus_Texture,
		Optikus_Sampler,

		Animus_Scalar,
		Animus_Vector2,
		Animus_Vector3,
		Animus_Vector4,
		Animus_Quaternion,
		Animus_Pose, // Combination of bone transforms
		Animus_Expression, // Combination of shapekeys weights

		Echo_Boolean,
		Echo_Integer,
		Echo_Scalar,
		Echo_Waveform,
	};

	enum class GraphPinIO : uint8_t
	{
		Input,
		Output
	};

	/** @tags(arena, arenaSize=e2::maxNumGraphPins) */
	class E2_API GraphPin : public e2::Object
	{
		ObjectDeclaration()
	public:

		GraphPin(e2::GraphNode* parent, e2::GraphPinIO newIO, e2::Name newDisplayName, e2::GraphPinDataType newDataType);
		virtual ~GraphPin();

		e2::GraphNode *node{};
		e2::GraphPinIO io{};

		e2::Name displayName;
		e2::GraphPinDataType dataType { e2::GraphPinDataType::Undefined };

		e2::StackVector<e2::GraphConnection*, e2::maxNumConnectionsPerPin> connections;
	};

	/** Abstract base class for graph nodes. Needs to be inherited. */
	class E2_API GraphNode
	{
	public:
	
		GraphNode(e2::Graph* inGraph);

		virtual ~GraphNode();

		virtual bool isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin) = 0;

		e2::GraphPin* createInputPin(e2::Name newDisplayName, e2::GraphPinDataType newDataType);
		e2::GraphPin* createOutputPin(e2::Name newDisplayName, e2::GraphPinDataType newDataType);

		e2::Graph* graph{};

		/** The visual position of this node in the graph */
		glm::vec2 position;

		e2::StackVector<e2::GraphPin*, e2::maxNumNodePins> inputPins;
		e2::StackVector<e2::GraphPin*, e2::maxNumNodePins> outputPins;
	};

	/** @tags(arena, arenaSize=e2::maxNumGraphConnections) */
	class E2_API GraphConnection : public e2::Object
	{
		ObjectDeclaration()
	public:
		GraphConnection(e2::GraphPin* inSourcePin, e2::GraphPin* inDestinationPin);
		virtual ~GraphConnection();

		e2::GraphPin* sourcePin{};
		e2::GraphPin* destinationPin{};
	};

	inline bool operator== (GraphConnection const& lhs, GraphConnection const& rhs) noexcept
	{
		return lhs.sourcePin == rhs.sourcePin && lhs.destinationPin == rhs.destinationPin;
	}

	/** Baseclass for all graphs. Used for shadergraphs, animationgraphs, etc.*/
	class E2_API Graph
	{
	public:
		virtual void invalidate() {}

		virtual bool isCompatible(e2::GraphPinDataType sourceType, e2::GraphPinDataType destinationType) = 0;

		/** Creates a new node on this graph, of the given type and construction arguments */
		template<typename NodeType, typename... Args>
		NodeType* createNode(Args&&... args)
		{
			NodeType* newNode = e2::create<NodeType>(std::forward<Args>(args)...);
			m_nodes.push(newNode);
			return newNode;
		}

		/** Destroys a previously created node from this graph */
		template<typename NodeType>
		void destroyNode(NodeType* oldNode)
		{
			m_nodes.removeFirstByValueFast(oldNode);
			e2::destroy(oldNode);
		}

		/** Connects the given pins */
		e2::GraphConnection* connect(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin);

		/** Disconnects the given pins */
		void disconnect(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin);

		e2::GraphConnection* getConnection(e2::GraphPin* sourcePin, e2::GraphPin* destinationPin);

	protected:
		friend e2::GraphNode;

		e2::StackVector<e2::GraphNode*, e2::maxNumNodesPerGraph> m_nodes;
		e2::StackVector<e2::GraphConnection*, e2::maxNumConnectionsPerGraph> m_connections;
	};

}

#include "graph.generated.hpp"