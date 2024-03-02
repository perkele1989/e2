
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>

#include <tuple>

namespace e2
{
	/*
	 * Dynamic mesh
	 * Mesh representation designed for programmatic use
	 * Can be serialized to/from memory/file
	 * Can be baked into a regular mesh, either by saving as asset, or runtime instancing
	 * Not well optimized for heavy runtime use 
	 */

	E2_API glm::vec3 snapToSteps(glm::vec3 const& in, float stepNumerator, float stepDenominator);

	/** */
	struct E2_API Vertex
	{
		glm::vec3 position{};
		glm::vec3 normal{};
		glm::vec3 tangent{};

		float tangentSign{1.0f};

		glm::vec4 uv01{};
		glm::vec4 uv23{};
		glm::vec4 color{};
		glm::vec4 weights{};
		glm::uvec4 bones{};

		auto as_tuple() const
		{
			glm::vec3 granularPosition = snapToSteps(position, 2, 10000);
			glm::vec3 granularNormal = snapToSteps(normal, 2, 1000);
			glm::vec3 granularTangent = snapToSteps(tangent, 2, 1000);
			bool granularSign = tangentSign > 0.0f;


			return std::make_tuple(
				granularPosition.x, granularPosition.y, granularPosition.z,
				granularNormal.x, granularNormal.y, granularNormal.z,
				granularTangent.x, granularTangent.y, granularTangent.z,
				granularSign,
				uv01.x, uv01.y, uv01.z, uv01.w,
				uv23.x, uv23.y, uv23.z, uv23.w,
				color.x, color.y, color.z, color.w,
				weights.x, weights.y, weights.z, weights.w,
				bones.x, bones.y, bones.z, bones.w);

			/*return std::tie(
				granularPosition.x, granularPosition.y, granularPosition.z,
				granularNormal.x, granularNormal.y, granularNormal.z,
				granularTangent.x, granularTangent.y, granularTangent.z,
				granularSign,
				uv01.x, uv01.y, uv01.z, uv01.w,
				uv23.x, uv23.y, uv23.z, uv23.w,
				color.x, color.y, color.z, color.w,
				weights.x, weights.y, weights.z, weights.w,
				bones.x, bones.y, bones.z, bones.w);*/
		}

		bool operator<(const Vertex& o) const
		{
			// We dont use tangents here, since we recalculate them after merging vertices anyway
			return as_tuple() < o.as_tuple();
		}

	};

	/**  */
	struct E2_API Triangle
	{
		union {
			struct {
				uint32_t a;
				uint32_t b;
				uint32_t c;
			};

			uint32_t vertexIds[3] = { 0, 0, 0};
		};

		glm::vec3 faceNormal{};
	};

	using VertexShaderFunction = void (*) (e2::Vertex *vertex, void* shaderFuncData);

	/** @tags(arena, arenaSize=e2::maxNumDynamicMeshes) */
	class E2_API DynamicMesh : public e2::Object
	{
		ObjectDeclaration()
	public:
		/** 
		 * Creates a dmesh from a mesh asset
		 * This is a slow operation with many allocations
		 * If you need to reuse the same mesh for many dmeshes, create a single dmesh and then reuse that to initialize the others
		 */
		DynamicMesh(e2::MeshPtr mesh, uint8_t submeshIndex, e2::VertexAttributeFlags requestedFlags = VertexAttributeFlags::All);
		DynamicMesh();
		virtual ~DynamicMesh();


		e2::MeshPtr bake(e2::MaterialPtr material, e2::VertexAttributeFlags attributeFlags);


		void reserve(uint32_t numVertices, uint32_t numTriangles);

		void cutSlack();

		/** Adds an entire other mesh to this mesh */
		void addMesh(e2::DynamicMesh const* other, glm::mat4 const& transform = glm::identity<glm::mat4>());

		/** Adds an entire other mesh to this mesh, with a shader function */
		void addMeshWithShaderFunction(e2::DynamicMesh const* other, glm::mat4 const& transform, VertexShaderFunction shaderFunc, void* shaderFuncData);

		/** Adds a vertex, returns its ID */
		uint32_t addVertex(Vertex const& newVertex);

		/** Adds a triangle, returns its ID */
		uint32_t addTriangle(Triangle const& newTriangle);

		/** Adds 3 new vertices and a triangle, returns its ID */
		uint32_t addTriangle(Vertex const& vertA, Vertex const& vertB, Vertex const& vertC);

		/** Swaps rhs with this */
		void swap(DynamicMesh& rhs);

		void calculateFaceNormals();

		/** Make sure you calculate or have face normals before calling this */
		void calculateVertexNormals();

		/** Make sure you calculate or have vertex normals before calling this */
		bool calculateVertexTangents();

		/** Merge duplicate vertices in this mesh */
		void mergeDuplicateVertices();

		inline uint32_t numVertices() const
		{
			return (uint32_t)m_vertices.size();
		}

		inline uint32_t numTriangles() const
		{
			return (uint32_t)m_triangles.size();
		}

		inline Vertex& getVertex(uint32_t id)
		{
			return m_vertices[id];
		}

		inline Triangle& getTriangle(uint32_t id)
		{
			return m_triangles[id];
		}


	protected:

		uint32_t m_vertexOffset{};
		uint32_t m_triangleOffset{};

		std::vector<Vertex> m_vertices;
		std::vector<Triangle> m_triangles;
	};


}

#include "dmesh.generated.hpp"