
#pragma once 
#include <cstdint>

/** 
 * Build configuration file for e2
 * ---
 * 
 * This file is manually authored to fit a target application, by configuring memory overhead.
 * It can be used to set parameters that control pre-allocations and arena-sizes in the engine.
 * Both per-case implementation specifics as well as the arenaSize annotations used for static code generation are affected by these numbers
 * 
 * Handle with care.
 */

namespace e2
{
	constexpr uint64_t maxNumDynamicMeshes = 1024;

	/** The total length in bytes of the databuffer for e2::Name, 16 megabytes for now */
	constexpr uint32_t nameBufferLength = 16 * 1024 * 1024;

	/** The number of managed blocks to preallocate globally, 128k block entries. This limits number of instances of e2::ManagedObjects (use e2::Object instead when you don't need to track it with e2::Ptr's) */
	constexpr uint64_t managedBlockArenaSize = 128 * 1024;

	/** The resolution of glyph maps (fonts use dynamically generated multi-atlas SDF glyphmaps) */
	constexpr uint32_t glyphMapResolution = 512;

	/** Maximum number of glyph maps per font asset */
	constexpr uint32_t maxNumFontGlyphMaps = 16;

	/** The maximum number of sessions per runtime (normal gamebuilds only need one, however editors may want extra for play-in-editor and similar usecases) */
#if defined(E2_GAME_BUILD)
	constexpr uint32_t maxNumSessions = 1;
#else 
	constexpr uint32_t maxNumSessions = 16;
#endif

	/// --- Begin Physics 

	constexpr uint32_t maxNumPhysicsWorlds = 8;

	constexpr uint32_t maxNumRigidBodies = 1024;

	/// --- End Physics 

	/// --- Begin Graphs 

	/** Maximum number of nodes in a graph */
	constexpr uint64_t maxNumNodesPerGraph = 256;

	/** Maximum number of connections in a graph */
	constexpr uint64_t maxNumConnectionsPerGraph = 512;

	/** Maximum number of input or output pins in a graphnode (effectively half the number of maximum pins, as there exists both input and output pins) */
	constexpr uint64_t maxNumNodePins = 16;

	/** Total maximum number of graph connections (active per runtime) */
	//constexpr uint64_t maxNumGraphConnections = 1024;
	constexpr uint64_t maxNumGraphConnections = 1;

	/** Total maximum number of graph pins (active per runtime) */
	//constexpr uint64_t maxNumGraphPins = 512;
	constexpr uint64_t maxNumGraphPins = 1;

	//constexpr uint64_t maxNumConnectionsPerPin = 8;
	constexpr uint64_t maxNumConnectionsPerPin = 1;

	/// --- End Graphs

	/// --- Begin Optikus (shadergraph system)

	/** Maximum number of texture parameters in an optikus graph */
	//constexpr uint64_t maxNumOptikusTextures = 8;
	constexpr uint64_t maxNumOptikusTextures = 1;

	/** Maximum number of vec4 parameters in an optikus graph */
	//constexpr uint64_t maxNumOptikusParams = 16;
	constexpr uint64_t maxNumOptikusParams = 1;

	//const uint64_t maxNumOptikusProxies = 4096;
	const uint64_t maxNumOptikusProxies = 1;

	/// --- End Optikus

	/// --- Begin Assets 

	/** The maximum number of asset entries. Be VERY careful when changing this in an active project!  */
	constexpr uint32_t maxNumAssetEntries = 1024;

	/** The maximum number of dependencies an asset can have. Be careful when changing this in an active project. Hard limit at 255 as we use 8-bit integers to index asset deps arrays. */
	constexpr uint32_t maxNumAssetDependencies = 16;

	/** The maximum number of concurrently loaded (or generated!) mesh assets we support. */
	constexpr uint32_t maxNumMeshAssets = 2048;


	/** The maximum number of concurrently loaded skeleton assets we support. */
	constexpr uint32_t maxNumSkeletonAssets = 256;

	/** The maximum number of concurrently loaded texture assets we support. */
	constexpr uint32_t maxNumSoundAssets = 256;

	/** The maximum number of concurrently loaded texture assets we support. */
	constexpr uint32_t maxNumTexture2DAssets = 256;

	/** The maximum number of concurrently loaded font assets we support. */
	constexpr uint32_t maxNumFontAssets = 32;

	/** The maximum number of concurrently loaded spritesheet assets we support. */
	constexpr uint32_t maxNumSpritesheetAssets = 128;

	constexpr uint32_t maxNumAljStates = 255;

	/// ---End Assets

	/// --- Begin Rendering

	/** Maximum number of registerable shader models */
	constexpr uint32_t maxNumShaderModels = 8;

	/** The maximum number of renderers in a session */
	constexpr uint32_t maxNumRenderers = 8;

	 /** The maximum number of submeshes per mesh (see e2::Mesh) */
	constexpr uint32_t maxNumSubmeshes = 8;

	/** The maximum number of active mesh proxies per session. This limits how many meshes can be rendered at once, excluding mesh instancing and submeshes. (see e2::MeshProxy, e2::Session) */
	constexpr uint32_t maxNumMeshProxies = 4096;
	constexpr uint32_t maxNumSkinProxies = 1024;

	/** The maximum number of bones in a skeletal mesh (see e2::SkinData) */
	constexpr uint64_t maxNumBoneChildren = 16;
	constexpr uint64_t maxNumRootBones = 16;
	constexpr uint64_t maxNumSkeletonBones = 128;


	/** The size of the render dispatch queue. Every renderer will use at least 1. Extra uses from other things such as quadrenderer etc. */
	constexpr uint32_t maxNumQueuedBuffers = 32;

	/** How many registered render callbacks that is supported. */
	constexpr uint32_t maxNumRenderCallbacks = 128;

	/** How many lightweight material proxies/instances we support. */
	constexpr uint32_t maxNumLightweightProxies = 512;

	/** How many water material proxies/instances we support. */
	constexpr uint32_t maxNumWaterProxies = 256;

	constexpr uint32_t maxNumTerrainProxies = 256;

	/// --- End Rendering


	/// --- Begin RHI

	/** The maximum number of bindings in a descriptor set layout */
	constexpr uint32_t maxSetBindings = 16;

	/** How many descriptor sets we support per pipeline. Beware that increasing this from 4 will go against whats supported in Vulkan */
	constexpr uint32_t maxPipelineSets = 4;

	/** Maximum number of writable render attachments in a render target */
	constexpr uint32_t maxNumRenderAttachments = 8;

	/** Maximum number of shader defines per shader */
	constexpr uint8_t maxShaderDefines = 16;

	/** The maximum number of vertex bindings in a vertex layout */
	constexpr uint32_t maxNumVertexBindings = 8;

	/** The maximum number of vertex attributes in a vertex layout */
	constexpr uint32_t maxNumVertexAttributes = 8;

	/// ------ Begin Vulkan

	constexpr uint32_t maxNumDescriptorImages = 16;

	/** Maximum number of Vulkan surface formats to query. They are 3 on 2070S. We likely have to look into this again with more realworld data @todo */
	constexpr uint32_t maxVkSurfaceFormats = 8;

	/** Maximum number of submittable command buffers in Vulkan */
	constexpr uint32_t maxVkSubmitInfos = 128;

	// Arena sizes for the Vulkan resource primitives 
	constexpr uint32_t maxVkPipelineLayouts = 128;

	constexpr uint32_t maxVkPipelines = 8192;

	constexpr uint32_t maxVkDataBuffers = 4096;

	constexpr uint32_t maxVkDescriptorSetLayouts = 256;

	constexpr uint32_t maxVkFences = 64;

	constexpr uint32_t maxVkRenderTargets = 64;

	constexpr uint32_t maxVkSemaphores = 64;

	constexpr uint32_t maxVkShaders = 1024;

	// note: actual VkSamplers here are intelligently cached and reused in the vulkan implementation, this number can thus be relatively high without much bad conscience
	constexpr uint32_t maxVkSamplers = 128;

	constexpr uint32_t maxVkTextures = 1024;

	constexpr uint32_t maxVkVertexLayouts = 64;

	constexpr uint32_t maxVkWindows = 8;

	/// ------ End Vulkan


	/// --- End RHI

}