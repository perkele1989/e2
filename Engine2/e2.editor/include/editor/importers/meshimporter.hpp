
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

	struct ImportSubmesh
	{
		aiMesh* assMesh{};
		aiMaterial* assMaterial{};

		std::map<std::string, uint32_t> boneMap;

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

	struct ImportAnimation
	{

		float framesPerSecond{60.0f};
	};

	struct ImportSkeleton
	{

	};

	struct ImportMesh
	{
		bool isMeshSkinned()
		{
			for (ImportSubmesh const& submesh : submeshes)
				if (submesh.hasWeights)
					return true;
			
			return false;
		}

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

		void displayContent();
		void startAnalyzing();
		void startProcessing();
		std::string m_failReason;

		MeshImportStatus m_status{ MIS_Analyzing };
		MeshImportConfig m_config;

		ImportMesh m_mesh;

		std::string m_title;
	};

}