
#include "e2/rhi/vk/vkshader.hpp"
#include "e2/rhi/vk/vkrendercontext.hpp"

#include <shaderc/shaderc.hpp>

namespace
{
	VkShaderStageFlagBits e2ToVk(e2::ShaderStage stage)
	{
		switch (stage)
		{
		default:
		case e2::ShaderStage::Vertex:
			return VK_SHADER_STAGE_VERTEX_BIT;
			break;
		case e2::ShaderStage::TessellationControl:
			return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			break;
		case e2::ShaderStage::TessellationEvaluation:
			return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			break;
		case e2::ShaderStage::Geometry:
			return VK_SHADER_STAGE_GEOMETRY_BIT;
			break;
		case e2::ShaderStage::Fragment:
			return VK_SHADER_STAGE_FRAGMENT_BIT;
			break;
		case e2::ShaderStage::Compute:
			return VK_SHADER_STAGE_COMPUTE_BIT;
			break;
		case e2::ShaderStage::RayGeneration:
			return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
			break;
		case e2::ShaderStage::RayAnyHit:
			return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
			break;
		case e2::ShaderStage::RayClosestHit:
			return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
			break;
		case e2::ShaderStage::RayMiss:
			return VK_SHADER_STAGE_MISS_BIT_KHR;
			break;
		case e2::ShaderStage::RayIntersection:
			return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
			break;
		case e2::ShaderStage::RayCallable:
			return VK_SHADER_STAGE_CALLABLE_BIT_KHR;
			break;
		}
	}
}

e2::IShader_Vk::IShader_Vk(IRenderContext* context, e2::ShaderCreateInfo const& createInfo)
	: e2::IShader(context, createInfo)
	, e2::ContextHolder_Vk(context)
	
{
	m_valid = false;
	m_vkStage = ::e2ToVk(createInfo.stage);

	// Compile ze shader 
	shaderc_shader_kind sc_kind;
	switch (createInfo.stage)
	{
	default:
	case e2::ShaderStage::Vertex:
		sc_kind = shaderc_shader_kind::shaderc_vertex_shader;
		break;
	case e2::ShaderStage::TessellationControl:
		sc_kind = shaderc_shader_kind::shaderc_tess_control_shader;
		break;
	case e2::ShaderStage::TessellationEvaluation:
		sc_kind = shaderc_shader_kind::shaderc_tess_evaluation_shader;
		break;
	case e2::ShaderStage::Geometry:
		sc_kind = shaderc_shader_kind::shaderc_geometry_shader;
		break;
	case e2::ShaderStage::Fragment:
		sc_kind = shaderc_shader_kind::shaderc_fragment_shader;
		break;
	case e2::ShaderStage::Compute:
		sc_kind = shaderc_shader_kind::shaderc_compute_shader;
		break;
	case e2::ShaderStage::RayGeneration:
		sc_kind = shaderc_shader_kind::shaderc_raygen_shader;
		break;
	case e2::ShaderStage::RayAnyHit:
		sc_kind = shaderc_shader_kind::shaderc_anyhit_shader;
		break;
	case e2::ShaderStage::RayClosestHit:
		sc_kind = shaderc_shader_kind::shaderc_closesthit_shader;
		break;
	case e2::ShaderStage::RayMiss:
		sc_kind = shaderc_shader_kind::shaderc_miss_shader;
		break;
	case e2::ShaderStage::RayIntersection:
		sc_kind = shaderc_shader_kind::shaderc_intersection_shader;
		break;
	case e2::ShaderStage::RayCallable:
		sc_kind = shaderc_shader_kind::shaderc_callable_shader;
		break;
	}

	shaderc::Compiler sc_compiler;
	shaderc::CompileOptions sc_options;
	sc_options.SetAutoMapLocations(true);

	for (e2::ShaderDefine const& shaderDefine : createInfo.defines)
	{
		sc_options.AddMacroDefinition(shaderDefine.name.string(), shaderDefine.value.string());
	}

	shaderc::SpvCompilationResult sc_result = sc_compiler.CompileGlslToSpv(createInfo.source, sc_kind, "@todo", "main", sc_options);
	if (sc_result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		LogError("Shader compilation failed: {} errors / {} warnings:", sc_result.GetNumErrors(), sc_result.GetNumWarnings());
		LogError("{}", sc_result.GetErrorMessage().c_str());
		return;
	}
	
	VkShaderModuleCreateInfo vkCreateInfo{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	vkCreateInfo.pCode = sc_result.cbegin();
	vkCreateInfo.codeSize = uint64_t(sc_result.cend() - sc_result.cbegin()) * sizeof(uint32_t);


	VkResult result = vkCreateShaderModule(m_renderContextVk->m_vkDevice, &vkCreateInfo, nullptr, &m_vkHandle);
	if (result != VK_SUCCESS)
	{
		LogError("vkCreateShaderModule failed: {}", int32_t(result));
		return;
	}

	m_valid = true;
}

e2::IShader_Vk::~IShader_Vk()
{
	vkDestroyShaderModule(m_renderContextVk->m_vkDevice, m_vkHandle, nullptr);
}

bool e2::IShader_Vk::valid()
{
	return m_valid;
}
