#pragma once 

#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/assets/mesh.hpp>
#include <nlohmann/json.hpp>

namespace e2
{


	class GameContext;

	struct StaticMeshSpecification
	{
		void populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps);
		void finalize(e2::GameContext* ctx);

		glm::vec3 scale;

		/** Mesh asset to use */
		e2::Name meshAssetName;
		e2::MeshPtr meshAsset;
	};

	class Entity;
	class MeshProxy;

	/** @tags(arena, arenaSize=4096) */
	class StaticMeshComponent : public e2::Object
	{
		ObjectDeclaration();
	public:
		StaticMeshComponent(e2::StaticMeshSpecification* specification, e2::Entity* entity);
		virtual ~StaticMeshComponent();

		glm::mat4 getScaleTransform();

		void updateVisibility();

		void applyTransform();

		void applyCustomTransform(glm::mat4 const& transform);
		
		inline void setHeightOffset(float newOffset)
		{
			m_heightOffset = newOffset;
		}

		inline e2::MeshProxy* meshProxy() {
			return m_meshProxy;
		}

	protected:

		e2::Entity* m_entity{};
		e2::StaticMeshSpecification* m_specification{};

		e2::MeshProxy* m_meshProxy{};

		float m_heightOffset{};

	};


}


#include "staticmeshcomponent.generated.hpp"