
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/assets/asset.hpp>
#include <e2/assets/material.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/renderer/meshspecification.hpp>

namespace e2
{

	struct E2_API ProceduralSubmesh
	{
		e2::MaterialPtr material;
		e2::VertexAttributeFlags attributes{ e2::VertexAttributeFlags::None };
		uint32_t numIndices{};
		uint32_t numVertices{};
		uint32_t* sourceIndices{}; // never null
		glm::vec4* sourcePositions{}; // never null
		glm::vec4* sourceNormals{}; // null if attributes doesn't contain Normal
		glm::vec4* sourceTangents{};// null if attributes doesn't contain Normal
		glm::vec4* sourceUv01{};// null if attributes doesn't contain Uv01
		glm::vec4* sourceUv23{};// null if attributes doesn't contain Uv23
		glm::vec4* sourceColors{};// null if attributes doesn't contain Color
		glm::vec4* sourceWeights{};// null if attributes doesn't contain Bones
		glm::uvec4* sourceBones{};// null if attributes doesn't contain Bones
	};


	/** @tags(dynamic, arena, arenaSize=e2::maxNumMeshAssets) */
	class E2_API Mesh : public e2::Asset
	{
		ObjectDeclaration()
	public: 
		Mesh();
		virtual ~Mesh();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		e2::SubmeshSpecification const& specification(uint8_t subIndex);

		e2::MaterialPtr material(uint8_t subIndex) const;

		uint8_t submeshCount() const;

		/**
		 * Procedural generation steps:
		 * 1. Create a clean mesh: auto mesh = e2::create<e2::Mesh>();
		 * 2. Add your procedurally generated submeshes: mesh->addProceduralSubmesh(pikachu);
		 * 3. When you've added the submeshes you want, invoke flagDone() to notify the systems that use this mesh that it's in a usable state.
		 */
		void addProceduralSubmesh(e2::ProceduralSubmesh const& sourceData);
		void flagDone();

		inline bool isDone() const
		{
			return m_done;
		}

	protected:
		e2::StackVector<e2::SubmeshSpecification, e2::maxNumSubmeshes> m_specifications;
		e2::StackVector<e2::MaterialPtr, e2::maxNumSubmeshes> m_materials;

		bool m_done{};
	};

}

#include "mesh.generated.hpp"