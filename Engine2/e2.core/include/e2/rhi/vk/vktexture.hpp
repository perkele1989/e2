
#pragma once 

#include <e2/buildcfg.hpp>

#include <e2/rhi/texture.hpp>
#include <e2/rhi/vk/vkresource.hpp>

#include <Volk/volk.h>
#include <vma/vk_mem_alloc.h>

namespace e2
{
	/** @tags(arena, arenaSize=e2::maxVkSamplers) */
	class E2_API ISampler_Vk : public e2::ISampler, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		ISampler_Vk(IRenderContext* context, e2::SamplerCreateInfo const& createInfo);
		ISampler_Vk(IRenderContext* context, VkSampler wrap);
		virtual ~ISampler_Vk();

		VkSampler m_vkHandle{};

	};

	/** @tags(arena, arenaSize=e2::maxVkTextures) */
	class E2_API ITexture_Vk : public e2::ITexture, public e2::ContextHolder_Vk
	{
		ObjectDeclaration()
	public:
		ITexture_Vk(IRenderContext* context, e2::TextureCreateInfo const& createInfo);
		virtual ~ITexture_Vk();

		virtual void generateMips() override;
		virtual void generateMipsCmd(e2::ICommandBuffer* buff) override;
		void mipsCmd(VkCommandBuffer buff);

		virtual void upload(uint32_t mip, glm::uvec3 offset, glm::uvec3 size, uint8_t const* data, uint64_t dataSize) override;

		VkImage m_vkImage{};
		VkImageView m_vkImageView{};
		VmaAllocation m_vmaHandle{};
		VkImageAspectFlags m_vkAspectFlags{};
		VkFormat m_vkFormat{};
		VkImageLayout m_vkLayout{};
		VkImageLayout m_vkTempLayout{};

		glm::uvec3 m_size;
		uint32_t m_layerCount{};
		uint32_t m_mipCount{};

		static VkFormat e2ToVk(e2::TextureFormat textureFormat);
		static VkImageAspectFlags e2ToAspectFlags(e2::TextureFormat textureFormat);
	};
}

#include "vktexture.generated.hpp"