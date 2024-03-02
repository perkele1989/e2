
#pragma once

#include <cstdint>

namespace e2
{

	/**/
	enum class ShaderStage : uint8_t
	{
		Vertex = 0,
		TessellationControl,
		TessellationEvaluation,
		Geometry,
		Fragment,

		Compute,

		RayGeneration,
		RayAnyHit,
		RayClosestHit,
		RayMiss,
		RayIntersection,
		RayCallable

	};

	enum class BufferType : uint8_t
	{
		IndexBuffer,
		VertexBuffer,
		UniformBuffer,
		StorageBuffer
	};

	enum class TextureType : uint8_t
	{
		Texture2D,
		Texture2DArray,
		TextureCube,
		TextureCubeArray,
		Texture3D
	};

	/** 
	 * This is pretty abstracted over a lowlevel rhi, however in practice this is pretty much what  99.9% of sampler usecases are.
	 * The reason for simplifying this is that it allows a very effective caching of samplers. 
	 * 
	 * Nearest = No filter 
	 * Bilinear = Linear magnifying filter
	 * Trilinear = Bilinear with mip filtering 
	 * Anisotropic = Trilinear with anisotropic filtering enabled
	 */
	enum class SamplerFilter : uint8_t
	{
		Nearest = 0,
		Bilinear,
		Trilinear,
		Anisotropic
	};

	/** @todo: Is there a single usecase where clamp is actually neccessary??? / tired fred */
	enum class SamplerWrap : uint8_t
	{
		Clamp = 0,
		Repeat
	};

	enum class TextureFormat : uint8_t
	{
		Undefined = 0,
		R8,
		RG8,
		RGB8,
		SRGB8,
		RGBA8,
		SRGB8A8,
		
		R16,
		RG16,
		RGB16,
		RGBA16,
		
		R32,
		RG32,
		RGB32,
		RGBA32,
		
		D32,
		D16,
		
		D24S8,
		D32S8,
		S8,

		Count
	};


	enum class TextureLayout : uint8_t
	{
		// Layout to be read in shader (i.e. sampled)
		ShaderRead,

		// Layouts to be attachment
		ColorAttachment,
		DepthAttachment,
		StencilAttachment,
		DepthStencilAttachment,

		// Layouts for transfer
		TransferSource,
		TransferDestinatinon,

		// Layout for present, likely to never be used by this RHI
		PresentSource
	};

	enum class VertexFormat : uint8_t
	{
		Float,
		Vec2,
		Vec3,
		Vec4,
		Int,
		Vec2i,
		Vec3i,
		Vec4i,
		Uint,
		Vec2u,
		Vec3u,
		Vec4u
	};

	enum class VertexRate : uint8_t
	{
		PerVertex = 0,
		PerInstance
	};

	enum class LoadOperation : uint8_t
	{
		Ignore = 0,
		Clear,
		Load
	};

	enum class StoreOperation : uint8_t
	{
		None = 0,
		DontCare,
		Store
		
	};

	enum class BlitComponents : uint8_t
	{
		None = 0,
		All = 0b0000'0111,
		Color = 0b0000'0001,
		Depth = 0b0000'0010,
		Stencil = 0b0000'0100
	};
}