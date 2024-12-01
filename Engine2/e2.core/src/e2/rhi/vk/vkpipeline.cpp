
#include "e2/rhi/vk/vkpipeline.hpp"
#include "e2/rhi/vk/vkdescriptorsetlayout.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"
#include "e2/rhi/vk/vkshader.hpp"
#include "e2/rhi/vk/vktexture.hpp"


e2::IPipelineLayout_Vk::IPipelineLayout_Vk(e2::IRenderContext* context, e2::PipelineLayoutCreateInfo const& createInfo)
	: e2::IPipelineLayout(context, createInfo)
	, e2::ContextHolder_Vk(context)
{

	VkPipelineLayoutCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

	VkPushConstantRange vkRange{};
	if (createInfo.pushConstantSize > 0)
	{
		vkRange.offset = 0;
		vkRange.size = createInfo.pushConstantSize;
		vkRange.stageFlags = VK_SHADER_STAGE_ALL;

		vkCreateInfo.pPushConstantRanges = &vkRange;
		vkCreateInfo.pushConstantRangeCount = 1;
	}


	e2::StackVector<VkDescriptorSetLayout, maxPipelineSets> setLayouts;
	for (IDescriptorSetLayout* layout : createInfo.sets)
	{
		if (layout)
			setLayouts.push(static_cast<IDescriptorSetLayout_Vk*>(layout)->m_vkHandle);
		else
			setLayouts.push(nullptr);
	}

	vkCreateInfo.setLayoutCount = (uint32_t)setLayouts.size();
	vkCreateInfo.pSetLayouts = setLayouts.data();

	VkResult result = vkCreatePipelineLayout(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreatePipelineLayout failed: {}", int32_t(result));
	}

}

e2::IPipelineLayout_Vk::~IPipelineLayout_Vk()
{
	vkDestroyPipelineLayout(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

e2::IPipeline_Vk::IPipeline_Vk(IRenderContext* context, e2::PipelineCreateInfo const& createInfo)
	: e2::IPipeline(context, createInfo)
	, e2::ContextHolder_Vk(context)
{
	



	VkGraphicsPipelineCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	vkCreateInfo.pVertexInputState = nullptr;

	VkPipelineDynamicStateCreateInfo dynamicInfo{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	VkDynamicState dynamicStates[17] = { 
		VK_DYNAMIC_STATE_VERTEX_INPUT_EXT, 
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR,
		VK_DYNAMIC_STATE_CULL_MODE,
		VK_DYNAMIC_STATE_FRONT_FACE,
		VK_DYNAMIC_STATE_DEPTH_BIAS,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS,
		VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK,
		VK_DYNAMIC_STATE_STENCIL_WRITE_MASK,
		VK_DYNAMIC_STATE_STENCIL_REFERENCE,
		VK_DYNAMIC_STATE_DEPTH_TEST_ENABLE,
		VK_DYNAMIC_STATE_DEPTH_WRITE_ENABLE,
		VK_DYNAMIC_STATE_DEPTH_COMPARE_OP,
		VK_DYNAMIC_STATE_DEPTH_BOUNDS_TEST_ENABLE,
		VK_DYNAMIC_STATE_STENCIL_TEST_ENABLE,
		VK_DYNAMIC_STATE_STENCIL_OP,
		VK_DYNAMIC_STATE_DEPTH_BIAS_ENABLE
	};
	dynamicInfo.pDynamicStates = dynamicStates;
	dynamicInfo.dynamicStateCount = 17;
	vkCreateInfo.pDynamicState = &dynamicInfo;


	char const* entryPoint = "main";
	e2::StackVector<VkPipelineShaderStageCreateInfo, e2::maxPipelineStages> vkStages;
	for (e2::IShader* shader : createInfo.shaders)
	{
		e2::IShader_Vk* vkShader = static_cast<e2::IShader_Vk*>(shader);
		VkPipelineShaderStageCreateInfo newStage{ VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		newStage.module = vkShader->m_vkHandle;
		newStage.pName = entryPoint;
		newStage.stage = vkShader->m_vkStage;
		vkStages.push(newStage);
	}
	vkCreateInfo.stageCount = (uint32_t)vkStages.size();
	vkCreateInfo.pStages = vkStages.data();


	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;
	
	if (createInfo.topology == PrimitiveTopology::Triangle)
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	else if (createInfo.topology == PrimitiveTopology::Line)
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
	else if (createInfo.topology == PrimitiveTopology::Point)
		inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
	else
		LogError("Invalid pipeline topology");

	vkCreateInfo.pInputAssemblyState = &inputAssemblyInfo;

	VkPipelineTessellationStateCreateInfo tessInfo{ VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO };
	tessInfo.patchControlPoints = createInfo.patchControlPoints;
	vkCreateInfo.pTessellationState = &tessInfo;

	VkPipelineViewportStateCreateInfo viewInfo{ VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewInfo.viewportCount = 1;
	viewInfo.scissorCount = 1;
	vkCreateInfo.pViewportState = &viewInfo;


	VkPipelineRasterizationStateCreateInfo rasterInfo{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO};
	rasterInfo.lineWidth = 1.0f;
	rasterInfo.polygonMode = VK_POLYGON_MODE_FILL;
	vkCreateInfo.pRasterizationState = &rasterInfo;

	VkPipelineMultisampleStateCreateInfo msInfo{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	msInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	msInfo.minSampleShading = 1.0f;
	vkCreateInfo.pMultisampleState = &msInfo;

	VkPipelineDepthStencilStateCreateInfo dsInfo{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	dsInfo.minDepthBounds = 0.0f;
	dsInfo.maxDepthBounds = 1.0f;
	vkCreateInfo.pDepthStencilState = &dsInfo;

	VkPipelineColorBlendStateCreateInfo cbInfo{ VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	e2::StackVector<VkPipelineColorBlendAttachmentState, e2::maxNumRenderAttachments> vkAttachments;
	vkAttachments.resize(createInfo.colorFormats.size());

	bool hasComponentMasks = createInfo.componentMasks.size() == createInfo.colorFormats.size();

	for (uint32_t i = 0; i < createInfo.colorFormats.size();i++)
	{
		
		if (createInfo.alphaBlending)
		{
			vkAttachments[i].blendEnable = true;
			vkAttachments[i].srcColorBlendFactor =  VK_BLEND_FACTOR_SRC_ALPHA;
			vkAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			//vkAttachments[i].srcColorBlendFactor =  VK_BLEND_FACTOR_ONE;
			//vkAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			vkAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			vkAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		}
		else
		{
			vkAttachments[i].blendEnable = false;
			vkAttachments[i].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			vkAttachments[i].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
			vkAttachments[i].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			vkAttachments[i].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		}

		vkAttachments[i].colorBlendOp = VK_BLEND_OP_ADD;
		vkAttachments[i].alphaBlendOp = VK_BLEND_OP_ADD;

		if (hasComponentMasks)
		{
			vkAttachments[i].colorWriteMask = (VkColorComponentFlags)createInfo.componentMasks[i];
		}
		else
		{
			vkAttachments[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}
	}
	cbInfo.attachmentCount = (uint32_t)vkAttachments.size();
	cbInfo.pAttachments = vkAttachments.data();
	cbInfo.blendConstants[0] = 1.0f;
	cbInfo.blendConstants[1] = 1.0f;
	cbInfo.blendConstants[2] = 1.0f;
	cbInfo.blendConstants[3] = 1.0f;
	cbInfo.logicOp = VK_LOGIC_OP_COPY;
	cbInfo.logicOpEnable = VK_FALSE;
	vkCreateInfo.pColorBlendState = &cbInfo;

	e2::IPipelineLayout_Vk *vkLayout = static_cast<e2::IPipelineLayout_Vk*>(createInfo.layout);
	vkCreateInfo.layout = vkLayout->m_vkHandle;
	vkCreateInfo.renderPass = nullptr;;
	vkCreateInfo.subpass = 0;
	vkCreateInfo.basePipelineHandle = nullptr;
	vkCreateInfo.basePipelineIndex = 0;

	VkPipelineRenderingCreateInfo renderingInfo{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO};

	e2::StackVector<VkFormat, e2::maxNumRenderAttachments> vkColors;
	for (uint32_t i = 0; i < createInfo.colorFormats.size(); i++)
	{
		VkFormat newFormat = e2::ITexture_Vk::e2ToVk(createInfo.colorFormats[i]);
		vkColors.push(newFormat);
	}
	renderingInfo.colorAttachmentCount = (uint32_t)vkColors.size();
	renderingInfo.pColorAttachmentFormats = vkColors.data();

	renderingInfo.depthAttachmentFormat = e2::ITexture_Vk::e2ToVk(createInfo.depthFormat);
	renderingInfo.stencilAttachmentFormat = e2::ITexture_Vk::e2ToVk(createInfo.stencilFormat);

	vkCreateInfo.pNext = &renderingInfo;

	VkResult result = vkCreateGraphicsPipelines(m_renderContextVk->m_vkDevice, nullptr, 1, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateGraphicsPipelines failed: {}", int32_t(result));
	}
}

e2::IPipeline_Vk::~IPipeline_Vk()
{
	vkDestroyPipeline(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}
