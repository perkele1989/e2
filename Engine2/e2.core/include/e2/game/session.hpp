
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/context.hpp>
#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/assets/texture2d.hpp>
#include <e2/managers/assetmanager.hpp>
#include <e2/renderer/shared.hpp>


#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace e2
{
	class World;

	class SkinProxy;
	class MeshProxy;
	class ShaderModel;
	class Renderer;
	class IDataBuffer;
	class IDescriptorSet;
	class IPipeline;
	class IDescriptorSetLayout;
	class IDescriptorPool;

	class AssetEntry;

	struct E2_API MeshProxyLODEntry
	{
		e2::MeshProxy* proxy{};
		uint8_t lod{};
		uint8_t submesh{};

		bool operator==(const MeshProxyLODEntry& rhs) const;
		bool operator<(const MeshProxyLODEntry& rhs) const;
	};

}
template <>
struct std::hash<e2::MeshProxyLODEntry>
{
	std::size_t operator()(const e2::MeshProxyLODEntry& k) const
	{

		size_t h{};
		e2::hash_combine(h, k.proxy);
		e2::hash_combine(h, k.lod);
		e2::hash_combine(h, k.submesh);
		return h;
	}
};
namespace e2
{
	class E2_API Session : public e2::Context
	{
	public:

		Session(e2::Context* ctx);
		virtual ~Session();

		virtual void preTick(double seconds);
		virtual void tick(double seconds);

		virtual Engine* engine() override;

		uint32_t registerMeshProxy(e2::MeshProxy* proxy);
		void unregisterMeshProxy(e2::MeshProxy* proxy);

		void registerMaterialProxy(e2::MaterialProxy* proxy);
		void unregisterMaterialProxy(e2::MaterialProxy* proxy);

		uint32_t registerSkinProxy(e2::SkinProxy* proxy);
		void unregisterSkinProxy(e2::SkinProxy* proxy);

		std::map<e2::RenderLayer, std::unordered_set<MeshProxyLODEntry>> const& submeshIndex() const;
		std::unordered_set<MeshProxyLODEntry> const& shadowSubmeshes() const;

		e2::IDescriptorSet* getModelSet(uint8_t frameIndex);

		e2::IDescriptorSetLayout* getModelSetLayout();

		e2::MaterialProxy* getOrCreateDefaultMaterialProxy(e2::MaterialPtr material);
		void nukeDefaultMaterialProxy(e2::MaterialPtr material);

		void invalidateAllPipelines();

	protected:

		e2::Engine* m_engine{};

		/** Identifiers for models (meshproxies) */
		e2::IdArena<uint32_t, e2::maxNumMeshProxies> m_modelIds;
		e2::IdArena<uint32_t, e2::maxNumSkinProxies> m_skinIds;

		/** Descriptor sets for model matrices (one per frame index) */
		e2::Pair<e2::IDescriptorSet*> m_modelSets{nullptr};

		/** Buffers for model matrices (one per frame index) */
		e2::Pair<e2::IDataBuffer*> m_modelBuffers{nullptr};

		/** Buffers for skin matrices (one per frame index) */
		e2::Pair<e2::IDataBuffer*> m_skinBuffers{ nullptr };

		friend MeshProxy;

		/** All the registered mesh proxies */
		std::unordered_set<e2::MeshProxy*> m_meshProxies;

		std::unordered_set<e2::SkinProxy*> m_skinProxies;

		/** Submeshes ordered by render layer */
		std::map<e2::RenderLayer, std::unordered_set<MeshProxyLODEntry>> m_submeshIndex;

		std::unordered_set<MeshProxyLODEntry> m_shadowSubmeshes;

		std::unordered_set<e2::MaterialProxy*> m_materialProxies;

		std::unordered_map<e2::Material*, e2::MaterialProxy*> m_defaultMaterialProxies;
	};
}

