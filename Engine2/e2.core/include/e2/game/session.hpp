
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

	class MeshProxy;
	class ShaderModel;
	class Renderer;
	class IDataBuffer;
	class IDescriptorSet;
	class IPipeline;
	class IDescriptorSetLayout;
	class IDescriptorPool;

	class AssetEntry;

	struct SkinData
	{
		alignas(16) glm::mat4 skin[e2::maxNumBones];
	};

	struct E2_API MeshProxySubmesh
	{
		e2::MeshProxy* proxy{};
		uint8_t submesh{};

		bool operator==(const MeshProxySubmesh& rhs) const;
		bool operator<(const MeshProxySubmesh& rhs) const;
	};

	class E2_API Session : public e2::Context
	{
	public:

		Session(e2::Context* ctx);
		virtual ~Session();

		virtual void tick(double seconds);

		virtual Engine* engine() override;


		virtual e2::World* spawnWorld();
		virtual void destroyWorld(e2::World* world);

		uint32_t registerMeshProxy(e2::MeshProxy* proxy);
		void unregisterMeshProxy(uint32_t id, e2::MeshProxy* proxy);

		void registerMaterialProxy(e2::MaterialProxy* proxy);
		void unregisterMaterialProxy(e2::MaterialProxy* proxy);

		uint32_t registerSkinId();
		void unregisterSkinId(uint32_t skinId);

		SkinData* skinData(uint32_t skinId);

		std::map<e2::RenderLayer, std::unordered_set<MeshProxySubmesh>> const& submeshIndex() const;

		inline e2::World* persistentWorld() const
		{
			return m_persistentWorld;
		}

		e2::IDescriptorSet* getModelSet(uint8_t frameIndex);

		e2::IDescriptorSetLayout* getModelSetLayout();

		e2::MaterialProxy* getOrCreateDefaultMaterialProxy(e2::MaterialPtr material);

	protected:

		e2::Engine* m_engine{};

		/** Identifiers for models (meshproxies) */
		e2::IdArena<uint32_t, e2::maxNumMeshProxies> m_modelIds;

		/** Set of all the worlds */
		std::unordered_set<e2::World*> m_worlds{};

		/** The persistent world */
		e2::World* m_persistentWorld{};

		/** Descriptor sets for model matrices (one per frame index) */
		e2::Pair<e2::IDescriptorSet*> m_modelSets{nullptr};

		/** Buffers for model matrices (one per frame index) */
		e2::Pair<e2::IDataBuffer*> m_modelBuffers{nullptr};

		/** All the registered mesh proxies */
		std::unordered_set<e2::MeshProxy*> m_meshProxies;

		/** Submeshes ordered by render layer */
		std::map<e2::RenderLayer, std::unordered_set<MeshProxySubmesh>> m_submeshIndex;

		std::unordered_set<e2::MaterialProxy*> m_materialProxies;

		std::unordered_map<e2::Material*, e2::MaterialProxy*> m_defaultMaterialProxies;
	};
}

template <>
struct std::hash<e2::MeshProxySubmesh>
{
	std::size_t operator()(const e2::MeshProxySubmesh& k) const
	{
		std::size_t res = 17;
		res = res * 31 + std::hash<e2::MeshProxy*>{}(k.proxy);
		res = res * 31 + std::hash<uint8_t>{}(k.submesh);
		return res;
	}
};