
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/graph/graph.hpp>
#include <e2/assets/texture2d.hpp>

#include <e2/rhi/pipeline.hpp>
#include <e2/rhi/shader.hpp>
#include <e2/rhi/descriptorsetlayout.hpp>

namespace e2
{
	enum class OptikusFlags : uint16_t
	{
		None = 0,

		// Inherited from VertexAttributeFlags
		VertexFlagsOffset = 0,
		Normal = 1 << 0, // Normal implies normal vectors AND tangent vectors (bitangent always derived in shader)
		TexCoords01 = 1 << 1, // Has at least 1 texcoords
		TexCoords23 = 1 << 2, // Has at least 3 texcoords
		Color = 1 << 3, // Has vertex colors
		Bones = 1 << 4, // Has bone weights and ids

		// Inherited from RendererFlags 
		RendererFlagsOffset = 5,
		Shadow = 1 << 5,
		Skin = 1 << 6,

		// Maps 
		MaterialFlagsOffset = 7,
		Count = 1 << 7

	};

	class OptikusGraph;
	class OptikusNodeBase : public e2::GraphNode
	{
	public:
		OptikusNodeBase(e2::OptikusGraph* inOptikusGraph);
		virtual ~OptikusNodeBase();
		e2::OptikusGraph* optikusGraph{};
	};

	class OptikusOutputNode : public e2::OptikusNodeBase
	{
	public:
		OptikusOutputNode(e2::OptikusGraph* inOptikusGraph);

		virtual ~OptikusOutputNode();

		virtual bool isCompatible(e2::GraphPinDataType sourceType, uint8_t destinationInputPin) override;

	};

	struct OptikusCacheEntry
	{
		e2::IPipeline* pipeline{};
		e2::IShader* vertexShader{};
		e2::IShader* fragmentShader{};
	};


	struct OptikusProperty
	{
		e2::Name name;
		glm::vec4 value;
	};

	struct OptikusTexture
	{
		e2::Name name;
		e2::Texture2DPtr texture;
	};

	/** */
	class OptikusGraph : public e2::Graph, public e2::Asset
	{
		ObjectDeclaration()
	public:
		OptikusGraph()=default;
		virtual ~OptikusGraph();

		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;
		virtual bool finalize() override;


	protected:
		// Asset data
		e2::StackVector<e2::OptikusProperty, e2::maxNumOptikusParams> m_properties;
		e2::StackVector<e2::OptikusTexture, e2::maxNumOptikusTextures> m_textures;

		// 
		e2::OptikusOutputNode* m_outputNode{};

		// Built data
		e2::IPipelineLayout* m_pipelineLayout{};
		e2::IDescriptorSetLayout* m_descriptorSetLayout{};
		e2::StackVector<e2::OptikusCacheEntry, uint16_t(e2::OptikusFlags::Count)> m_pipelineCache;

	};

}


EnumFlagsDeclaration(e2::OptikusFlags)

#include "optikusgraph.generated.hpp"