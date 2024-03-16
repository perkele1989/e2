
#include "editor/importers/ufbximporter.hpp"

#include "e2/assets/asset.hpp"
#include "e2/utils.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/ui/uicontext.hpp"


#include "ufbx.h"

#include <filesystem>
#include <format>
#include <thread>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace fs = std::filesystem;
namespace
{
	glm::quat glmQuat(ufbx_quat const& from)
	{
		//return glm::quat(from.w, -from.x, -from.z, from.y);
		return glm::quat(from.w, from.x, from.y, from.z);
	}

	glm::vec3 glmVec3(ufbx_vec3 const& from)
	{
		//return { -from.x, -from.z, from.y };
		return { from.x, from.y, from.z };
	}

	glm::mat4 glmMat4(ufbx_matrix const& from)
	{
		return glm::mat4x3(
			glmVec3(from.cols[0]),
			glmVec3(from.cols[1]),
			glmVec3(from.cols[2]),
			glmVec3(from.cols[3])
		);
	}

}



e2::UfbxImporter::UfbxImporter(e2::Editor* editor, UfbxImportConfig const& config)
	: e2::Importer(editor)
	, m_config(config)
{

	std::filesystem::path inputPath = std::filesystem::path(m_config.input);
	std::filesystem::path folderPath = inputPath.parent_path();

	if (inputPath.extension() == ".mesh")
	{
		std::string linesStr;
		if (!e2::readFile(m_config.input, linesStr))
		{
			LogError("failed to read mesh file");
			return;
		}
		std::vector<std::string> lines = e2::split(linesStr, '\n');
		for (std::string& l : lines)
		{
			std::string trimmed = e2::trim(l);
			std::vector<std::string> parts = e2::split(trimmed, ' ');

			if (parts.size() < 2)
				continue;

			std::string cmd = e2::toLower(parts[0]);
			if (cmd == "source" && parts.size() == 2)
			{
				m_config.input = (folderPath / parts[1]).string();
			}
			if (cmd == "material" && parts.size() == 3)
			{
				m_config.materialMappings[parts[1]] = parts[2];
			}
		}
	}

	std::string inputName = std::filesystem::path(m_config.input).filename().string();

	m_title = std::format("Import {}...##import_mesh_{}", inputName, (uint64_t)this);
	
	startAnalyzing();
}

e2::UfbxImporter::~UfbxImporter()
{
	if(m_ufbxScene)
		ufbx_free_scene(m_ufbxScene);
}

void e2::UfbxImporter::update(double seconds)
{
	e2::Importer::update(seconds);

	std::string inputName = std::filesystem::path(m_config.input).filename().string();

	e2::UIContext* ui = uiContext();

	const e2::Name id_stackV("mi_stackV");
	const e2::Name id_Writing("mi_writing");
	const e2::Name id_Analyzing("mi_analyzing");
	const e2::Name id_Success("mi_success");
	const e2::Name id_Fail("mi_fail");
	const e2::Name id_BtnImport("mi_btnImport");
	const e2::Name id_ChkSkeleton("mi_chkSkeleton");
	const e2::Name id_NoSkeleton("mi_noSkeleton");
	const e2::Name id_ChkMesh("mi_chkMesh");
	const e2::Name id_NoMesh("mi_noMesh");

	ui->beginStackV(id_stackV);

	if (m_status == UfbxImportStatus::Writing)
	{
		ui->label(id_Writing, "Writing..");
	}
	else if (m_status == UfbxImportStatus::Analyzing)
	{
		ui->label(id_Analyzing, std::format("Analyzing file {}..", inputName));
	}
	else if (m_status == UfbxImportStatus::Ready)
	{
		if (m_mesh.hasSkeleton)
		{
			ui->checkbox(id_ChkSkeleton, m_mesh.importSkeleton, std::format("**Skeleton:** {}", m_mesh.skeletonName));
		}
		else
		{
			ui->label(id_NoSkeleton, "No skeleton.");
		}

		if (m_mesh.hasMesh)
		{
			ui->checkbox(id_ChkMesh, m_mesh.importMesh, std::format("**Mesh:** ({})", m_mesh.meshName));

			uint32_t i = 0;
			for (UfbxImportSubmesh& submesh : m_mesh.submeshes)
			{
				const e2::Name normals("nm");
				const e2::Name smName("subname");
				const e2::Name stackH("stackH");
				const e2::Name modeIgnore("ignore");
				const e2::Name modeGenerate("generate");
				const e2::Name modeImport("import");

				const e2::Name impcol("col");
				const e2::Name impuv0("uv0");
				const e2::Name impuv1("uv1");
				const e2::Name impuv2("uv2");
				const e2::Name impuv3("uv3");

				const e2::Name impw("w");

				ui->pushId(std::format("submesh{}", i++));

				ui->label(smName, std::format("**Submesh** *{}*:", submesh.materialName));

				if (submesh.hasNormals)
				{
					ui->checkbox(impcol, submesh.importNormals, "Import Normals");
				}


				if (submesh.hasColors)
				{
					ui->checkbox(impcol, submesh.importColors, "Import Colors");
				}

				if (submesh.hasUv)
				{
					ui->checkbox(impuv0, submesh.importUv, "Import Uv");
				}

				if (submesh.hasWeights)
				{
					ui->checkbox(impw, submesh.importWeights, "Import Weights");
				}

				ui->popId();

			}
		}
		else
		{
			ui->label(id_NoMesh, "No mesh.");
		}

		if (m_animations.size() > 0)
		{
			for (UfbxImportAnimation& anim : m_animations)
			{
				ui->checkbox(std::format("animation_{}", anim.name), anim.import, std::format("**Animation:** {} ({} fps, {} frames, {} channels)", anim.name, anim.framesPerSecond, anim.duration, anim.channels.size()));
			}
		}
		else
		{
			ui->label(id_NoSkeleton, "No animations.");
		}



		if (ui->button(id_BtnImport, "Import"))
		{
			startProcessing();
		}
	}
	else if (m_status == UfbxImportStatus::Success)
	{
		ui->label(id_Success, std::format("Successfully imported {}", inputName));
	}
	else if (m_status == UfbxImportStatus::Failed)
	{
		ui->label(id_Fail, std::format("Failed to import {}: {}", inputName, m_failReason));
	}

	ui->endStackV();

}

bool e2::UfbxImporter::analyze()
{
	ufbx_load_opts opts = {  }; // Optional, pass NULL for defaults
	opts.reverse_winding = true;
	//opts.target_unit_meters = 1;
	/*opts.space_conversion = UFBX_SPACE_CONVERSION_MODIFY_GEOMETRY;
	opts.target_axes.front = UFBX_COORDINATE_AXIS_NEGATIVE_Z;
	opts.target_axes.up = UFBX_COORDINATE_AXIS_NEGATIVE_Y;
	opts.target_axes.right = UFBX_COORDINATE_AXIS_POSITIVE_X;
	opts.clean_skin_weights = true;*/

	ufbx_error error; // Optional, pass NULL if you don't care about errors
	m_ufbxScene = ufbx_load_file(m_config.input.c_str(), &opts, &error);

	

	if (!m_ufbxScene)
	{
		LogError("ufbx: {}", error.description.data);
		m_failReason = error.description.data;
		return false;
	}

	// Use and inspect `scene`, it's just plain data!

	// Let's just list all objects within the scene for example:
	for (size_t i = 0; i < m_ufbxScene->nodes.count; i++) {
		ufbx_node* node = m_ufbxScene->nodes.data[i];

		//LogNotice("Object: {}", node->name.data);
		if (node->mesh && !m_ufbxMeshNode)
		{
			LogNotice("Mesh: {}", node->name.data);
			m_ufbxMeshNode = node;
			m_ufbxMesh = node->mesh;
		}
		
		if (node->attrib && node->attrib_type == UFBX_ELEMENT_BONE)
		{
			LogNotice("Bone: {}", node->name.data);
		}

		if (node->attrib && node->attrib_type == UFBX_ELEMENT_POSE)
		{
			LogNotice("Pose: {}", node->name.data);
		}

		if (node->attrib && node->attrib_type == UFBX_ELEMENT_SKIN_DEFORMER)
		{
			LogNotice("Skeleton: {}", node->name.data);
		}

	}

	// find topmost bone and set that as root, then find other bones from it 

	ufbx_node* root = m_ufbxScene->root_node;

	std::queue<ufbx_node*> q;
	q.push(root);
	while (!q.empty())
	{
		ufbx_node* n = q.front();
		q.pop();

		if (n->attrib && n->attrib_type == UFBX_ELEMENT_BONE)
		{
			m_mesh.skeleton.ufbxRootNode = n;
			break;
		}

		for (ufbx_node* c : n->children)
		{
			q.push(c);
		}
	}

	m_mesh.hasSkeleton = m_mesh.skeleton.ufbxRootNode != nullptr;
	m_mesh.importSkeleton = m_mesh.hasSkeleton;

	m_mesh.hasMesh = m_ufbxMeshNode != nullptr;
	m_mesh.importMesh = m_mesh.hasMesh;
	m_mesh.meshName = m_ufbxMeshNode->name.data;

	if (m_mesh.meshName.size() == 0)
	{
		m_mesh.meshName = "New";
	}

	std::string prefix = "SM_";

	if (m_mesh.meshName.size() < prefix.size() || m_mesh.meshName.substr(0, prefix.size()) != prefix)
		m_mesh.meshName = prefix + m_mesh.meshName;

	if (m_mesh.skeletonName.size() == 0)
	{
		m_mesh.skeletonName = "SK_" + m_mesh.meshName.substr(3);
	}


	if (m_mesh.hasSkeleton)
	{
		std::queue<ufbx_node*> q;
		q.push(m_mesh.skeleton.ufbxRootNode);
		while (!q.empty())
		{
			ufbx_node* n = q.front();
			q.pop();

			bool isBone = n->attrib && n->attrib_type == UFBX_ELEMENT_BONE;
			if (!isBone)
				continue;

			ufbx_skin_cluster* bone_cluster{};

			if (m_ufbxMesh->skin_deformers.count > 0)
			{
				for (ufbx_skin_cluster* c : m_ufbxMesh->skin_deformers[0]->clusters)
				{
					if (c->bone_node == n)
					{
						bone_cluster = c;
						break;
					}
				}
			}

			uint32_t currIndex = m_mesh.skeleton.bones.size();
			m_mesh.skeleton.nameIndex[std::string(n->name.data)] = currIndex;
			m_mesh.skeleton.nodeIndex[n] = currIndex;
			m_mesh.skeleton.clusterIndex[bone_cluster] = currIndex;

			UfbxImportBone newBone;
			newBone.node = n;
			newBone.cluster = bone_cluster;
			newBone.isRoot = n == m_mesh.skeleton.ufbxRootNode;
			newBone.id = currIndex;
			newBone.name = n->name.data;

			newBone.bindMatrix = ::glmMat4(bone_cluster->geometry_to_bone);

			newBone.localTransform = ::glmMat4(n->node_to_parent);
			m_mesh.skeleton.bones.push_back(newBone);

			for (ufbx_node* c : n->children)
			{
				q.push(c);
			}
		}
		// resolve parent ids
		for (UfbxImportBone& bone : m_mesh.skeleton.bones)
		{
			auto finder = m_mesh.skeleton.nodeIndex.find(bone.node->parent);
			bone.parentId = bone.isRoot || finder == m_mesh.skeleton.nodeIndex.end() ? -1 : finder->second;
		}

	}


	if (m_mesh.hasMesh)
	{

		for (uint32_t i = 0; i < m_ufbxMesh->material_parts.count; i++)
		{
			e2::UfbxImportSubmesh submesh;
			submesh.ufbxSubmesh = &m_ufbxMesh->material_parts.data[i];
			submesh.materialName = m_ufbxMesh->materials.count > 0 ? m_ufbxMesh->materials[i]->name.data : "M_Default";
			submesh.hasNormals = m_ufbxMesh->vertex_normal.exists;
			submesh.importNormals = submesh.hasNormals;

			submesh.hasColors = m_ufbxMesh->vertex_color.exists;
			submesh.importColors = submesh.hasColors;

			submesh.hasUv = m_ufbxMesh->vertex_uv.exists;
			submesh.importUv = submesh.hasUv;

			submesh.hasWeights = m_ufbxMesh->skin_deformers.count > 0;
			submesh.importWeights = submesh.hasWeights;

			m_mesh.submeshes.push_back(submesh);
		}
	}


	for (uint32_t i = 0; i < m_ufbxScene->anim_stacks.count; i++)
	{
		ufbx_anim_stack* animStack = m_ufbxScene->anim_stacks.data[i];
		e2::UfbxImportAnimation newAnim;

		// hardcode framerate
		float durationSecs = animStack->time_end - animStack->time_begin;

		newAnim.duration = glm::clamp((uint32_t)glm::ceil(durationSecs * 24.0f), (uint32_t)2, (uint32_t)e2::maxNumTrackFrames);
		newAnim.framesPerSecond = ((float)newAnim.duration - 1) / durationSecs;
		newAnim.name = animStack->name.data;
		std::vector<std::string> parts = e2::split(newAnim.name, '|');
		if (parts.size() > 1)
			newAnim.name = parts[1];
		newAnim.import = true;

		for (e2::UfbxImportBone& bone : m_mesh.skeleton.bones)
		{
			ufbx_node* node = bone.node;

			UfbxImportChannel channel;

			for (size_t i = 0; i < newAnim.duration; i++)
			{
				float time = animStack->time_begin + float(i) / newAnim.framesPerSecond;
				ufbx_transform transform = ufbx_evaluate_transform(animStack->anim, node, time);

				UfbxVec3Key posKey;
				posKey.time = time;
				posKey.vector = ::glmVec3(transform.translation);
				channel.positionKeys.push_back(posKey);

				UfbxQuatKey rotKey;
				rotKey.time = time;
				rotKey.quat = ::glmQuat(transform.rotation);

				channel.rotationKeys.push_back(rotKey);

			}
			newAnim.channels[bone.name] = channel;
		}

		m_animations.push_back(newAnim);

	}


	return true;
}


uint32_t e2::UfbxImportSubmesh::newVertex(ufbx_mesh* fromMesh, uint32_t ufbxIndex)
{
	auto finder = ufbxVertexIdMap.find(ufbxIndex);
	if (finder != ufbxVertexIdMap.end())
	{
		return finder->second;
	}

	uint32_t newIndex = newVertices.size();
	e2::Vertex newVertex;

	ufbx_vec3 pos = ufbx_get_vertex_vec3(&fromMesh->vertex_position, ufbxIndex);
	newVertex.position = ::glmVec3(pos);

	if (hasNormals)
	{
		ufbx_vec3 nrm = ufbx_get_vertex_vec3(&fromMesh->vertex_normal, ufbxIndex);
		newVertex.normal = ::glmVec3(nrm);

		if (fromMesh->vertex_tangent.exists)
		{
			ufbx_vec3 tng = ufbx_get_vertex_vec3(&fromMesh->vertex_tangent, ufbxIndex);
			newVertex.tangent = ::glmVec3(tng);

			if (fromMesh->vertex_bitangent.exists)
			{
				ufbx_vec3 bit = ufbx_get_vertex_vec3(&fromMesh->vertex_bitangent, ufbxIndex);
				glm::vec3 bitangent = ::glmVec3(bit);
				newVertex.tangentSign = glm::sign(glm::dot(glm::cross(newVertex.normal, newVertex.tangent), bitangent));
			}
		}

	}

	if (hasUv)
	{
		ufbx_vec2 uv = ufbx_get_vertex_vec2(&fromMesh->vertex_uv, ufbxIndex);
		newVertex.uv01.x = uv.x;
		newVertex.uv01.y = uv.y;
	}

	if (hasColors)
	{
		ufbx_vec4 colors = ufbx_get_vertex_vec4(&fromMesh->vertex_color, ufbxIndex);
		newVertex.color.r = colors.x;
		newVertex.color.g = colors.y;
		newVertex.color.b = colors.z;
		newVertex.color.a = colors.w;
	}

	newVertices.push_back(newVertex);
	ufbxVertexIdMap[ufbxIndex] = newIndex;
	ourVertexIdMap[newIndex] = ufbxIndex;
	ufbxVertexIndices.insert(ufbxIndex);
	return newIndex;
}


bool e2::UfbxImporter::writeAssets()
{
	if (m_mesh.hasMesh && m_mesh.importMesh)
	{
		

		// prepare header first, so we can add depedencies to it
		e2::AssetHeader meshHeader;
		meshHeader.version = e2::AssetVersion::Latest;
		meshHeader.assetType = "e2::Mesh";

		e2::StackVector<uint32_t, 256> tmpIndices;

		// Process
		for (e2::UfbxImportSubmesh &submesh : m_mesh.submeshes)
		{
			e2::UfbxImportSubmesh::IntermediateData& im = submesh.intermediate;
			im.indexData = e2::Buffer(false);

			auto matFinder = m_config.materialMappings.find(submesh.materialName);
			if (matFinder == m_config.materialMappings.end())
			{
				im.materialUuid = createMaterial(submesh.materialName);
			}
			else
			{
				im.materialUuid = assetManager()->database().entryFromPath(matFinder->second)->uuid;
				if (!im.materialUuid.valid())
				{
					LogError("material mapping for {} pointed to invalid material: {}", submesh.materialName, matFinder->second);
					return false;
				}
			}

			meshHeader.dependencies.push({ im.materialUuid });



			for (uint32_t faceIt = 0; faceIt < submesh.ufbxSubmesh->face_indices.count; faceIt++)
			{
				uint32_t faceIndex = submesh.ufbxSubmesh->face_indices.data[faceIt];
				ufbx_face* f = &m_ufbxMesh->faces.data[faceIndex];
				//if (f->num_indices != 3)
				//	continue;


				uint32_t num_tris = ufbx_triangulate_face(tmpIndices.data(), 256, m_ufbxMesh, *f);

				for (uint32_t tri = 0; tri < num_tris; tri++)
				{
					uint32_t a = submesh.newVertex(m_ufbxMesh, tmpIndices[tri * 3 + 0]);
					uint32_t b = submesh.newVertex(m_ufbxMesh, tmpIndices[tri * 3 + 1]);
					uint32_t c = submesh.newVertex(m_ufbxMesh, tmpIndices[tri * 3 + 2]);

					submesh.newIndices.push_back(a);
					submesh.newIndices.push_back(b);
					submesh.newIndices.push_back(c);
				}
			}

			im.indexCount = submesh.newIndices.size();
			im.vertexCount = submesh.newVertices.size();


			for (uint32_t index : submesh.newIndices)
				im.indexData << index;

			// setup flags 
			bool useNormals = submesh.hasNormals && submesh.importNormals;
			
			bool useUv = submesh.hasUv && submesh.importUv;
			bool useColors = submesh.hasColors && submesh.importColors;
			bool useWeights = submesh.hasWeights && submesh.importWeights;

			if (useNormals)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Normal;
			if(useUv)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::TexCoords01;
			if (useColors)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Color;
			if (useWeights)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Bones;


			// handle positions
			UfbxImportAttribute attribPos{};
			
			attribPos.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
			for (uint32_t i = 0; i < im.vertexCount; i++)
			{
				glm::vec3 pos = submesh.newVertices[i].position;
				attribPos.vertexData << pos << 1.0f;
			}

			im.attributes.push_back(attribPos);

			// handle normals
			if (useNormals)
			{
				UfbxImportAttribute attribNormals{};
				attribNormals.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					glm::vec3 nrm = submesh.newVertices[i].normal;
					attribNormals.vertexData << nrm << 0.0f;
				}

				im.attributes.push_back(attribNormals);

				UfbxImportAttribute attribTangents{};
				attribTangents.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					glm::vec3 tan = submesh.newVertices[i].tangent;
					attribTangents.vertexData << tan << submesh.newVertices[i].tangentSign;
				}

				im.attributes.push_back(attribTangents);
			}


			// handle uv01
			if (useUv)
			{

				UfbxImportAttribute attribUv01{};
				attribUv01.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					attribUv01.vertexData << submesh.newVertices[i].uv01;
				}
				im.attributes.push_back(attribUv01);
			}

			// handle colors
			if (useColors)
			{
				UfbxImportAttribute attribColors{};
				attribColors.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					attribColors.vertexData << submesh.newVertices[i].color;
				}
				im.attributes.push_back(attribColors);
			}

			if (useWeights)
			{
				// Having multiple skin deformers attached at once is exceedingly rare so we can just
				// pick the first one without having to worry too much about it.
				ufbx_skin_deformer* ufbxSkin = m_ufbxMesh->skin_deformers.data[0];

				for (uint32_t ourIndex = 0; ourIndex < im.vertexCount; ourIndex++)
				{
					auto ff = submesh.ourVertexIdMap.find(ourIndex);
					if (ff == submesh.ourVertexIdMap.end())
					{
						continue;
					}

					uint32_t ufbxIndex = ff->second;
					uint32_t ii = m_ufbxMesh->vertex_indices.data[ufbxIndex];

					ufbx_skin_vertex* skin_vert = &ufbxSkin->vertices[ii];
					uint32_t ww = 0;
					for (uint32_t w = 0; w < skin_vert->num_weights; w++)
					{
						if (ww >= 4)
						{
							break;
						}
						uint32_t wi = skin_vert->weight_begin + w;
						ufbx_skin_weight* skin_weight = &ufbxSkin->weights[wi];
						ufbx_skin_cluster* cluster = ufbxSkin->clusters[skin_weight->cluster_index];

						auto finder = m_mesh.skeleton.clusterIndex.find(cluster);
						if (finder == m_mesh.skeleton.clusterIndex.end())
							continue;

						uint32_t boneIndex = finder->second;
						float boneWeight = skin_weight->weight;
						submesh.newVertices[ourIndex].bones[ww] = boneIndex;
						submesh.newVertices[ourIndex].weights[ww] = boneWeight;
						ww++;
					}
				}

				UfbxImportAttribute attribWeights;
				attribWeights.vertexData = e2::Buffer(false, sizeof(glm::vec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					attribWeights.vertexData << submesh.newVertices[i].weights;
				}

				im.attributes.push_back(attribWeights);

				UfbxImportAttribute attribIds;
				attribIds.vertexData = e2::Buffer(false, sizeof(glm::uvec4) * im.vertexCount);
				for (uint32_t i = 0; i < im.vertexCount; i++)
				{
					attribIds.vertexData << submesh.newVertices[i].bones;
				}

				im.attributes.push_back(attribIds);
			}


		}

		// write the data 
		e2::Buffer meshData; 

		meshData << uint32_t(m_mesh.skeleton.bones.size());
		for (uint32_t i = 0; i < m_mesh.skeleton.bones.size(); i++)
		{
			meshData << std::string(m_mesh.skeleton.bones[i].name);
			meshData << uint32_t(i);
		}

		meshData << uint32_t(m_mesh.submeshes.size());
		for (e2::UfbxImportSubmesh &submesh : m_mesh.submeshes)
		{

			e2::UfbxImportSubmesh::IntermediateData& im = submesh.intermediate;
			meshData << im.vertexCount;
			meshData << im.indexCount;
			meshData << uint8_t(im.attributeFlags);
			meshData << uint32_t(im.indexData.size());
			meshData.write(im.indexData.begin(), im.indexData.size());
			for (UfbxImportAttribute const& attr : im.attributes)
			{
				meshData << uint32_t(attr.vertexData.size());
				meshData.write(attr.vertexData.begin(), attr.vertexData.size());
			}
		}

		meshHeader.size = meshData.size();

		e2::Buffer fileBuffer(true, 1024 + meshData.size());
		fileBuffer << meshHeader;
		fileBuffer.write(meshData.begin(), meshData.size());

		std::string outFile =(fs::path(m_config.outputDirectory) / (m_mesh.meshName + ".e2a")).string();
		if (fileBuffer.writeToFile(outFile))
		{
			auto newRef = assetManager()->database().invalidateAsset(outFile);
			assetManager()->database().validate(true);
		}
		else
		{
			LogError("Failed to write file ./Assets/{}", outFile);
		}
	}

	if (m_mesh.hasSkeleton && m_mesh.importSkeleton)
	{
		e2::AssetHeader skeletonHeader;
		skeletonHeader.version = e2::AssetVersion::Latest;
		skeletonHeader.assetType = "e2::Skeleton";

		// write the data 
		e2::Buffer skeletonData;

		skeletonData << uint32_t(m_mesh.skeleton.bones.size());
		for (uint32_t i = 0; i < m_mesh.skeleton.bones.size(); i++)
		{
			e2::UfbxImportBone& b = m_mesh.skeleton.bones[i];
			LogNotice("Writing bone {} {} with parent {}", i, b.name, b.parentId);

			skeletonData << std::string(b.name);
			skeletonData << glm::mat4(b.bindMatrix);
			skeletonData << glm::mat4(b.localTransform);
			skeletonData << int32_t(b.parentId);
		}

		skeletonHeader.size = skeletonData.size();

		e2::Buffer fileBuffer(true, 1024 + skeletonData.size());
		fileBuffer << skeletonHeader;
		fileBuffer.write(skeletonData.begin(), skeletonData.size());

		std::string outFile = (fs::path(m_config.outputDirectory) / (m_mesh.skeletonName + ".e2a")).string();
		if (fileBuffer.writeToFile(outFile))
		{
			auto newRef = assetManager()->database().invalidateAsset(outFile);
			assetManager()->database().validate(true);
		}
		else
		{
			LogError("Failed to write file ./Assets/{}", outFile);
		}
	}

	for (UfbxImportAnimation& anim : m_animations)
	{
		if (!anim.import)
			continue;

		e2::AssetHeader animHeader;
		animHeader.version = e2::AssetVersion::Latest;
		animHeader.assetType = "e2::Animation";

		// write the data 
		e2::Buffer animData;


		animData << uint32_t(anim.duration);
		animData << float(anim.framesPerSecond);
		animData << uint32_t(anim.channels.size() * 2);

		for (auto& pair : anim.channels)
		{
			std::string channelName = pair.first + ".position";
			UfbxImportChannel& channel = pair.second;

			animData << channelName;

			animData << uint8_t(2);//vec3

			for (uint32_t frameIndex = 0; frameIndex < anim.duration; frameIndex++)
			{
				float time = (float(frameIndex) / float(anim.duration)) * (float(anim.duration) / anim.framesPerSecond);
				glm::vec3 pos = channel.samplePosition(time);
				animData << pos.x;
				animData << pos.y;
				animData << pos.z;
				animData << 0.0f;
			}
		}

		for (auto& pair : anim.channels)
		{
			std::string channelName = pair.first + ".rotation";
			UfbxImportChannel& channel = pair.second;

			animData << channelName;

			animData << uint8_t(3);//quat

			for (uint32_t frameIndex = 0; frameIndex < anim.duration; frameIndex++)
			{
				float time = (float(frameIndex) / float(anim.duration)) * (float(anim.duration) / anim.framesPerSecond);
				glm::quat rot = channel.sampleRotation(time);
				animData << rot.x;
				animData << rot.y;
				animData << rot.z;
				animData << rot.w;
			}
		}


		animHeader.size = animData.size();

		e2::Buffer fileBuffer(true, 1024 + animData.size());
		fileBuffer << animHeader;
		fileBuffer.write(animData.begin(), animData.size());

		std::string outFile = (fs::path(m_config.outputDirectory) / (anim.name + ".e2a")).string();
		if (fileBuffer.writeToFile(outFile))
		{
			auto newRef = assetManager()->database().invalidateAsset(outFile);
			assetManager()->database().validate(true);
		}
		else
		{
			LogError("Failed to write file ./Assets/{}", outFile);
		}
	}

	return true;
}

e2::UUID e2::UfbxImporter::createMaterial(std::string const& outName)
{
	std::string outFile = (fs::path(m_config.outputDirectory) / (outName + ".e2a")).string();

	e2::AssetHeader materialHeader;
	materialHeader.version = e2::AssetVersion::Latest;
	materialHeader.assetType = "e2::Material";

	e2::Buffer materialData;
	e2::Name shaderModel = "e2::LightweightModel";
	materialData << shaderModel.string();

	// numdefines
	materialData << uint8_t(0);
	// numvectors 
	materialData << uint8_t(0);
	// numtextures
	materialData << uint8_t(0);


	materialHeader.size = materialData.size();

	e2::Buffer fileBuffer(true, 1024 + materialData.size());
	fileBuffer << materialHeader;
	fileBuffer.write(materialData.begin(), materialData.size());
	if (fileBuffer.writeToFile(outFile))
	{
		return assetManager()->database().invalidateAsset(outFile);
	}
	else
	{
		LogError("Failed to create material, could not write to file: {}", outFile);
		return e2::UUID();
	}
}

void e2::UfbxImporter::startAnalyzing()
{
	m_status = UfbxImportStatus::Analyzing;
	std::thread analyzeThread([this]() {

		if (!analyze())
		{
			m_status = UfbxImportStatus::Failed;
		}
		else
		{
			m_status = UfbxImportStatus::Ready;
		}
		});
	analyzeThread.detach();
}

void e2::UfbxImporter::startProcessing()
{
	m_status = UfbxImportStatus::Writing;
	std::thread processingThread([this]() {

		if (!writeAssets())
		{
			m_status = UfbxImportStatus::Failed;
		}

		m_status = UfbxImportStatus::Success;

	});
	processingThread.detach();
}
glm::vec3 e2::UfbxImportChannel::samplePosition(float time)
{
	for (int32_t ki = 0; ki < positionKeys.size(); ki++)
	{
		if (positionKeys[ki].time == time)
			return positionKeys[ki].vector;
	}

	int32_t frameA = 0;
	int32_t frameB = 0;

	for (int32_t ki = 0; ki < positionKeys.size(); ki++)
	{
		if (positionKeys[ki].time > time)
		{
			frameB = ki;
			break;
		}
	}

	frameA = frameB - 1;
	if (frameA < 0)
	{
		return positionKeys[frameB].vector;
	}
	float timeA = positionKeys[frameA].time;
	float timeB = positionKeys[frameB].time;

	float frameDelta = (time - timeA) / (timeB - timeA);

	return glm::mix(positionKeys[frameA].vector, positionKeys[frameB].vector, frameDelta);
}

glm::quat e2::UfbxImportChannel::sampleRotation(float time)
{
	for (int32_t ki = 0; ki < rotationKeys.size(); ki++)
	{
		if (rotationKeys[ki].time == time)
			return rotationKeys[ki].quat;
	}

	int32_t frameA = 0;
	int32_t frameB = 0;

	for (int32_t ki = 0; ki < rotationKeys.size(); ki++)
	{
		if (rotationKeys[ki].time > time)
		{
			frameB = ki;
			break;
		}
	}

	frameA = frameB - 1;
	if (frameA < 0)
	{
		return rotationKeys[frameB].quat;
	}
	float timeA = rotationKeys[frameA].time;
	float timeB = rotationKeys[frameB].time;

	float frameDelta = (time - timeA) / (timeB - timeA);

	return glm::slerp(rotationKeys[frameA].quat, rotationKeys[frameB].quat, frameDelta);
}
