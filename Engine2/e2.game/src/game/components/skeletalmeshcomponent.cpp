
#include "game/components/skeletalmeshcomponent.hpp"
#include "game/entity.hpp"
#include "game/game.hpp"

#include <e2/game/gamesession.hpp>

e2::SkeletalMeshComponent::SkeletalMeshComponent(e2::SkeletalMeshSpecification* specification, e2::Entity* entity)
	: m_specification(specification)
	, m_entity(entity)
{
	e2::Session* session = m_entity->gameSession();

	for (auto& [id, action] : m_specification->actions)
	{
		e2::SkeletalMeshAction newAction;

		newAction.id = id;
		newAction.pose = e2::create<e2::AnimationPose>(m_specification->skeletonAsset, action.animationAsset, false);
		newAction.blendInTime = action.blendInTime;
		newAction.blendOutTime = action.blendOutTime;
		newAction.speed = action.speed;

		for (e2::TriggerSpecification& trigger : action.triggers)
		{
			e2::SkeletalMeshTrigger newTrigger;
			newTrigger.id = trigger.id;
			newTrigger.time = trigger.time;
			newTrigger.triggered = false;

			newAction.triggers.push(newTrigger);
		}

		m_animationActions.push(newAction);
	}

	for (auto& [id, pose] : m_specification->poses)
	{
		e2::SkeletalMeshPose newPose;

		newPose.id = id;
		newPose.pose = e2::create<e2::AnimationPose>(m_specification->skeletonAsset, pose.animationAsset, true);
		newPose.blendTime = pose.blendTime;
		newPose.speed = pose.speed;

		for (e2::TriggerSpecification& trigger : pose.triggers)
		{
			e2::SkeletalMeshTrigger newTrigger;
			newTrigger.id = trigger.id;
			newTrigger.time = trigger.time;
			newTrigger.triggered = false;

			newPose.triggers.push(newTrigger);
		}

		m_animationPoses.push(newPose);
	}


	if (m_specification->meshAsset)
	{
		e2::MeshProxyConfiguration proxyConf{};
		e2::MeshLodConfiguration lod;

		for (e2::MaterialPtr material : m_specification->materialAssets)
		{
			lod.materials.push(session->getOrCreateDefaultMaterialProxy(material));
		}
		

		lod.mesh = m_specification->meshAsset;
		proxyConf.lods.push(lod);

		m_meshProxy = e2::create<e2::MeshProxy>(session, proxyConf);
	}

	if (m_specification->meshAsset && m_specification->skeletonAsset)
	{
		e2::SkinProxyConfiguration conf;
		conf.mesh = m_specification->meshAsset;
		conf.skeleton = m_specification->skeletonAsset;
		m_skinProxy = e2::create<e2::SkinProxy>(session, conf);
		m_meshProxy->skinProxy = m_skinProxy;
		m_meshProxy->invalidatePipeline();

		m_mainPose = e2::create<e2::Pose>(m_specification->skeletonAsset);
	}

	if (m_specification->defaultPose.index() > 0)
		setPose(m_specification->defaultPose);
}

e2::SkeletalMeshComponent::~SkeletalMeshComponent()
{
	for (e2::SkeletalMeshAction& action : m_animationActions)
		e2::destroy(action.pose);

	for (e2::SkeletalMeshPose& pose : m_animationPoses)
		e2::destroy(pose.pose);

	if (m_skinProxy)
		e2::destroy(m_skinProxy);

	if (m_meshProxy)
		e2::destroy(m_meshProxy);
}

void e2::SkeletalMeshComponent::updateVisibility()
{
	if (!m_meshProxy)
		return;

	bool proxyEnabled = m_meshProxy->enabled();
	bool inView = m_entity->isInView();

	if (inView && !proxyEnabled)
	{
		m_meshProxy->enable();
		applyTransform();
	}
	else if (!inView && proxyEnabled)
	{
		m_meshProxy->disable();
	}
}

void e2::SkeletalMeshComponent::applyTransform()
{
	if (m_meshProxy)
	{
		glm::mat4 heightOffset = glm::translate(glm::identity<glm::mat4>(), { 0.0f, m_heightOffset, 0.0f });
		m_meshProxy->modelMatrix = heightOffset * m_entity->getTransform()->getTransformMatrix(e2::TransformSpace::World) * getScaleTransform();
		m_meshProxy->modelMatrixDirty = true;
	}
}


void e2::SkeletalMeshComponent::applyCustomTransform(glm::mat4 const& transform)
{
	if (m_meshProxy)
	{
		m_meshProxy->modelMatrix = transform;
		m_meshProxy->modelMatrixDirty = true;
	}
}

glm::mat4 const& e2::SkeletalMeshComponent::boneTransform(e2::Name name)
{
	static glm::mat4 fallback{glm::identity<glm::mat4>()};

	e2::SkeletonPtr skeleton = m_specification->skeletonAsset;
	if (!skeleton)
		return fallback;
	e2::Bone* bone = skeleton->boneByName(name);
	if (!bone)
		return fallback;
	
	uint32_t boneId = bone->index;
	if (boneId >= e2::maxNumSkeletonBones)
		return fallback;

	e2::PoseBone *poseBone = m_mainPose->poseBoneById(boneId);
	if (!poseBone)
		return fallback;

	return poseBone->cachedGlobalTransform;
	//return poseBone->cachedGlobalTransform;

	//return m_mainPose->skin()[boneId];
}

void e2::SkeletalMeshComponent::updateAnimation(double seconds)
{
	if (!m_meshProxy)
		return;

	bool proxyEnabled = m_meshProxy->enabled();
	bool inView = m_entity->isInView();

	if (!m_skinProxy)
		return;

	if (inView)
		m_mainPose->applyBindPose();

	double blendTime = m_lastChangePose.durationSince().seconds();
	double poseBlend = 0.0;
	if (m_lerpTime >= 0.0001)
	{
		poseBlend = glm::clamp(blendTime / m_lerpTime, 0.0, 1.0);
		if (poseBlend >= 1.0)
			m_oldPose = nullptr;
	}
	else
		m_oldPose = nullptr;

	if (m_oldPose)
		m_oldPose->updateAnimation(seconds, !inView);

	if (m_currentPose)
	{
		m_currentPose->updateAnimation(seconds * m_poseSpeed, !inView);

		e2::AnimationPose* poseAnim = m_currentPose->cast<e2::AnimationPose>();
		if (poseAnim && m_currentMeshPose)
		{
			double currentTime = poseAnim->time();
			double lastTime = m_lastPoseTime;
			m_lastPoseTime = currentTime;

			bool loopedAround = currentTime < lastTime;
			if (loopedAround)
			{
				for (e2::SkeletalMeshTrigger& trigger : m_currentMeshPose->triggers)
				{
					if (trigger.time > lastTime)
					{
						if (!trigger.triggered)
						{
							if (m_triggerListener)
							{
								m_triggerListener->onTrigger(m_currentMeshPose->id, trigger.id);
							}
						}
					}
					trigger.triggered = false;
				}
				lastTime = 0.0f;
			}

			for (e2::SkeletalMeshTrigger& trigger : m_currentMeshPose->triggers)
			{
				bool triggerInRange = trigger.time <= currentTime && trigger.time >= lastTime;
				if (triggerInRange && !trigger.triggered)
				{
					if (m_triggerListener)
					{
						m_triggerListener->onTrigger(m_currentMeshPose->id, trigger.id);
					}
					trigger.triggered = true;
				}
			}

		}

	}

	if (inView)
	{
		if (m_oldPose && m_currentPose)
		{
			m_mainPose->applyBlend(m_oldPose, m_currentPose, poseBlend);
		}
		else if (m_currentPose)
		{
			m_mainPose->applyPose(m_currentPose);
		}
	}

	if (m_actionPose)
	{
		m_actionPose->updateAnimation(seconds * m_actionSpeed, !inView);
		if (m_actionPose->playing())
		{
			double actionCurrentTime = m_actionPose->time();

			if (m_currentAction)
			{
				for (e2::SkeletalMeshTrigger& trigger : m_currentAction->triggers)
				{
					bool triggerInRange = trigger.time <= actionCurrentTime && trigger.time >= m_lastActionTime;
					if (triggerInRange && !trigger.triggered)
					{
						if (m_triggerListener)
						{
							m_triggerListener->onTrigger(m_currentAction->id, trigger.id);
						}
						trigger.triggered = true;
					}
				}
			}


			// 
			m_lastActionTime = actionCurrentTime;

			if (inView)
			{
				double actionTotalTime = m_actionPose->animation()->timeSeconds();
				double blendInCoeff = glm::smoothstep(0.0, m_actionBlendInTime, actionCurrentTime);
				double blendOutCoeff = 1.0 - glm::smoothstep(actionTotalTime - m_actionBlendOutTime, actionTotalTime, actionCurrentTime);
				double actionBlendCoeff = blendInCoeff * blendOutCoeff;


				m_mainPose->blendWith(m_actionPose, actionBlendCoeff);
			}
		}
		else
		{
			m_actionPose = nullptr;

			if (m_currentAction)
			{
				// trigger any untriggered events (may be last frame or smth)
				// and then set triggered to false for all of them
				for (e2::SkeletalMeshTrigger& trigger : m_currentAction->triggers)
				{
					if (!trigger.triggered && m_triggerListener)
					{
						m_triggerListener->onTrigger(m_currentAction->id, trigger.id);
					}

					trigger.triggered = false;
				}
			}
			m_currentAction = nullptr;
		}

	}

	if (inView)
	{
		m_mainPose->updateSkin();
		m_skinProxy->applyPose(m_mainPose);
	}


}

glm::mat4 e2::SkeletalMeshComponent::getScaleTransform()
{
	return glm::scale(glm::identity<glm::mat4>(), m_specification->scale);
}

void e2::SkeletalMeshComponent::setPose(e2::Name poseName)
{
	e2::SkeletalMeshPose* pose{};

	for (e2::SkeletalMeshPose& p : m_animationPoses)
	{
		if (p.id == poseName)
		{
			pose = &p;
			break;
		}
	}

	if (!pose)
		return;

	if (m_currentMeshPose != pose)
	{
		m_currentMeshPose = pose;
		for (e2::SkeletalMeshTrigger& trigger : m_currentMeshPose->triggers)
		{
			trigger.triggered = false;
			m_lastPoseTime = 0.0f;
		}
	}

	setPose2(pose->pose, pose->blendTime);
}

void e2::SkeletalMeshComponent::setPose2(e2::Pose* pose, double lerpTime)
{
	if (!m_skinProxy)
		return;

	if (pose == m_currentPose)
		return;

	if (lerpTime <= 0.00001)
		m_oldPose = nullptr;
	else
		m_oldPose = m_currentPose;

	m_currentPose = pose;

	m_lerpTime = lerpTime;
	m_lastChangePose = e2::timeNow();
}

void e2::SkeletalMeshComponent::playAction(e2::Name actionName)
{
	e2::SkeletalMeshAction* action{};

	for (e2::SkeletalMeshAction& a : m_animationActions)
	{
		if (a.id == actionName)
		{
			action = &a;
			break;
		}
	}

	if (!action)
		return;

	m_currentAction = action;
	for (auto& t : m_currentAction->triggers)
	{
		t.triggered = false;
	}

	m_lastActionTime = 0.0f;

	playAction2(action->pose, action->blendInTime, action->blendOutTime, action->speed);
}

bool e2::SkeletalMeshComponent::isActionPlaying(e2::Name actionName)
{
	e2::SkeletalMeshAction* action{};

	for (e2::SkeletalMeshAction& a : m_animationActions)
	{
		if (a.id == actionName)
		{
			action = &a;
			break;
		}
	}

	if (!action)
		return false;

	return m_actionPose && (m_actionPose == action->pose) && m_actionPose->playing();
}

bool e2::SkeletalMeshComponent::isAnyActionPlaying()
{
	return m_actionPose && m_actionPose->playing();
}

void e2::SkeletalMeshComponent::stopAction()
{
	if (m_actionPose)
		m_actionPose->stop();
}

void e2::SkeletalMeshComponent::playAction2(e2::AnimationPose* anim, double blendIn /*= 0.2f*/, double blendOut /*= 0.2f*/, double speed)
{
	if (!m_skinProxy)
		return;

	m_actionPose = anim;
	m_actionBlendInTime = blendIn;
	m_actionBlendOutTime = blendOut;
	m_actionSpeed = speed;

	m_actionPose->play(false);
}

void e2::SkeletalMeshComponent::setTriggerListener(ITriggerListener* listener)
{
	m_triggerListener = listener;
}

void e2::SkeletalMeshSpecification::populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps)
{
	if (obj.contains("mesh"))
	{
		meshAssetName = obj.at("mesh").template get<std::string>();
		deps.insert(meshAssetName);
	}

	if (obj.contains("materials"))
	{
		for (nlohmann::json& material : obj.at("materials"))
		{
			e2::Name materialName = material.template get<std::string>();
			materialAssetNames.push(materialName);
			deps.insert(materialName);
		}
	}

	if (obj.contains("skeleton"))
	{
		skeletonAssetName = obj.at("skeleton").template get<std::string>();
		deps.insert(skeletonAssetName);
	}

	if (obj.contains("defaultPose"))
	{
		defaultPose = obj.at("defaultPose").template get<std::string>();
	}

	if (obj.contains("poses"))
	{
		nlohmann::json& objPoses = obj.at("poses");
		for (nlohmann::json& pose : objPoses)
		{
			if (!pose.contains("blendTime") || !pose.contains("asset") || !pose.contains("name"))
			{
				LogError("invalid pose; blendTime, asset and name required.. ");
				continue;
			}

			e2::PoseSpecification newPose;
			newPose.blendTime = pose.at("blendTime").template get<float>();
			newPose.animationAssetName = pose.at("asset").template get<std::string>();
			deps.insert(newPose.animationAssetName);

			std::string poseName = pose.at("name").template get<std::string>();

			if (pose.contains("speed"))
				newPose.speed = pose.at("speed").template get<float>();

			if (pose.contains("triggers"))
			{
				for (nlohmann::json& trigger : pose.at("triggers"))
				{
					if (!trigger.contains("id") || !trigger.contains("time"))
					{
						LogError("invalid action trigger: id, time required.. ");
						continue;
					}

					TriggerSpecification newTrigger;
					newTrigger.id = trigger.at("id").template get<std::string>();
					newTrigger.time = trigger.at("time").template get<double>();

					newPose.triggers.push(newTrigger);
				}
			}

			poses[poseName] = newPose;
		}
	}

	if (obj.contains("meshScale"))
	{
		scale.x = obj.at("meshScale")[0].template get<float>();
		scale.y = obj.at("meshScale")[1].template get<float>();
		scale.z = obj.at("meshScale")[2].template get<float>();
	}

	if (obj.contains("actions"))
	{
		nlohmann::json& objActions = obj.at("actions");
		for (nlohmann::json& action : objActions)
		{
			if (!action.contains("blendInTime") || !action.contains("blendOutTime") || !action.contains("asset") || !action.contains("name"))
			{
				LogError("invalid action; blendInTime, blendOutTime, asset and name required.. ");
				continue;
			}

			e2::ActionSpecification newAction;
			newAction.blendInTime = action.at("blendInTime").template get<float>();
			newAction.blendOutTime = action.at("blendOutTime").template get<float>();

			if (action.contains("speed"))
				newAction.speed = action.at("speed").template get<float>();

			newAction.animationAssetName = action.at("asset").template get<std::string>();
			deps.insert(newAction.animationAssetName);

			std::string actionName = action.at("name").template get<std::string>();

			if (action.contains("triggers"))
			{
				for (nlohmann::json& trigger : action.at("triggers"))
				{
					if (!trigger.contains("id") || !trigger.contains("time"))
					{
						LogError("invalid action trigger: id, time required.. ");
						continue;
					}

					TriggerSpecification newTrigger;
					newTrigger.id = trigger.at("id").template get<std::string>();
					newTrigger.time = trigger.at("time").template get<double>();

					newAction.triggers.push(newTrigger);
				}
			}

			actions[actionName] = newAction;
		}
	}

}

void e2::SkeletalMeshSpecification::finalize(e2::GameContext* ctx)
{
	
	e2::AssetManager *am = ctx->game()->assetManager();


	meshAsset = am->get(meshAssetName).cast<e2::Mesh>();

	if (materialAssetNames.size() > 0)
	{
		for (e2::Name materialName : materialAssetNames)
		{
			e2::MaterialPtr material = am->get(materialName).cast<e2::Material>();
			materialAssets.push(material);
		}		
	}
	
	if (skeletonAssetName.index() != 0)
		skeletonAsset = am->get(skeletonAssetName).cast<e2::Skeleton>();

	for (auto& [name, pose] : poses)
	{
		pose.animationAsset = am->get(pose.animationAssetName).cast<e2::Animation>();
	}

	for (auto& [name, action] : actions)
	{
		action.animationAsset = am->get(action.animationAssetName).cast<e2::Animation>();
	}
}
