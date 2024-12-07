#pragma once 

#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/assets/mesh.hpp>
#include <nlohmann/json.hpp>

namespace e2
{

	class ITriggerListener
	{
	public:
		virtual ~ITriggerListener() = default;
		virtual void onTrigger(e2::Name actionId, e2::Name triggerId)=0;
	};

	constexpr uint32_t maxNumTriggersPerAction = 4;

	constexpr uint32_t maxNumPosesPerMesh = 8;
	constexpr uint32_t maxNumActionsPerMesh = 8;


	struct TriggerSpecification
	{
		e2::Name id;
		double time{};
	};

	struct PoseSpecification
	{
		e2::Name animationAssetName;
		e2::AnimationPtr animationAsset;
		float blendTime{ 0.0f };
		float speed{ 1.0f };

		e2::StackVector<e2::TriggerSpecification, e2::maxNumTriggersPerAction> triggers;
	};

	struct ActionSpecification
	{
		e2::Name animationAssetName;
		e2::AnimationPtr animationAsset;
		float blendInTime{ 0.0f };
		float blendOutTime{ 0.0f };
		float speed{ 1.0f };

		e2::StackVector<e2::TriggerSpecification, e2::maxNumTriggersPerAction> triggers;
	};

	class GameContext;

	struct SkeletalMeshSpecification
	{
		void populate(nlohmann::json &obj, std::unordered_set<e2::Name> &deps);
		void finalize(e2::GameContext *ctx);

		glm::vec3 scale;

		/** Mesh asset to use */
		e2::Name meshAssetName;
		e2::MeshPtr meshAsset;

		e2::StackVector<e2::Name, e2::maxNumSubmeshes> materialAssetNames;
		e2::StackVector<e2::MaterialPtr, e2::maxNumSubmeshes>  materialAssets;

		/** Skeleton asset to use */
		e2::Name skeletonAssetName;
		e2::SkeletonPtr skeletonAsset;

		e2::Name defaultPose;

		/** Animation poses */
		std::unordered_map<e2::Name, e2::PoseSpecification> poses;

		/** Animation actions*/
		std::unordered_map<e2::Name, e2::ActionSpecification> actions;
	};

	struct SkeletalMeshTrigger
	{
		e2::Name id;
		double time{};

		bool triggered{};
	};

	struct SkeletalMeshAction
	{
		e2::Name id;
		e2::AnimationPose* pose{};
		float blendInTime{};
		float blendOutTime{};
		float speed{ 1.0f };

		e2::StackVector<e2::SkeletalMeshTrigger, e2::maxNumTriggersPerAction> triggers;
	};

	struct SkeletalMeshPose
	{
		e2::Name id;
		e2::AnimationPose* pose{};
		float blendTime{};
		float speed{ 1.0f };

		e2::StackVector<e2::SkeletalMeshTrigger, e2::maxNumTriggersPerAction> triggers;
	};

	class Entity;
	class MeshProxy;
	class SkinProxy;

	/** @tags(arena, arenaSize=4096) */
	class SkeletalMeshComponent : public e2::Object
	{
		ObjectDeclaration();
	public:
		SkeletalMeshComponent(e2::SkeletalMeshSpecification* specification, e2::Entity *entity);
		virtual ~SkeletalMeshComponent();

		void updateVisibility();

		void updateAnimation(double seconds);

		glm::mat4 getScaleTransform();

		void setPose(e2::Name poseName);
		void playAction(e2::Name actionName);
		bool isActionPlaying(e2::Name actionName);
		bool isAnyActionPlaying();
		void stopAction();

		void setPose2(e2::Pose* pose, double lerpTime);
		void playAction2(e2::AnimationPose* anim, double blendIn = 0.2f, double blendOut = 0.2f, double speed = 1.0);

		void setTriggerListener(ITriggerListener* listener);

		inline void setPoseSpeed(double newSpeed)
		{
			m_poseSpeed = newSpeed;
		}

		void applyTransform();
		void applyCustomTransform(glm::mat4 const& transform);

		inline void setHeightOffset(float newOffset)
		{
			m_heightOffset = newOffset;
		}

		glm::mat4 const& boneTransform(e2::Name name);

	protected:

		e2::Entity* m_entity{};
		e2::SkeletalMeshSpecification* m_specification{};

		ITriggerListener* m_triggerListener{};

		e2::MeshProxy* m_meshProxy{};
		e2::SkinProxy* m_skinProxy{};

		float m_heightOffset{};

		e2::Pose* m_mainPose{};

		double m_lerpTime{};
		e2::Moment m_lastChangePose;
		e2::Pose* m_currentPose{};
		e2::Pose* m_oldPose{};
		double m_poseSpeed{ 1.0f };
		double m_lastPoseTime = 0.0f;
		e2::SkeletalMeshPose* m_currentMeshPose{};

		e2::AnimationPose* m_actionPose{};
		double m_actionBlendInTime = 0.2;
		double m_actionBlendOutTime = 0.2;
		double m_actionSpeed = 1.0;
		double m_lastActionTime = 0.0;
		e2::SkeletalMeshAction* m_currentAction{};

		e2::StackVector<SkeletalMeshPose, e2::maxNumPosesPerMesh> m_animationPoses;
		e2::StackVector<SkeletalMeshAction, e2::maxNumActionsPerMesh> m_animationActions;
	};


}


#include "skeletalmeshcomponent.generated.hpp"