
#pragma once 

#include <e2/context.hpp>
#include <e2/renderer/shared.hpp>
#include <e2/utils.hpp>

#include <e2/assets/mesh.hpp>
#include <e2/assets/material.hpp>
#include <e2/game/session.hpp>

#include <glm/glm.hpp>

#include <vector>
#include <unordered_set>

namespace e2
{

	class Session;
	struct SkinData;
	class IDescriptorSet;
	class IPipeline;
	class IPipelineLayout;

	/** 
	 * Base-class for GPU proxies of a material asset.
	 * Implementation contains and tracks descriptor sets, uniform buffers, texture pointers etc.
	 * Every e2::Material has a defaultProxy
	 */
	class E2_API MaterialProxy : public e2::Context, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		MaterialProxy(e2::Session* inSession, e2::MaterialPtr materialAsset);
		virtual ~MaterialProxy();

		virtual Engine* engine() override;

		virtual void bind(e2::ICommandBuffer* buffer, uint8_t frameIndex) {};

		virtual void invalidate(uint8_t frameIndex) {};
		e2::Session* session{};

		e2::MaterialPtr asset; 

	};




	struct E2_API SkinProxyConfiguration
	{
		e2::SkeletonPtr skeleton;
		e2::MeshPtr mesh;
	};



	/**
	 * GPU proxy of a skin instance
	 * Contains and tracks data for skin
	 *
	 * Preallocate n where n = maximum number of mesh proxies per session * maximum number of sessions
	 *
	 * @tags(arena, arenaSize=e2::maxNumSkinProxies*e2::maxNumSessions)
	 */
	class E2_API SkinProxy : public e2::Context, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		SkinProxy(e2::Session* inSession, e2::SkinProxyConfiguration const& config);
		virtual ~SkinProxy();

		virtual Engine* engine() override;

		e2::Session* session{};

		/** The unique identifier we got from session when registering. Used for things like modelmatrix buffer offsets etc. */
		uint32_t id{ UINT32_MAX };

		e2::SkeletonPtr skeletonAsset{};
		e2::MeshPtr meshAsset{};

		void applyPose(e2::Pose* pose);

		glm::mat4 skin[e2::maxNumSkeletonBones];
		e2::Pair<bool> skinDirty{ true };
	};

	struct E2_API MeshProxyConfiguration
	{
		e2::MeshPtr mesh;

		// if this is empty, will use default materials from mesh
		e2::StackVector<e2::MaterialProxy*, e2::maxNumSubmeshes> materials;
	};


	/**
	 * GPU proxy of a mesh instance
	 * Contains and tracks data for uniform buffers, material proxies, skin proxies etc.
	 * 
	 * @tags(arena, arenaSize=e2::maxNumMeshProxies*e2::maxNumSessions)
	 */
	class E2_API MeshProxy : public e2::Context, public e2::ManagedObject
	{ 
		ObjectDeclaration()
	public:
		MeshProxy(e2::Session* inSession, e2::MeshProxyConfiguration const& config);
		virtual ~MeshProxy();

		bool enabled();
		void enable();
		void disable();

		void invalidatePipeline();

		virtual Engine* engine() override;

		e2::Session* session{};

		/** The unique identifier we got from session when registering. Used for things like modelmatrix buffer offsets etc. */
		uint32_t id{UINT32_MAX};

		e2::MeshPtr asset{};

		/** Material proxies for this mesh proxy, they default to the materials default proxies unless overridden. */
		e2::StackVector<e2::MaterialProxy*, e2::maxNumSubmeshes> materialProxies;

		/** Cache for the pipelines to use for the given submeshes in this proxy. Cached from session. */
		e2::StackVector<e2::IPipeline*, e2::maxNumSubmeshes> pipelines;

		/** Cache for the pipeline layouts to used for the given submeshes in this proxy. Cached from session. */
		e2::StackVector<e2::IPipelineLayout*, e2::maxNumSubmeshes> pipelineLayouts;

		// can be set whenever we bind it at render time if needed
		e2::SkinProxy* skinProxy{};

		// Local buffered mesh data, the data to be written to buffer next frame, if dirty
		glm::mat4 modelMatrix{};
		e2::Pair<bool> modelMatrixDirty{ true };
	};
}  
  
#include "meshproxy.generated.hpp"
