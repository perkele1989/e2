
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/renderer/meshspecification.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>
#include <e2/dmesh/dmesh.hpp>

#include "ufbx.h"

#include <vector>
#include <map>

struct aiMesh;
struct aiMaterial;
struct aiScene;

namespace e2
{

	struct UfbxImportConfig
	{
		std::string input;
		std::string outputDirectory{"./assets/"};

		std::unordered_map<std::string, std::string> materialMappings;
	};

	struct UfbxImportAttribute
	{
		e2::Buffer vertexData;
	};


	struct UfbxImportSubmesh
	{
		ufbx_mesh_part* ufbxSubmesh{};


		uint32_t newVertex(ufbx_mesh* fromMesh, uint32_t ufbxIndex);

		// Maps ufbx vertex index to vertex index for this submesh (in newvertices)
		std::unordered_map<uint32_t, uint32_t> ufbxVertexIdMap;

		std::unordered_set<uint32_t> ufbxVertexIndices;
		// maps OUR vertex indices for this submesh -> ufbx 
		std::unordered_map<uint32_t, uint32_t> ourVertexIdMap;

		std::vector<e2::Vertex> newVertices;
		std::vector<uint32_t> newIndices;

		std::string materialName;
		e2::UUID existingMaterialUUID;

		// this includes tangents
		bool hasNormals{};
		bool importNormals{};

		bool hasColors{};
		bool importColors{};

		bool hasUv{};
		bool importUv{};

		bool hasWeights{};
		bool importWeights{};

		// intermediate data here 

		struct IntermediateData
		{
			e2::UUID materialUuid{};
			uint32_t vertexCount{};
			uint32_t indexCount{};

			e2::VertexAttributeFlags attributeFlags { e2::VertexAttributeFlags::None };
			e2::Buffer indexData;
			std::vector<UfbxImportAttribute> attributes;
		} intermediate;

	};

	struct UfbxVec3Key
	{
		float time;
		glm::vec3 vector;
	};

	struct UfbxQuatKey
	{
		float time;
		glm::quat quat;
	};

	struct UfbxImportChannel
	{
		glm::vec3 samplePosition(float time);

		glm::quat sampleRotation(float time);

		std::vector<UfbxVec3Key> positionKeys;
		std::vector<UfbxQuatKey> rotationKeys;
	};

	struct UfbxImportAnimation
	{
		bool import{};
		std::string name;
		uint32_t duration{};
		float framesPerSecond{24.0f};
		std::unordered_map<std::string, UfbxImportChannel> channels;
	};

	struct UfbxImportBone
	{
		ufbx_node* node{};
		ufbx_skin_cluster* cluster{};

		bool isRoot{};
		uint32_t id{};
		int32_t parentId{};
		std::string name;
		std::string parentName;

		// geometry-to-bone (is this inverse bind pose?)
		glm::mat4 bindMatrix;

		// node-to-parent
		glm::mat4 localTransform;
	};

	struct UfbxImportSkeleton
	{
		ufbx_node* ufbxRootNode{};
		std::unordered_map<std::string, uint32_t> nameIndex;
		std::unordered_map<ufbx_node*, uint32_t> nodeIndex; 
		std::unordered_map< ufbx_skin_cluster*, uint32_t> clusterIndex;
		std::vector<UfbxImportBone> bones;
	};

	struct UfbxImportMesh
	{

		bool hasMesh{};
		bool importMesh{};
		std::string meshName;
		std::vector<UfbxImportSubmesh> submeshes;

		bool hasSkeleton{};
		bool importSkeleton{};
		std::string skeletonName;
		UfbxImportSkeleton skeleton;



	};

	enum class UfbxImportStatus : uint8_t
	{
		Analyzing,
		Ready,
		Writing,
		Success,
		Failed
	};

	class UfbxImporter : public Importer
	{
	public:
		UfbxImporter(e2::Editor *editor, UfbxImportConfig const &config);
		virtual ~UfbxImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		e2::UUID createMaterial(std::string const& outName);

		void startAnalyzing();
		void startProcessing();
		std::string m_failReason;

		UfbxImportStatus m_status{ UfbxImportStatus::Analyzing };
		UfbxImportConfig m_config;

		ufbx_scene* m_ufbxScene{};
		ufbx_node* m_ufbxMeshNode{};
		ufbx_mesh* m_ufbxMesh{};

		ufbx_node* m_ufbxSkeletonRoot{};
		UfbxImportMesh m_mesh;

		std::vector<UfbxImportAnimation> m_animations;

		std::string m_title;
	};

}