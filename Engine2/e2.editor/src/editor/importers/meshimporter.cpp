
#include "editor/importers/meshimporter.hpp"

#include "e2/assets/asset.hpp"
#include "e2/utils.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/ui/uicontext.hpp"


//#include "imgui.h"
//#include "imgui_ext.h"
//#include "misc/cpp/imgui_stdlib.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <filesystem>
#include <format>
#include <thread>

#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace fs = std::filesystem;


namespace
{
	glm::quat glmQuat(aiQuaternion const& from)
	{
		return glm::quat(from.w, from.x, from.z, from.y);
	}

	glm::vec3 glmVec3(aiVector3D const& from)
	{
		return { from.x, from.z, from.y };
	}

	glm::vec3 blenderToE2(glm::vec3 const& from)
	{
		return { from.x, from.z, from.y };
	}

	glm::quat blenderToE2(glm::quat const& from)
	{
		return glm::quat(from.w,from.x, from.z, from.y);
	}


}

e2::MeshImporter::MeshImporter(e2::Editor* editor, MeshImportConfig const& config)
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

e2::MeshImporter::~MeshImporter()
{
	delete m_mesh.assImporter;
}

void e2::MeshImporter::update(double seconds)
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

	if (m_status == MIS_Writing)
	{
		ui->label(id_Writing, "Writing..");
	}
	else if (m_status == MIS_Analyzing)
	{
		ui->label(id_Analyzing, std::format("Analyzing file {}..", inputName));
	}
	else if (m_status == MIS_Ready)
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
			for (ImportSubmesh& submesh : m_mesh.submeshes)
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

				ui->label(normals, "**Normals:**");
				ui->beginStackH(stackH, 20.0f);
				ui->radio(modeIgnore, IAM_Ignore, submesh.normalsMode, "Ignore");
				ui->radio(modeGenerate, IAM_Generate, submesh.normalsMode, "Generate");
				ui->radio(modeImport, IAM_Import, submesh.normalsMode, "Import");
				ui->endStackH();

				if (submesh.hasColors)
				{
					ui->checkbox(impcol, submesh.importColors, "Import Colors");
				}

				if (submesh.hasUv0)
				{
					ui->checkbox(impuv0, submesh.importUv0, "Import Uv0");
				}

				if (submesh.hasUv1)
				{
					ui->checkbox(impuv1, submesh.importUv1, "Import Uv1");
				}

				if (submesh.hasUv2)
				{
					ui->checkbox(impuv2, submesh.importUv2, "Import Uv2");
				}

				if (submesh.hasUv3)
				{
					ui->checkbox(impuv3, submesh.importUv3, "Import Uv3");
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
			for (ImportAnimation& anim : m_animations)
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
	else if (m_status == MIS_Success)
	{
		ui->label(id_Success, std::format("Successfully imported {}", inputName));
	}
	else if (m_status == MIS_Failed)
	{
		ui->label(id_Fail, std::format("Failed to import {}: {}", inputName, m_failReason));
	}

	ui->endStackV();

}

bool e2::MeshImporter::analyze()
{

	m_mesh.assImporter = new Assimp::Importer();
	m_mesh.assScene = m_mesh.assImporter->ReadFile(m_config.input,
		aiProcess_JoinIdenticalVertices |
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		aiProcess_LimitBoneWeights |
		aiProcess_ImproveCacheLocality |
		//aiProcess_PopulateArmatureData |
		aiProcess_FixInfacingNormals |
		aiProcess_OptimizeMeshes |
		aiProcess_GenBoundingBoxes | 
		aiProcess_CalcTangentSpace | 
		aiProcess_FlipWindingOrder /* |
		aiProcess_MakeLeftHanded*/);

	if (!m_mesh.assScene)
	{
		m_failReason = m_mesh.assImporter->GetErrorString();
		return false;
	}



	m_mesh.hasMesh = m_mesh.assScene->HasMeshes();
	m_mesh.importMesh = m_mesh.hasMesh;

	m_mesh.meshName = m_mesh.assScene->mName.C_Str();
	if (m_mesh.meshName.size() == 0)
	{
		m_mesh.meshName = m_mesh.assScene->mMeshes[0]->mName.C_Str();
	}

	auto cleanDot = e2::split(m_mesh.meshName, '.');
	if (cleanDot.size() == 0 || cleanDot[0].size() == 0)
		m_mesh.meshName = "";
	else
		m_mesh.meshName = cleanDot[0];

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

	if (m_mesh.hasMesh)
	{


		
		uint32_t boneIdMaker{};


		for (uint64_t i = 0; i < m_mesh.assScene->mNumMeshes; i++)
		{
			e2::ImportSubmesh submesh;
			aiMesh* mesh = m_mesh.assScene->mMeshes[i];
			if (!mesh->HasPositions())
			{
				m_failReason = "No vertex positions in mesh, fuck you trying to do?";
				return false;
			}

			submesh.assMaterial = m_mesh.assScene->mMaterials[mesh->mMaterialIndex];
			submesh.assMesh = mesh;

			submesh.materialName = m_mesh.assScene->mMaterials[mesh->mMaterialIndex]->GetName().C_Str();

			submesh.hasNormals = mesh->HasNormals() && mesh->HasTangentsAndBitangents();
			submesh.normalsMode = submesh.hasNormals ? IAM_Import : IAM_Generate;

			submesh.hasColors = mesh->HasVertexColors(0);
			submesh.importColors = submesh.hasColors;
			
			submesh.hasUv0 = mesh->HasTextureCoords(0);
			submesh.importUv0 = submesh.hasUv0;

			submesh.hasUv1 = mesh->HasTextureCoords(1);
			submesh.importUv1 = submesh.hasUv1;

			submesh.hasUv2 = mesh->HasTextureCoords(2);
			submesh.importUv2 = submesh.hasUv2;

			submesh.hasUv3 = mesh->HasTextureCoords(3);
			submesh.importUv3 = submesh.hasUv3;

			submesh.hasWeights = mesh->HasBones();
			submesh.importWeights = submesh.hasWeights;

			if(mesh->HasBones())
				submesh.weightData.resize(submesh.assMesh->mNumVertices);

			

			std::unordered_map<uint32_t, uint32_t> weightCounts;
			for (uint32_t i = 0; i < submesh.assMesh->mNumVertices; i++)
			{
				weightCounts[i] = 0;
			}

			for (uint32_t i = 0; i < submesh.assMesh->mNumBones; i++)
			{
				aiBone* assBone = submesh.assMesh->mBones[i];
				aiNode* assNode = m_mesh.assScene->mRootNode->FindNode(assBone->mName.C_Str());

				if (assNode)
				{
					if (m_mesh.skeleton.boneIndex.find(assBone->mName.C_Str()) == m_mesh.skeleton.boneIndex.end())
					{
						ImportBone newBone;
						newBone.id = boneIdMaker++;
						newBone.name = assNode->mName.C_Str();
						newBone.parentId = -1;
						newBone.parentName = assNode->mParent ? assNode->mParent->mName.C_Str() : "none";

						aiVector3D assTranslation;
						aiQuaternion assRotation;
						aiVector3D assScale;
						assNode->mTransformation.Decompose(assScale, assRotation, assTranslation);

						newBone.localTranslation = ::glmVec3(assTranslation);
						newBone.localRotation = ::glmQuat(assRotation);

						m_mesh.skeleton.boneIndex[newBone.name] = newBone.id;
						m_mesh.skeleton.bones.push_back(newBone);
					}

					uint32_t currId = m_mesh.skeleton.boneIndex[assBone->mName.C_Str()];
					
					for (uint32_t j = 0; j < assBone->mNumWeights; j++)
					{
						aiVertexWeight* weight = &assBone->mWeights[j];
						WeightData* weightData = &submesh.weightData[weight->mVertexId];
						switch (weightCounts.at(weight->mVertexId))
						{
						case 0:
							weightData->ids.x = currId;
							weightData->weights.x = weight->mWeight;
							weightCounts.at(weight->mVertexId)++;
							break;

						case 1:
							weightData->ids.y = currId;
							weightData->weights.y = weight->mWeight;
							weightCounts.at(weight->mVertexId)++;
							break;

						case 2:
							weightData->ids.z = currId;
							weightData->weights.z = weight->mWeight;
							weightCounts.at(weight->mVertexId)++;
							break;

						case 3:
							weightData->ids.w = currId;
							weightData->weights.w = weight->mWeight;
							weightCounts.at(weight->mVertexId)++;
							break;

						}
					}
				}

			}


			m_mesh.submeshes.push_back(submesh);
		}
	
		for (e2::ImportBone& bone : m_mesh.skeleton.bones)
		{
			aiNode* boneNode = m_mesh.assScene->mRootNode->FindNode(bone.name.c_str());
			if (!boneNode)
			{
				LogWarning("Bone no longer finds its scene node, this shouldn't happen!");
				continue;
			}

			if (!boneNode->mParent)
			{
				if (m_mesh.skeletonName.size() == 0)
				{
					m_mesh.skeletonName = boneNode->mName.C_Str();
				}
				bone.isRoot = true;
				bone.parentId = -1;
			}
			else
			{
				bool found = false;
				for (e2::ImportBone& subBone : m_mesh.skeleton.bones)
				{
					if (subBone.name == std::string(boneNode->mParent->mName.C_Str()))
					{
						bone.parentId = m_mesh.skeleton.boneIndex.at(std::string(boneNode->mParent->mName.C_Str()));
						found = true;
					}
				}
				if (!found)
				{
					bone.isRoot = true;
					bone.parentId = -1;
				}
			}
		}

		m_mesh.hasSkeleton = m_mesh.skeleton.bones.size();
		if(m_mesh.hasSkeleton)
			m_mesh.importSkeleton = true;

		// anims
		for (uint32_t i = 0; i < m_mesh.assScene->mNumAnimations; i++)
		{
			aiAnimation* anim = m_mesh.assScene->mAnimations[i];
			std::string animName = anim->mName.C_Str();
			if (animName.substr(0, 11) == std::string("AnimStack::"))
			{
				animName = animName.substr(11);
			}

			std::vector<std::string> nameParts = e2::split(animName, '|');
			if (nameParts.size() > 1)
			{
				animName = nameParts[1];
			}

			ImportAnimation newAnim;
			newAnim.import = true;
			newAnim.name = animName;
			newAnim.framesPerSecond = (float)anim->mTicksPerSecond;
			newAnim.duration = (uint32_t)anim->mDuration;

			LogNotice("anim{}: {}", i, newAnim.name);

			for (uint32_t c = 0; c < anim->mNumChannels; c++)
			{
				aiNodeAnim* channel = anim->mChannels[c];
				std::string channelName = channel->mNodeName.C_Str();
				LogNotice("channel{}: {}", c, channelName);

				ImportChannel newChannel;

				for (uint32_t t = 0; t < channel->mNumPositionKeys; t++)
				{
					aiVectorKey k = channel->mPositionKeys[t];
					Vec3Key newKey;
					newKey.vector = ::blenderToE2(glm::vec3(k.mValue.x, k.mValue.y, k.mValue.z));
					newKey.time = (float)k.mTime / newAnim.framesPerSecond;
					newChannel.positionKeys.push_back(newKey);
				}

				for (uint32_t t = 0; t < channel->mNumRotationKeys; t++)
				{
					aiQuatKey k = channel->mRotationKeys[t];
					QuatKey newKey;
					newKey.quat = ::blenderToE2(glm::quat(k.mValue.w, k.mValue.x, k.mValue.y, k.mValue.z));
					newKey.time = (float)k.mTime / newAnim.framesPerSecond;
					newChannel.rotationKeys.push_back(newKey);
				}

				newAnim.channels[channelName] = newChannel;
			}

			m_animations.push_back(newAnim);

		}


	}



	return true;
}



bool e2::MeshImporter::writeAssets()
{
	if (m_mesh.hasMesh && m_mesh.importMesh)
	{
		// prepare header first, so we can add depedencies to it
		e2::AssetHeader meshHeader;
		meshHeader.version = e2::AssetVersion::Latest;
		meshHeader.assetType = "e2::Mesh";

		// Process
		for (e2::ImportSubmesh &submesh : m_mesh.submeshes)
		{
			e2::ImportSubmesh::IntermediateData& im = submesh.intermediate;
			aiMesh* sm = submesh.assMesh;

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

			// vertex count
			im.vertexCount = sm->mNumVertices;

			// handle indices (special care to calculate index count, as its used for rendering)
			im.indexData = e2::Buffer(false, sizeof(uint32_t) * sm->mNumFaces * 3);
			for (uint32_t i = 0; i < sm->mNumFaces; i++)
			{
				if (sm->mFaces[i].mNumIndices == 3)
				{
					im.indexCount += 3;
					im.indexData << uint32_t(sm->mFaces[i].mIndices[2]);
					im.indexData << uint32_t(sm->mFaces[i].mIndices[1]);
					im.indexData << uint32_t(sm->mFaces[i].mIndices[0]);
				/*
					im.indexData << uint32_t(sm->mFaces[i].mIndices[2]);
					im.indexData << uint32_t(sm->mFaces[i].mIndices[1]);
					im.indexData << uint32_t(sm->mFaces[i].mIndices[0]);*/
				}
			}

			// setup flags 
			bool useNormals = submesh.hasNormals && submesh.normalsMode != IAM_Ignore;
			uint32_t numUvs = 0;

			bool useUv0 = submesh.hasUv0 && submesh.importUv0;
			bool useUv1 = submesh.hasUv1 && submesh.importUv1;
			bool useUv2 = submesh.hasUv2 && submesh.importUv2;
			bool useUv3 = submesh.hasUv3 && submesh.importUv3;

			if (useUv0) numUvs++;
			if (useUv1) numUvs++;
			if (useUv2) numUvs++;
			if (useUv3) numUvs++;
			bool useUv01 = numUvs >= 1;
			bool useUv23 = numUvs >= 3;
			bool useColors = submesh.hasColors && submesh.importColors;
			bool useWeights = submesh.hasWeights && submesh.importWeights;
			//bool useWeights = false;// @todo switch back when implemented

			if (useNormals)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Normal;
			if(useUv01)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::TexCoords01;
			if (useUv23)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::TexCoords23;
			if (useColors)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Color;
			if (useWeights)
				im.attributeFlags = im.attributeFlags | VertexAttributeFlags::Bones;


			// handle positions
			ImportAttribute attribPos{};
			attribPos.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);
			for (uint32_t i = 0; i < sm->mNumVertices; i++)
			{
				//attribPos.vertexData << float(-sm->mVertices[i].x) << float(-sm->mVertices[i].z) << float(sm->mVertices[i].y) << 1.0f;

				glm::vec3 pos = ::glmVec3(sm->mVertices[i]);
				pos.x = -pos.x;
				pos.y = -pos.y;
				attribPos.vertexData << pos << 1.0f;
			}

			im.attributes.push_back(attribPos);

			// handle normals
			if (useNormals)
			{
				ImportAttribute attribNormals{};
				attribNormals.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);

				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					glm::vec3 nrm = ::glmVec3(sm->mNormals[i]);
					nrm.x = -nrm.x;
					nrm.y = -nrm.y;
					attribNormals.vertexData << nrm << 0.0f;
				}

				im.attributes.push_back(attribNormals);

				ImportAttribute attribTangents{};
				attribTangents.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);

				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					glm::vec3 tan = ::glmVec3(sm->mTangents[i]);
					tan.x = -tan.x;
					tan.y = -tan.y;
					attribTangents.vertexData << tan << 0.0f;
				}

				im.attributes.push_back(attribTangents);
			}


			int32_t uvIndex01a = -1, uvIndex01b = -1, uvIndex23a = -1, uvIndex23b = -1;
			int32_t* uvIndices[4] = { &uvIndex01a, &uvIndex01b, &uvIndex23a, &uvIndex23b };
			bool useUvs[4] = { useUv0, useUv1, useUv2, useUv3 };

			uint32_t nextIndex = 0;
			for (uint32_t i = 0; i < 4; i++)
			{
				if (useUvs[i])
				{
					*(uvIndices[nextIndex++]) = i;
				}
			}


			// handle uv01
			if (useUv01)
			{

				ImportAttribute attribUv01{};
				attribUv01.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);
				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
					if (uvIndex01a >= 0)
					{
						x = float(sm->mTextureCoords[uvIndex01a][i].x);
						y = float(sm->mTextureCoords[uvIndex01a][i].y);
					}
					if (uvIndex01b >= 0)
					{
						z = float(sm->mTextureCoords[uvIndex01b][i].x);
						w = float(sm->mTextureCoords[uvIndex01b][i].y);
					}
					attribUv01.vertexData << x << y << z << w;
				}
				im.attributes.push_back(attribUv01);
			}

			// handle uv23
			if (useUv23)
			{

				ImportAttribute attribUv23{};
				attribUv23.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);
				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					float x = 0.0f, y = 0.0f, z = 0.0f, w = 0.0f;
					if (uvIndex23a >= 0)
					{
						x = float(sm->mTextureCoords[uvIndex23a][i].x);
						y = float(sm->mTextureCoords[uvIndex23a][i].y);
					}
					if (uvIndex23b >= 0)
					{
						z = float(sm->mTextureCoords[uvIndex23b][i].x);
						w = float(sm->mTextureCoords[uvIndex23b][i].y);
					}
					attribUv23.vertexData << x << y << z << w;
				}
				im.attributes.push_back(attribUv23);
			}

			// handle colors
			if (useColors)
			{
				ImportAttribute attribColors{};
				attribColors.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);
				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					attribColors.vertexData << float(sm->mColors[0][i].r) << float(sm->mColors[0][i].g) << float(sm->mColors[0][i].b) << float(sm->mColors[0][i].a);
				}
				im.attributes.push_back(attribColors);
			}

			
			// handle weights
			if (useWeights)
			{
				ImportAttribute attribWeights;
				attribWeights.vertexData = e2::Buffer(false, sizeof(glm::vec4) * sm->mNumVertices);

				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					attribWeights.vertexData << submesh.weightData[i].weights;
				}

				im.attributes.push_back(attribWeights);

				ImportAttribute attribIds;
				attribIds.vertexData = e2::Buffer(false, sizeof(glm::uvec4) * sm->mNumVertices);
				for (uint32_t i = 0; i < sm->mNumVertices; i++)
				{
					attribIds.vertexData << submesh.weightData[i].ids;
				}

				im.attributes.push_back(attribIds);
			}


		}

		// write the data 
		e2::Buffer meshData; 

		meshData << uint32_t(m_mesh.skeleton.boneIndex.size());
		for (auto& pair : m_mesh.skeleton.boneIndex)
		{
			meshData << std::string(pair.first);
			meshData << uint32_t(pair.second);
		}

		meshData << uint32_t(m_mesh.submeshes.size());
		for (e2::ImportSubmesh &submesh : m_mesh.submeshes)
		{

			e2::ImportSubmesh::IntermediateData& im = submesh.intermediate;
			meshData << im.vertexCount;
			meshData << im.indexCount;
			meshData << uint8_t(im.attributeFlags);
			meshData << uint32_t(im.indexData.size());
			meshData.write(im.indexData.begin(), im.indexData.size());
			for (ImportAttribute const& attr : im.attributes)
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
			e2::ImportBone& b = m_mesh.skeleton.bones[i];
			LogNotice("Writing bone {} {} with parent {}", i, b.name, b.parentId);


			skeletonData << std::string(b.name);
			skeletonData << glm::vec3(b.localTranslation);
			skeletonData << glm::quat(b.localRotation);
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

	for (ImportAnimation& anim : m_animations)
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
			ImportChannel& channel = pair.second;

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
			ImportChannel& channel = pair.second;

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

e2::UUID e2::MeshImporter::createMaterial(std::string const& outName)
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

void e2::MeshImporter::startAnalyzing()
{
	m_status = MIS_Analyzing;
	std::thread analyzeThread([this]() {

		if (!analyze())
		{
			m_status = MIS_Failed;
		}
		else
		{
			m_status = MIS_Ready;
		}
		});
	analyzeThread.detach();
}

void e2::MeshImporter::startProcessing()
{
	m_status = MIS_Writing;
	std::thread processingThread([this]() {

		if (!writeAssets())
		{
			m_status = MIS_Failed;
		}

		m_status = MIS_Success;

	});
	processingThread.detach();
}

std::string e2::attributeString(ImportAttributeMode mode)
{
	switch (mode)
	{
	case IAM_Ignore:
		return "Ignore";
		break;
	case IAM_Generate:
		return "Generate";
		break;
	case IAM_Import:
		return "Import";
		break;
	default:
		break;
	}

	return "";
}

glm::vec3 e2::ImportChannel::samplePosition(float time)
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

glm::quat e2::ImportChannel::sampleRotation(float time)
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
