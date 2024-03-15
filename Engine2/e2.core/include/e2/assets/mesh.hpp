
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/assets/asset.hpp>
#include <e2/assets/material.hpp>
//#include <e2/renderer/shared.hpp>
#include <e2/renderer/meshspecification.hpp>

#include <unordered_map>

namespace e2
{

	struct E2_API Bone
	{
		uint32_t index{};
		e2::Name name;

		glm::mat4 transform;
		glm::mat4 inverseBindTransform;

		e2::Bone* parent{};
		e2::StackVector<e2::Bone*, maxNumBoneChildren> children;

	};

	/** @tags(dynamic, arena, arenaSize=e2::maxNumSkeletonAssets) */
	class E2_API Skeleton : public e2::Asset
	{
		ObjectDeclaration();
	public:
		Skeleton();
		virtual ~Skeleton();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		e2::Bone* boneByName(e2::Name name);
		uint32_t numBones();
		e2::Bone* boneById(uint32_t id);

		uint32_t numRootBones();
		e2::Bone* rootBoneById(uint32_t rootId);

		glm::mat4 const& inverseTransform();

	protected:
		glm::mat4 m_inverseTransform;
		e2::StackVector<e2::Bone, maxNumSkeletonBones> m_bones;
		e2::StackVector<e2::Bone*, maxNumRootBones> m_roots;

		std::unordered_map<e2::Name, e2::Bone*> m_boneMap;
	};







	enum class AnimationType : uint8_t
	{
		Float,
		Vec2,
		Vec3,
		Quat
	};

	struct AnimationTrackFrame
	{
		float asFloat();

		glm::vec2 asVec2();
		glm::vec3 asVec3();
		glm::quat asQuat();

		// x y z w, always
		float data[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	};

	struct AnimationTrack
	{

		std::pair<uint32_t, float> getFrameDelta(float time, float frameRate);

		float getFloat(float time, float frameRate);

		glm::vec2 getVec2(float time, float frameRate);

		glm::vec3 getVec3(float time, float frameRate);

		glm::quat getQuat(float time, float frameRate);

		e2::Name name;
		AnimationType type;
		e2::StackVector<AnimationTrackFrame, e2::maxNumTrackFrames> frames;
	};

	/** @tags(arena, arenaSize=1024, dynamic) */
	class E2_API Animation : public e2::Asset
	{
		ObjectDeclaration();
	public:
		Animation() = default;
		virtual ~Animation();

		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;


		e2::AnimationTrack* trackByName(e2::Name name, AnimationType type);

		uint32_t numFrames();
		float frameRate();
		float timeSeconds();
	protected:

		uint32_t m_numFrames{};
		float m_frameRate{};
		std::unordered_map<e2::Name, uint32_t> m_trackIndex;
		e2::StackVector<e2::AnimationTrack, e2::maxNumTracksPerAnimation> m_tracks;
	};

	struct PoseBone
	{
		glm::vec3 bindTranslation{ 0.0f, 0.0f, 0.0f };
		glm::quat bindRotation{ 1.0f, 0.0f, 0.0f, 0.0f };

		glm::vec3 localTranslation{ 0.0f, 0.0f, 0.0f };
		glm::quat localRotation{ 1.0f, 0.0f, 0.0f, 0.0f };

		glm::mat4 localTransform();

		glm::mat4 cachedGlobalTransform;
		glm::mat4 cachedSkinTransform;

		void updateSkin(glm::mat4 const& parentTransform, glm::mat4 const& globalInverseTransform, glm::mat4 const& inverseBindPose)
		{
			cachedGlobalTransform = parentTransform * localTransform();
			cachedSkinTransform = globalInverseTransform * cachedGlobalTransform * inverseBindPose;
		}

	};


	/*
	
	struct E2_API BoneMask
	{
	public:
		BoneMask(e2::Ptr<e2::Skeleton> skeleton, );
		e2::StackVector<uint32_t, e2::maxNumBonesInMask> boneIds;
	};*/

	/** @tags(arena, arenaSize=4096) */
	class E2_API Pose : public e2::Object
	{
		ObjectDeclaration();
	public:
		/** initialize pose from skeleton */
		Pose(e2::Ptr<e2::Skeleton> skeleton);
		virtual ~Pose();

		glm::quat const& localBoneRotation(uint32_t boneIndex);
		glm::vec3 const& localBoneTranslation(uint32_t boneIndex);
		glm::mat4 localBoneTransform(uint32_t boneIndex);

		void updateSkin();

		/** Applies bind pose (t-pose)*/
		void applyBindPose();

		/** Applies a blend between a and b on this pose */
		void applyBlend(Pose* a, Pose* b, float alpha);

		/** Blends this pose directly with b, applying it to this pose */
		void blendWith(Pose* b, float alpha);

		/** 
		 * Apply the given animation to this pose, at the given time in seconds
		 * Ignores any bones not driven by animation
		 */
		void applyAnimation(e2::Ptr<e2::Animation> anim, float time);

		e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> const& skin();
		e2::Ptr<e2::Skeleton> skeleton();

	protected:
		e2::Ptr<e2::Skeleton> m_skeleton;
		e2::StackVector<e2::PoseBone, e2::maxNumSkeletonBones> m_poseBones;
		e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> m_skin;
	};









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
		 * 1. Create a clean mesh: auto mesh = e2::MeshPtr::create();
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
		std::unordered_map<e2::Name, uint32_t> m_boneIndex;
		bool m_done{};
	};

}

#include "mesh.generated.hpp"