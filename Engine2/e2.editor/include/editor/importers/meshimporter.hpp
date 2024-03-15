
#pragma once 

#include <editor/importer.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/renderer/meshspecification.hpp>
#include <e2/utils.hpp>
#include <e2/export.hpp>

#include <vector>
#include <map>

namespace Assimp
{
	class Importer;
}

struct aiMesh;
struct aiMaterial;
struct aiScene;

namespace e2
{

	struct MeshImportConfig
	{
		std::string input;
		std::string outputDirectory{"./assets/"};

		std::unordered_map<std::string, std::string> materialMappings;
	};

	enum ImportAttributeMode : uint32_t
	{
		IAM_Ignore = 0,
		IAM_Generate,
		IAM_Import
	};

	enum ImportMaterialMode : uint32_t
	{
		IMM_New = 0,
		IMM_Specify
	};

	std::string attributeString(ImportAttributeMode mode);

	static char const* attributeItems[] = {"Ignore", "Generate", "Import"};

	struct ImportAttribute
	{
		e2::Buffer vertexData;
	};

	struct WeightData
	{
		glm::vec4 weights;
		glm::uvec4 ids;
	};


	struct ImportSubmesh
	{
		aiMesh* assMesh{};
		aiMaterial* assMaterial{};

		std::string materialName;
		e2::UUID existingMaterialUUID;

		// this includes tangents
		bool hasNormals{};
		uint32_t normalsMode{IAM_Ignore};

		bool hasColors{};
		bool importColors{};

		bool hasUv0{};
		bool importUv0{};

		bool hasUv1{};
		bool importUv1{};

		bool hasUv2{};
		bool importUv2{};

		bool hasUv3{};
		bool importUv3{};

		bool hasWeights{};
		bool importWeights{};

		std::vector<WeightData> weightData;

		// intermediate data here 

		struct IntermediateData
		{
			e2::UUID materialUuid{};
			uint32_t vertexCount{};
			uint32_t indexCount{};

			e2::VertexAttributeFlags attributeFlags { e2::VertexAttributeFlags::None };
			e2::Buffer indexData;
			std::vector<ImportAttribute> attributes;
		} intermediate;

	};

	struct Vec3Key
	{
		float time;
		glm::vec3 vector;
	};

	struct QuatKey
	{
		float time;
		glm::quat quat;
	};

	struct ImportChannel
	{
		glm::vec3 samplePosition(float time);

		glm::quat sampleRotation(float time);

		std::vector<Vec3Key> positionKeys;
		std::vector<QuatKey> rotationKeys;
	};

	struct ImportAnimation
	{
		bool import{};
		std::string name;
		uint32_t duration{};
		float framesPerSecond{24.0f};
		std::unordered_map<std::string, ImportChannel> channels;
	};

	struct ImportBone
	{
		bool isRoot{};
		uint32_t id{};
		int32_t parentId{};
		std::string name;
		std::string parentName;

		// local bind pose translation
		glm::vec3 localTranslation;

		// local bind pose rotation 
		glm::quat localRotation;

	};

	struct ImportSkeleton
	{
		std::unordered_map<std::string, uint32_t> boneIndex;
		std::vector<ImportBone> bones;
	};

	struct ImportMesh
	{

		Assimp::Importer* assImporter{};
		aiScene const *assScene{};

		bool hasMesh{};
		bool importMesh{};
		std::string meshName;
		std::vector<ImportSubmesh> submeshes;

		bool hasSkeleton{};
		bool importSkeleton{};
		std::string skeletonName;
		ImportSkeleton skeleton;

	};

	enum MeshImportStatus
	{
		MIS_Analyzing,
		MIS_Ready,
		MIS_Writing,
		MIS_Success,
		MIS_Failed
	};

	class MeshImporter : public Importer
	{
	public:
		MeshImporter(e2::Editor *editor, MeshImportConfig const &config);
		virtual ~MeshImporter();

		virtual void update(double seconds) override;

		bool analyze();

		bool writeAssets();
	protected:

		e2::UUID createMaterial(std::string const& outName);

		void startAnalyzing();
		void startProcessing();
		std::string m_failReason;

		MeshImportStatus m_status{ MIS_Analyzing };
		MeshImportConfig m_config;

		ImportMesh m_mesh;

		std::vector<ImportAnimation> m_animations;

		std::string m_title;
	};

}