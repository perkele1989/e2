
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

		// geometry-to-bone (inverse bind pose)
		glm::mat4 bindMatrix;

		// node-to-parent 
		glm::mat4 localTransform;

		e2::Bone* parent{};
		e2::StackVector<e2::Bone*, maxNumBoneChildren> children;

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

		std::pair<uint32_t, double> getFrameDelta(double time, double frameRate);

		float getFloat(double time, double frameRate, bool wrap);

		glm::vec2 getVec2(double time, double frameRate, bool wrap);

		glm::vec3 getVec3(double time, double frameRate, bool wrap);

		glm::quat getQuat(double time, double frameRate, bool wrap);

		e2::Name name;
		AnimationType type;
		std::vector<AnimationTrackFrame> frames;
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
		double frameRate();
		double timeSeconds();
	protected:

		uint32_t m_numFrames{};
		double m_frameRate{};
		std::unordered_map<e2::Name, uint32_t> m_trackIndex;
		std::vector<e2::AnimationTrack> m_tracks;
	};


	/** @tags(arena, arenaSize=16384)*/
	struct E2_API AnimationBinding
	{
		std::vector<e2::AnimationTrack*> translationTracks;
		std::vector<e2::AnimationTrack*> rotationTracks;
		std::vector<e2::AnimationTrack*> scaleTracks;
	};

	class Animation;

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

		e2::AnimationBinding* getOrCreateBinding(e2::Ptr<Animation> anim);

	protected:
		glm::mat4 m_inverseTransform;
		e2::StackVector<e2::Bone, maxNumSkeletonBones> m_bones;
		e2::StackVector<e2::Bone*, maxNumRootBones> m_roots;

		std::unordered_map<e2::Name, e2::Bone*> m_boneMap;
		std::unordered_map<e2::UUID, e2::AnimationBinding*> m_animationBindings;
	};









	struct PoseBone
	{
		e2::Bone* assetBone{};

		uint32_t id{};
		glm::mat4 localTransform;

		glm::mat4 cachedGlobalTransform;
		glm::mat4 cachedSkinTransform;

	};

	class E2_API Pose : public e2::Object, public e2::Context
	{
		ObjectDeclaration();
	public:
		/** initialize pose from skeleton */
		Pose() = default;
		Pose(e2::Ptr<e2::Skeleton> skeleton);
		virtual ~Pose();

		virtual Engine* engine() override;

		virtual void updateAnimation(double timeDelta, bool onlyTickTime);

		void updateSkin();

		/** Applies bind pose (t-pose) */
		void applyBindPose();

		void applyPose(Pose* otherPose);

		/** Applies a blend between a and b on this pose */
		void applyBlend(Pose* a, Pose* b, double alpha);

		/** Blends this pose directly with b, applying it to this pose */
		void blendWith(Pose* b, double alpha);

		/** 
		 * Apply the given animation to this pose, at the given time in seconds
		 * Ignores any bones not driven by animation
		 * SLOW
		 */
		//void applyAnimation(e2::Ptr<e2::Animation> anim, double time);

		e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> const& skin();
		e2::Ptr<e2::Skeleton> skeleton();

		e2::PoseBone* poseBoneById(uint32_t id);

	protected:
		e2::Ptr<e2::Skeleton> m_skeleton;
		e2::StackVector<e2::PoseBone, e2::maxNumSkeletonBones> m_poseBones;
		e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> m_skin;
	};

	/** @tags(arena, arenaSize=16384) */
	class E2_API AnimationPose : public e2::Pose
	{
		ObjectDeclaration();
	public:
		AnimationPose() = default;
		AnimationPose(e2::Ptr<e2::Skeleton> skeleton, e2::Ptr<e2::Animation> animation, bool loop);
		virtual ~AnimationPose();

		virtual void updateAnimation(double timeDelta, bool onlyTickTime) override;

		void pause()
		{
			m_playing = false;
		}

		void resume()
		{
			m_playing = true;
		}

		void stop()
		{
			m_time = 0.0f;
			m_playing = false;
		}

		void play(bool loop)
		{
			m_loop = loop;
			m_time = 0.0f;
			m_playing = true;
		}

		inline double time() const
		{
			return m_time;
		}

		inline bool playing() const
		{
			return m_playing;
		}

		inline bool loop() const
		{
			return m_loop;
		}

		e2::Ptr<e2::Animation> animation() const
		{
			return m_animation;
		}

		inline uint32_t frameIndex()
		{
			return m_frameIndex;
		}

	protected:
		e2::Ptr<e2::Animation> m_animation;
		e2::AnimationBinding* m_binding{};

		bool m_loop{};
		bool m_playing{};
		double m_time{};
		uint32_t m_frameIndex{};
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

		int32_t boneIndexByName(e2::Name name)
		{
			auto finder = m_boneIndex.find(name);
			if (finder == m_boneIndex.end())
			{
				return -1;
			}

			return finder->second;
		}

	protected:
		e2::StackVector<e2::SubmeshSpecification, e2::maxNumSubmeshes> m_specifications;
		e2::StackVector<e2::MaterialPtr, e2::maxNumSubmeshes> m_materials;
		std::unordered_map<e2::Name, uint32_t> m_boneIndex;
		bool m_done{};
	};

}

#include "mesh.generated.hpp"