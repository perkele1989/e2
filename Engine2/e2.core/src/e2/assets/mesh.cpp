
#include "e2/assets/mesh.hpp"
#include "e2/assets/material.hpp"

#include "e2/managers/assetmanager.hpp"
#include "e2/managers/rendermanager.hpp"

#include "e2/rhi/rendercontext.hpp"

#include "e2/e2.hpp"

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/matrix_interpolation.hpp>
//GLM_GTX_matrix_interpolation



namespace
{
	glm::mat4 recompose(glm::vec3 const& translation, glm::vec3 const& scale, glm::vec3 const& skew, glm::vec4 const& perspective, glm::quat const& rotation)
	{
		glm::mat4 m = glm::mat4(1.f);

		m[0][3] = perspective.x;
		m[1][3] = perspective.y;
		m[2][3] = perspective.z;
		m[3][3] = perspective.w;

		m *= glm::translate(translation);
		m *= glm::mat4_cast(rotation);

		if (skew.x) {
			glm::mat4 tmp{ 1.f };
			tmp[2][1] = skew.x;
			m *= tmp;
		}

		if (skew.y) {
			glm::mat4 tmp{ 1.f };
			tmp[2][0] = skew.y;
			m *= tmp;
		}

		if (skew.z) {
			glm::mat4 tmp{ 1.f };
			tmp[1][0] = skew.z;
			m *= tmp;
		}

		m *= glm::scale(scale);

		return m;
	}
}


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
	uint32_t numBones{};
	source >> numBones;

	e2::StackVector<int32_t, e2::maxNumSkeletonBones> parentIds;

	for (uint32_t i = 0; i < numBones; i++)
	{
		e2::Bone newBone;
		newBone.index = i;

		std::string name; 
		source >> name; 
		newBone.name = name;

		source >> newBone.bindMatrix;

		source >> newBone.localTransform;

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

		e2::PoseBone poseBone;
		poseBone.id = i;
		poseBone.localTransform = poseBone.localTransform;
		poseBone.assetBone = bone;

		m_poseBones.push(poseBone);
	}
	m_skin.resize(m_skeleton->numBones());

}

e2::Pose::~Pose()
{

}

e2::Engine* e2::Pose::engine()
{
	return m_skeleton->engine();
}

void e2::Pose::updateSkin()
{
	E2_BEGIN_SCOPE();

	// these are sorted by hierarchy so fine to just do them linearly
	glm::mat4 identityTransform = glm::identity<glm::mat4>();

	for (uint32_t id = 0; id < m_skeleton->numBones(); id++)
	{
		e2::Bone* bone = m_skeleton->boneById(id);
		e2::Bone* parentBone = bone->parent;

		e2::PoseBone* poseBone = &m_poseBones[id];
		e2::PoseBone* parentPoseBone = parentBone ? &m_poseBones[parentBone->index] : nullptr;

		glm::mat4 parentTransform = parentPoseBone ? parentPoseBone->cachedGlobalTransform : identityTransform;

		poseBone->cachedGlobalTransform = parentTransform * poseBone->localTransform;
		poseBone->cachedSkinTransform = poseBone->cachedGlobalTransform * bone->bindMatrix;
		m_skin[id] = poseBone->cachedSkinTransform;
	}

	/*
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

		poseBone->cachedGlobalTransform = chunk.parentTransform * poseBone->localTransform;
		poseBone->cachedSkinTransform = poseBone->cachedGlobalTransform * chunk.bone->bindMatrix;

		m_skin[chunk.bone->index] = poseBone->cachedSkinTransform;

		for (e2::Bone* child : chunk.bone->children)
		{
			chunks.push({ poseBone->cachedGlobalTransform, child });
		}
	}
	*/
	E2_END_SCOPE();
}

void e2::Pose::applyBindPose()
{
	E2_BEGIN_SCOPE();
	for (e2::PoseBone& bone : m_poseBones)
	{
		bone.localTransform = bone.assetBone->localTransform;
	}
	E2_END_SCOPE();
}

void e2::Pose::applyPose(Pose* otherPose)
{
	E2_BEGIN_SCOPE();
	if (m_skeleton != otherPose->skeleton())
	{
		LogError("incompatible poses (skeleton mismatch)");
		E2_END_SCOPE();
		return;
	}

	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];

		poseBone->localTransform = otherPose->poseBoneById(boneId)->localTransform;
	}
	E2_END_SCOPE();
}

void e2::Pose::applyBlend(Pose* a, Pose* b, float alpha)
{
	E2_BEGIN_SCOPE();
	if (m_skeleton != a->skeleton() || m_skeleton != b->skeleton())
	{
		LogError("incompatible poses (skeleton mismatch)");
		E2_END_SCOPE();
		return;
	}

	if (alpha < 0.001f)
	{
		applyPose(a);
		E2_END_SCOPE();
		return;
	}
	
	if (alpha > 1.0f - 0.001f)
	{
		applyPose(b);
		E2_END_SCOPE();
		return;
	}
	
	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];

		glm::vec3 translationA, scaleA, skewA, translationB, scaleB, skewB, translationC, scaleC, skewC;
		glm::vec4 perspectiveA, perspectiveB, perspectiveC;
		glm::quat rotationA, rotationB, rotationC;
		glm::decompose(a->poseBoneById(boneId)->localTransform, scaleA, rotationA, translationA, skewA, perspectiveA);
		glm::decompose(b->poseBoneById(boneId)->localTransform, scaleB, rotationB, translationB, skewB, perspectiveB);

		translationC = glm::mix(translationA, translationB, alpha);
		scaleC = glm::mix(scaleA, scaleB, alpha);
		skewC = glm::mix(skewA, skewB, alpha);
		perspectiveC = glm::mix(perspectiveA, perspectiveB, alpha);
		rotationC = glm::slerp(rotationA, rotationB, alpha);

		poseBone->localTransform = recompose(translationC, scaleC, skewC, perspectiveC, rotationC);


		//poseBone->localTransform = glm::interpolate(a->poseBoneById(boneId)->localTransform, b->poseBoneById(boneId)->localTransform, alpha);
	}
	E2_END_SCOPE();
}

void e2::Pose::blendWith(Pose* b, float alpha)
{
	if (alpha < 0.001f)
		return;

	// gg ez
	applyBlend(this, b, alpha);
}

void e2::Pose::applyAnimation(e2::Ptr<e2::Animation> anim, float time)
{
	E2_BEGIN_SCOPE();
	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		e2::Bone* bone = m_skeleton->boneById(boneId);
		
		e2::Name trackName_pos = std::format("{}.position", bone->name.cstring());
		e2::AnimationTrack* track_pos = anim->trackByName(trackName_pos, e2::AnimationType::Vec3);

		e2::Name trackName_rot = std::format("{}.rotation", bone->name.cstring());
		e2::AnimationTrack* track_rot = anim->trackByName(trackName_rot, e2::AnimationType::Quat);


		glm::vec3 translation, scale, skew;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(poseBone->localTransform, scale, rotation, translation, skew, perspective);

		if (track_pos)
			translation = track_pos->getVec3(time, anim->frameRate());

		if (track_rot)
			rotation = track_rot->getQuat(time, anim->frameRate());

		poseBone->localTransform = recompose(translation, scale, skew, perspective, rotation);
	}
	E2_END_SCOPE();
}

e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> const& e2::Pose::skin()
{
	return m_skin;
}

e2::Ptr<e2::Skeleton> e2::Pose::skeleton()
{
	return m_skeleton;
}

e2::PoseBone* e2::Pose::poseBoneById(uint32_t id)
{
	if (id >= m_poseBones.size())
		return nullptr;

	return &m_poseBones[id];
}

e2::AnimationPose::AnimationPose(e2::Ptr<e2::Skeleton> skeleton, e2::Ptr<e2::Animation> animation, bool loop)
	: e2::Pose(skeleton)
	, m_animation(animation)
	, m_loop(loop)
	, m_playing(true)
{
	m_rotationTracks.resize(skeleton->numBones());
	m_translationTracks.resize(skeleton->numBones());

	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		e2::Bone* bone = m_skeleton->boneById(boneId);

		e2::Name trackName_pos = std::format("{}.position", bone->name.cstring());
		m_translationTracks[boneId] = m_animation->trackByName(trackName_pos, e2::AnimationType::Vec3);

		e2::Name trackName_rot = std::format("{}.rotation", bone->name.cstring());
		m_rotationTracks[boneId] = m_animation->trackByName(trackName_rot, e2::AnimationType::Quat);
	}
}

e2::AnimationPose::~AnimationPose()
{

}

void e2::AnimationPose::updateAnimation(float timeDelta, bool onlyTickTime)
{
	if (!m_playing)
		return;

	E2_BEGIN_SCOPE();
	m_time += timeDelta;
	while (m_time >= m_animation->timeSeconds())
	{
		if (!m_loop)
		{
			m_playing = false;
			m_time = m_animation->timeSeconds();
			break;
		}

		m_time -= m_animation->timeSeconds();
	}

	if (onlyTickTime)
		return;

	//applyBindPose();
	for (uint32_t boneId = 0; boneId < m_poseBones.size(); boneId++)
	{
		e2::PoseBone* poseBone = &m_poseBones[boneId];
		e2::Bone* bone = poseBone->assetBone; // m_skeleton->boneById(boneId);

		e2::AnimationTrack* posTrack = m_translationTracks[boneId];
		e2::AnimationTrack* rotTrack = m_rotationTracks[boneId];

		//poseBone->localTransform = poseBone->assetBone->localTransform;

		if (!posTrack && !rotTrack)
		{
			poseBone->localTransform = poseBone->assetBone->localTransform;
			continue;
		}

		glm::vec3 translation, scale, skew;
		glm::vec4 perspective;
		glm::quat rotation;
		glm::decompose(poseBone->assetBone->localTransform, scale, rotation, translation, skew, perspective);

		if (posTrack)
			translation = posTrack->getVec3(m_time, m_animation->frameRate());

		if (rotTrack)
			rotation = rotTrack->getQuat(m_time, m_animation->frameRate());

		poseBone->localTransform = recompose(translation, scale, skew, perspective, rotation);
	}

	//applyAnimation(m_animation, m_time);
	E2_END_SCOPE();
}
