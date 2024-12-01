
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <e2/rhi/renderresource.hpp>
#include <e2/rhi/enums.hpp>
#include <e2/rhi/threadcontext.hpp>
#include <e2/rhi/rendertarget.hpp>

#include <glm/glm.hpp> 

namespace e2
{


	class IDataBuffer;
	class IShader;
	class IPipeline;

	// use dynamic vertex attributes 
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VK_EXT_vertex_input_dynamic_state.html

	// User dynamic viewport 
	// https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/vkCmdSetViewport.html

	/* 
	 * Use dynamic :
	 * 
	 * Vertex attributes 
	 * viewport 
	 * scissor 
	 * depth bias 
	 * blend constants 
	 * depth bounds 
	 * stencil compare
	 * stencil write 
	 * stencil ref 
	 * stencil test 
	 * stencil op 
	 * 
	 * cull mode 
	 * front face 
	 * depth test 
	 * depth write 
	 * depth compare 
	 * depth bias 
	 * 
	 */


	/*
	
		Cache DescriptorSetLayout in Vulkan Impl (Vector<Type, Count>)
		Cache PipelineLayout in Vulkan Impl (SetLayouts, PushConstantSize)
	
		Dynamically handle DescriptorPools from within implementation
			For each cached DescriptorSetLayout;
				Maintain a dynamically growing pool of sets, tracking the pools free entries 
				On allocation, check if existing pool has room, if so, allocate
					if no pool has room, create a new pool. 

					Use static number of maxSets (32, 64, 128, or 256 for example)
					Prewarm with 1-4 pools on first creation 


	*/

	/** This is not a build configuration, but rather a constant to what our functionality support. */
	constexpr uint32_t maxPipelineStages = 6;



	class IDescriptorSetLayout;

	struct E2_API PipelineLayoutCreateInfo
	{
		e2::StackVector<IDescriptorSetLayout*, maxPipelineSets> sets;
		uint32_t pushConstantSize{ 0 };
	};

	class E2_API IPipelineLayout : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IPipelineLayout(e2::IRenderContext* context, e2::PipelineLayoutCreateInfo const& createInfo);
		virtual ~IPipelineLayout();
	};

	enum class PrimitiveTopology : uint8_t
	{
		Point = 0,
		Line,
		Triangle
	};

	enum class ComponentMask : uint8_t
	{
		R = 0b0000'0001,
		G = 0b0000'0010,
		B = 0b0000'0100,
		A = 0b0000'1000,
		RGBA = 0b0000'1111,
	};

	struct E2_API PipelineCreateInfo
	{
		IPipelineLayout* layout{};
		e2::StackVector<e2::IShader*, maxPipelineStages> shaders;
		uint32_t patchControlPoints{ 1 };
		bool alphaBlending{false};
		PrimitiveTopology topology{ PrimitiveTopology::Triangle };

		e2::StackVector<e2::TextureFormat, e2::maxNumRenderAttachments> colorFormats;
		e2::StackVector<e2::ComponentMask, e2::maxNumRenderAttachments> componentMasks;
		e2::TextureFormat depthFormat { e2::TextureFormat::Undefined }; // target is null if unused
		e2::TextureFormat stencilFormat { e2::TextureFormat::Undefined }; // target is null if unused

	};


	class E2_API IPipeline : public e2::RenderResource
	{
		ObjectDeclaration()
	public:
		IPipeline(e2::IRenderContext* renderContext, PipelineCreateInfo const& createInfo);
		virtual ~IPipeline();
	};

	EnumFlagsDeclaration(e2::ComponentMask);
}

#include "pipeline.generated.hpp"