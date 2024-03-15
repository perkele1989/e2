
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"

#include "e2/managers/assetmanager.hpp"
#include "e2/managers/rendermanager.hpp"

#include "e2/rhi/rendercontext.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace
{
	//uint64_t numMeshes{};
}

e2::Mesh::Mesh()
{
	//::numMeshes++;
	//LogNotice("Num meshes increased to {}", ::numMeshes);
}

e2::Mesh::~Mesh()
{
	//::numMeshes--;
	//LogNotice("Num meshes decreased to {}", ::numMeshes);

	for (uint8_t i = 0; i < m_specifications.size(); i++)
	{
		e2::SubmeshSpecification &spec = m_specifications[i];
		for (uint8_t j = 0; j < spec.vertexAttributes.size(); j++)
		{
			spec.vertexAttributes[j]->keepAround();
			e2::discard(spec.vertexAttributes[j]);
		}
		spec.indexBuffer->keepAround();
		e2::discard(spec.indexBuffer);
	}
}

void e2::Mesh::write(Buffer& destination) const
{

}

bool e2::Mesh::read(Buffer& source)
{
	if (!e2::Asset::read(source))
		return false;

	if (version >= AssetVersion::AddSkeletons)
	{
		uint32_t numBoneMappings{ 0 };
		source >> numBoneMappings;
		for (uint32_t i = 0; i < numBoneMappings; i++)
		{
			std::string mappingName;
			source >> mappingName;

			uint32_t mappingBoneIndex{};
			source >> mappingBoneIndex;

			m_boneIndex[mappingName] = mappingBoneIndex;
		}
	}

	uint32_t submeshCount{};
	source >> submeshCount;

	for (uint32_t i = 0; i < submeshCount; i++)
	{
		// fetch material directly from deps array using submesh index
		e2::MaterialPtr material = assetManager()->get(dependencies[i].uuid).cast<e2::Material>();
		m_materials.push(material);

		e2::SubmeshSpecification newSpecification;

		source >> newSpecification.vertexCount;
		source >> newSpecification.indexCount;

		uint8_t attributeFlags{};
		source >> attributeFlags;
		newSpecification.attributeFlags = (e2::VertexAttributeFlags)attributeFlags;

		newSpecification.vertexLayout = renderManager()->getOrCreateVertexLayout(newSpecification.attributeFlags);

		auto readIndexBuffer = [this, &source, &newSpecification]() -> void {
			uint32_t bufferSize{};
			source >> bufferSize;

			e2::DataBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.dynamic = false;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.type = BufferType::IndexBuffer;
			e2::IDataBuffer* newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);

			uint32_t* indexBuffer = reinterpret_cast<uint32_t*>(source.current());
			for (uint32_t ti = 0; ti < (newSpecification.indexCount / 3); ti++)
			{
				uint32_t a = indexBuffer[ti * 3 + 0];
				uint32_t b = indexBuffer[ti * 3 + 1];
				uint32_t c = indexBuffer[ti * 3 + 2];

				if (a >= newSpecification.vertexCount || b >= newSpecification.vertexCount || c >= newSpecification.vertexCount)
					LogError("corrupt data detected");
			}

			// upload data direct from source buffer, and then use consume to skip 
			newBuffer->upload(source.current(), bufferSize, 0, 0);
			uint64_t oldCursor{};
			source.consume(bufferSize, oldCursor);

			newSpecification.indexBuffer = newBuffer;
		};

		auto readVertexBuffer = [this, &source, &newSpecification]() -> void {
			uint32_t bufferSize{};
			source >> bufferSize;

			e2::DataBufferCreateInfo bufferCreateInfo{};
			bufferCreateInfo.dynamic = false;
			bufferCreateInfo.size = bufferSize;
			bufferCreateInfo.type = BufferType::VertexBuffer;
			e2::IDataBuffer* newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);

			// upload data direct from source buffer, and then use consume to skip 
			newBuffer->upload(source.current(), bufferSize, 0, 0);
			uint64_t oldCursor{};
			source.consume(bufferSize, oldCursor);

			newSpecification.vertexAttributes.push(newBuffer);
		};

		readIndexBuffer();

		// Position 
		readVertexBuffer();

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
		{
			readVertexBuffer();
			readVertexBuffer();
		}
		
		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
		{
			readVertexBuffer();
		}

		if ((newSpecification.attributeFlags & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
		{
			readVertexBuffer();
			readVertexBuffer();
		}

		m_specifications.push(newSpecification);

	}

	flagDone();

	return true;
}

e2::SubmeshSpecification const& e2::Mesh::specification(uint8_t subIndex)
{
	return m_specifications[subIndex];
}

e2::MaterialPtr e2::Mesh::material(uint8_t subIndex) const
{
	return m_materials[subIndex];
}

uint8_t e2::Mesh::submeshCount() const
{
	return (uint8_t)m_specifications.size();
}

void e2::Mesh::addProceduralSubmesh(e2::ProceduralSubmesh const& sourceData)
{
	if (m_done)
	{
		LogError("attempted to add procedural submesh, but mesh is flagged as done, refusing.");
		return;
	}


	e2::SubmeshSpecification newSpecification;

	newSpecification.vertexCount = sourceData.numVertices;
	newSpecification.indexCount = sourceData.numIndices;
	newSpecification.attributeFlags = sourceData.attributes;
	newSpecification.vertexLayout = renderManager()->getOrCreateVertexLayout(newSpecification.attributeFlags);

	e2::IDataBuffer* newBuffer{};
	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = false;

	// Indices
	bufferCreateInfo.type = BufferType::IndexBuffer;
	bufferCreateInfo.size = sizeof(uint32_t) * sourceData.numIndices;
	newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
	newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceIndices), bufferCreateInfo.size, 0, 0);
	newSpecification.indexBuffer = newBuffer;

	// Positions
	bufferCreateInfo.type = BufferType::VertexBuffer;
	bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
	newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
	newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourcePositions), bufferCreateInfo.size, 0, 0);
	newSpecification.vertexAttributes.push(newBuffer);

	// Note: The order here matters, and depends on sourceData.attributes

	// Normals + Tangents
	if ((sourceData.attributes & e2::VertexAttributeFlags::Normal) == e2::VertexAttributeFlags::Normal)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceNormals), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);

		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceTangents), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Uv01
	if ((sourceData.attributes & e2::VertexAttributeFlags::TexCoords01) == e2::VertexAttributeFlags::TexCoords01)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceUv01), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Uv23
	if ((sourceData.attributes & e2::VertexAttributeFlags::TexCoords23) == e2::VertexAttributeFlags::TexCoords23)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceUv23), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Colors
	if ((sourceData.attributes & e2::VertexAttributeFlags::Color) == e2::VertexAttributeFlags::Color)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceColors), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	// Weights + Ids
	if ((sourceData.attributes & e2::VertexAttributeFlags::Bones) == e2::VertexAttributeFlags::Bones)
	{
		bufferCreateInfo.size = sizeof(glm::vec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceWeights), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);

		bufferCreateInfo.size = sizeof(glm::uvec4) * sourceData.numVertices;
		newBuffer = renderContext()->createDataBuffer(bufferCreateInfo);
		newBuffer->upload(reinterpret_cast<uint8_t const*>(sourceData.sourceBones), bufferCreateInfo.size, 0, 0);
		newSpecification.vertexAttributes.push(newBuffer);
	}

	m_specifications.push(newSpecification);
	m_materials.push(sourceData.material);
}

void e2::Mesh::flagDone()
{
	m_done = true;
}

e2::Skeleton::Skeleton()
{

}

e2::Skeleton::~Skeleton()
{

}

void e2::Skeleton::write(Buffer& destination) const
{

}

bool e2::Skeleton::read(Buffer& source)
{

	source >> m_inverseTransform;
	uint32_t numBones{};
	source >> numBones;

	e2::StackVector<int32_t, e2::maxNumSkeletonBones> parentIds;

	for (uint32_t i = 0; i < numBones; i++)
	{
		e2::Bone newBone;
		newBone.index = i;
		source >> newBone.name;
		source >> newBone.transform;
		source >> newBone.inverseBindTransform;

		int32_t parentId{ -1 };
		source >> parentId;
		parentIds.push(parentId);

		m_bones.push(newBone);
		e2::Bone* newBonePtr = &m_bones[m_bones.size() - 1];

		if (parentId == -1)
			m_roots.push(newBonePtr);
		m_boneMap[newBonePtr->name] = newBonePtr;
	}

	// resolve parent-child relations 
	for (uint32_t i = 0; i < m_bones.size(); i++)
	{
		int32_t parentId = parentIds[i];
		if (parentId >= 0)
		{
			m_bones[i].parent = &m_bones[parentId];
			m_bones[parentId].children.push(&m_bones[i]);
		}
	}
	return true;
}

e2::Bone* e2::Skeleton::boneByName(e2::Name name)
{
	auto finder = m_boneMap.find(name);
	if (finder != m_boneMap.end())
	{
		return finder->second;
	}

	return nullptr;
}

uint32_t e2::Skeleton::numBones()
{
	return m_bones.size();
}

e2::Bone* e2::Skeleton::boneById(uint32_t id)
{
	if (id >= m_bones.size())
		return nullptr;

	return &m_bones[id];
}

uint32_t e2::Skeleton::numRootBones()
{
	return m_roots.size();
}

e2::Bone* e2::Skeleton::rootBoneById(uint32_t rootId)
{
	return m_roots[rootId];
}

glm::mat4 const& e2::Skeleton::inverseTransform()
{
	return m_inverseTransform;
}

e2::Animation::~Animation()
{

}

void e2::Animation::write(Buffer& destination) const
{

}

bool e2::Animation::read(Buffer& source)
{
	source >> m_numFrames;
	source >> m_frameRate;
	
	uint32_t numTracks{};
	source >> numTracks;
	for (uint32_t i = 0; i < numTracks; i++)
	{
		std::string trackName;
		source >> trackName;

		e2::AnimationTrack newTrack;
		newTrack.name = trackName;

		uint8_t trackType = 0;
		source >> trackType;
		switch (trackType)
		{
		case 0:
			newTrack.type = AnimationType::Float;
			break;
		case 1:
			newTrack.type = AnimationType::Vec2;
			break;
		case 2:
			newTrack.type = AnimationType::Vec3;
			break;
		case 3:
			newTrack.type = AnimationType::Quat;
			break;
		}

		for (uint32_t j = 0; j < m_numFrames; j++)
		{
			AnimationTrackFrame newFrame;
			source >> newFrame.data[0];
			source >> newFrame.data[1];
			source >> newFrame.data[2];
			source >> newFrame.data[3];
			newTrack.frames.push(newFrame);
		}

		m_tracks.push(newTrack);
		m_trackIndex[trackName] = i;
	}

	return true;
}

e2::AnimationTrack* e2::Animation::trackByName(e2::Name name, AnimationType type)
{
	auto finder = m_trackIndex.find(name);
	if (finder == m_trackIndex.end())
		return nullptr;

	e2::AnimationTrack* track = &m_tracks[finder->second];
	if (track->type != type)
		return nullptr;

	return track;
}

uint32_t e2::Animation::numFrames()
{
	return m_numFrames;
}

float e2::Animation::frameRate()
{
	return m_frameRate;
}

float e2::Animation::timeSeconds()
{
	return float(m_numFrames) / m_frameRate;
}

std::pair<uint32_t, float> e2::AnimationTrack::getFrameDelta(float time, float frameRate)
{
	float totalTime = (float)frames.size() / frameRate;
	while (time >= totalTime)
		time -= totalTime;

	float timeCoefficient = time / totalTime;
	float frameF = timeCoefficient * frames.size();
	float truncated = glm::trunc(frameF);
	float frameDelta = frameF - truncated;
	uint32_t frameIndex = (uint32_t)truncated;

	return { frameIndex, frameDelta };
}

float e2::AnimationTrack::getFloat(float time, float frameRate)
{
	if (type != AnimationType::Float)
		LogWarning("using non-float animtrack as float");

	auto [frameIndex, frameDelta] = getFrameDelta(time, frameRate);

	uint32_t nextFrame = frameIndex + 1;
	if (nextFrame >= frames.size())
		nextFrame = 0;

	float a = frames[frameIndex].asFloat();
	float b = frames[nextFrame].asFloat();
	return glm::mix(a, b, frameDelta);
}

glm::vec2 e2::AnimationTrack::getVec2(float time, float frameRate)
{
	if (type != AnimationType::Vec2)
		LogWarning("using non-vec2 animtrack as vec2");

	auto [frameIndex, frameDelta] = getFrameDelta(time, frameRate);

	uint32_t nextFrame = frameIndex + 1;
	if (nextFrame >= frames.size())
		nextFrame = 0;

	glm::vec2 a = frames[frameIndex].asVec2();
	glm::vec2 b = frames[nextFrame].asVec2();
	return glm::mix(a, b, frameDelta);
}

glm::vec3 e2::AnimationTrack::getVec3(float time, float frameRate)
{
	if (type != AnimationType::Vec3)
		LogWarning("using non-vec3 animtrack as vec3");

	auto [frameIndex, frameDelta] = getFrameDelta(time, frameRate);

	uint32_t nextFrame = frameIndex + 1;
	if (nextFrame >= frames.size())
		nextFrame = 0;

	glm::vec3 a = frames[frameIndex].asVec3();
	glm::vec3 b = frames[nextFrame].asVec3();
	return glm::mix(a, b, frameDelta);
}

glm::quat e2::AnimationTrack::getQuat(float time, float frameRate)
{
	if (type != AnimationType::Quat)
		LogWarning("using non-quat animtrack as quat");

	auto [frameIndex, frameDelta] = getFrameDelta(time, frameRate);

	uint32_t nextFrame = frameIndex + 1;
	if (nextFrame >= frames.size())
		nextFrame = 0;

	glm::quat a = frames[frameIndex].asQuat();
	glm::quat b = frames[nextFrame].asQuat();
	return glm::slerp(a, b, frameDelta);
}

float e2::AnimationTrackFrame::asFloat()
{
	return data[0];
}

glm::vec2 e2::AnimationTrackFrame::asVec2()
{
	return { data[0], data[1] };
}

glm::vec3 e2::AnimationTrackFrame::asVec3()
{
	return { data[0], data[1], data[2] };
}

glm::quat e2::AnimationTrackFrame::asQuat()
{
	return { data[3], data[0], data[1], data[2] };
}

e2::Pose::Pose(e2::Ptr<e2::Skeleton> skeleton)
	: m_skeleton(skeleton)
{

	for (uint32_t i = 0; i < m_skeleton->numBones(); i++)
	{
		e2::Bone* bone = m_skeleton->boneById(i);

		glm::vec3 scale, translation, skew;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(bone->transform, scale, rotation, translation, skew, perspective);

		e2::PoseBone poseBone;
		poseBone.bindTranslation = translation;
		poseBone.bindRotation = rotation;
		poseBone.localRotation = poseBone.bindRotation;
		poseBone.localTranslation = poseBone.bindTranslation;

		m_poseBones.push(poseBone);
	}
	m_skin.resize(m_skeleton->numBones());

}

e2::Pose::~Pose()
{

}

glm::quat const& e2::Pose::localBoneRotation(uint32_t boneIndex)
{
	static glm::quat def(1.0f, 0.0f, 0.0f, 0.0f);
	if (boneIndex >= m_poseBones.size())
		return def;

	return m_poseBones[boneIndex].localRotation;
}

glm::vec3 const& e2::Pose::localBoneTranslation(uint32_t boneIndex)
{
	static glm::vec3 def{};
	if (boneIndex >= m_poseBones.size())
		return def;

	return m_poseBones[boneIndex].localTranslation;
}

glm::mat4 e2::Pose::localBoneTransform(uint32_t boneIndex)
{
	return m_poseBones[boneIndex].localTransform();
}

void e2::Pose::updateSkin()
{
	struct WorkChunk
	{
		glm::mat4 parentTransform;
		e2::Bone* bone{};
	};

	std::queue<WorkChunk> chunks;

	glm::mat4 identityTransform = glm::identity<glm::mat4>();
	for (uint32_t id = 0; id < m_skeleton->numRootBones(); id++)
	{
		chunks.push({ identityTransform, m_skeleton->rootBoneById(id) });
	}

	while (!chunks.empty())
	{
		WorkChunk chunk = chunks.front();
		chunks.pop();

		e2::PoseBone* poseBone = &m_poseBones[chunk.bone->index];
		poseBone->updateSkin(chunk.parentTransform, m_skeleton->inverseTransform(), chunk.bone->inverseBindTransform);

		m_skin[chunk.bone->index] = poseBone->cachedSkinTransform;

		for (e2::Bone* child : chunk.bone->children)
		{
			chunks.push({ poseBone->cachedGlobalTransform, child });
		}
	}
}

void e2::Pose::applyBindPose()
{
	for (e2::PoseBone& bone : m_poseBones)
	{
		bone.localTranslation = bone.bindTranslation;
		bone.localRotation = bone.bindRotation;
	}
}

void e2::Pose::applyBlend(Pose* a, Pose* b, float alpha)
{
	if (m_skeleton != a->skeleton() || m_skeleton != b->skeleton())
	{
		LogError("incompatible poses (skeleton mismatch)");
	}

	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		poseBone->localRotation = glm::slerp(a->localBoneRotation(boneId), b->localBoneRotation(boneId), alpha);
		poseBone->localTranslation = glm::mix(a->localBoneTranslation(boneId), b->localBoneTranslation(boneId), alpha);
	}
}

void e2::Pose::blendWith(Pose* b, float alpha)
{
	if (m_skeleton != b->skeleton())
	{
		LogError("incompatible poses (skeleton mismatch)");
	}

	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		poseBone->localRotation = glm::slerp(poseBone->localRotation, b->localBoneRotation(boneId), alpha);
		poseBone->localTranslation = glm::mix(poseBone->localTranslation, b->localBoneTranslation(boneId), alpha);
	}
}

void e2::Pose::applyAnimation(e2::Ptr<e2::Animation> anim, float time)
{
	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		e2::Bone* bone = m_skeleton->boneById(boneId);
		
		e2::Name trackName_pos = std::format("{}.position", bone->name.cstring());
		e2::AnimationTrack* track_pos = anim->trackByName(trackName_pos, e2::AnimationType::Vec3);

		e2::Name trackName_rot = std::format("{}.rotation", bone->name.cstring());
		e2::AnimationTrack* track_rot = anim->trackByName(trackName_rot, e2::AnimationType::Quat);

		if (track_pos)
			poseBone->localTranslation = track_pos->getVec3(time, anim->frameRate());

		if (track_rot)
			poseBone->localRotation = track_rot->getQuat(time, anim->frameRate());
	}
}

e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> const& e2::Pose::skin()
{
	return m_skin;
}

e2::Ptr<e2::Skeleton> e2::Pose::skeleton()
{
	return m_skeleton;
}

glm::mat4 e2::PoseBone::localTransform()
{
	return glm::toMat4(localRotation)* glm::translate(localTranslation);
}
