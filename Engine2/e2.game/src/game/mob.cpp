
#include "game/mob.hpp"
#include "game/wave.hpp"
#include "e2/renderer/meshproxy.hpp"
#include "game/game.hpp"
#include "e2/game/gamesession.hpp"

#include <e2/e2.hpp>
#include <e2/utils.hpp>
#include <e2/transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <nlohmann/json.hpp>

#include <fstream>

using json = nlohmann::json;

namespace
{


	std::vector<e2::MobSpecification> mobSpecifications;
	std::map<e2::Name, size_t> mobSpecificationIndex;
}

void e2::MobSpecification::initializeSpecifications(e2::GameContext* ctx)
{
	for (const std::filesystem::directory_entry& entry : std::filesystem::recursive_directory_iterator("data/mobs/"))
	{
		if (!entry.is_regular_file())
			continue;
		if (entry.path().extension() != ".json")
			continue;

		std::ifstream unitFile(entry.path());
		json data;
		try
		{
			data = json::parse(unitFile);
		}
		catch (json::parse_error& e)
		{
			LogError("failed to load specifications, json parse error: {}", e.what());
			continue;
		}

		for (json& mob : data.at("mobs"))
		{
			MobSpecification newSpec;

			if (!mob.contains("id"))
			{
				LogError("invalid mob specification: id required. skipping..");
				continue;
			}
			e2::Name id = mob.at("id").template get<std::string>();
			newSpec.id = id;


			if (!mob.contains("displayName"))
			{
				LogError("invalid mob specification: displayName required. skipping..");
				continue;
			}

			if (!mob.contains("mesh"))
			{
				LogError("invalid mob specification: mesh required. skipping..");
				continue;
			}

			newSpec.displayName = mob.at("displayName").template get<std::string>();

			if (mob.contains("passableFlags"))
			{
				for (json flag : mob.at("passableFlags"))
				{
					std::string flagStr = e2::toLower(flag.template get<std::string>());
					if (flagStr == "none")
						newSpec.passableFlags = e2::PassableFlags::None;
					else if (flagStr == "land")
						newSpec.passableFlags |= e2::PassableFlags::Land;
					else if (flagStr == "watershallow")
						newSpec.passableFlags |= e2::PassableFlags::WaterShallow;
					else if (flagStr == "waterdeep")
						newSpec.passableFlags |= e2::PassableFlags::WaterDeep;
					else if (flagStr == "water")
						newSpec.passableFlags |= e2::PassableFlags::WaterDeep | e2::PassableFlags::WaterShallow;
					else if (flagStr == "air")
						newSpec.passableFlags |= e2::PassableFlags::Air;
				}
			}


			if (mob.contains("moveType"))
			{
				std::string moveTypeStr = e2::toLower(mob.at("moveType").template get<std::string>());
				if (moveTypeStr == "static")
					newSpec.moveType = EntityMoveType::Static;
				else if (moveTypeStr == "linear")
					newSpec.moveType = EntityMoveType::Linear;
				else if (moveTypeStr == "smooth")
					newSpec.moveType = EntityMoveType::Smooth;
			}

			if (mob.contains("layerIndex"))
			{
				std::string layerIndexStr = e2::toLower(mob.at("layerIndex").template get<std::string>());
				if (layerIndexStr == "unit")
					newSpec.layerIndex = EntityLayerIndex::Unit;
				else if (layerIndexStr == "air")
					newSpec.layerIndex = EntityLayerIndex::Air;
				else if (layerIndexStr == "structure")
					newSpec.layerIndex = EntityLayerIndex::Structure;
			}



			if (mob.contains("maxHealth"))
				newSpec.maxHealth = mob.at("maxHealth").template get<float>();

			if (mob.contains("moveSpeed"))
				newSpec.moveSpeed = mob.at("moveSpeed").template get<float>();

			if (mob.contains("attackStrength"))
				newSpec.attackStrength = mob.at("attackStrength").template get<float>();

			if (mob.contains("defensiveStrength"))
				newSpec.defensiveStrength = mob.at("defensiveStrength").template get<float>();

			if (mob.contains("meshScale"))
			{
				json& meshScale = mob.at("meshScale");
				newSpec.meshScale = glm::vec3(meshScale[0].template get<float>(), meshScale[1].template get<float>(), meshScale[2].template get<float>());
			}

			if (mob.contains("meshHeightOffset"))
				newSpec.meshHeightOffset = mob.at("meshHeightOffset").template get<float>();

			if (mob.contains("reward"))
			{
				json& reward = mob.at("reward");
				if (reward.contains("gold"))
					newSpec.reward.gold = reward.at("gold").template get<float>();
				if (reward.contains("wood"))
					newSpec.reward.wood = reward.at("wood").template get<float>();
				if (reward.contains("iron"))
					newSpec.reward.iron = reward.at("iron").template get<float>();
				if (reward.contains("steel"))
					newSpec.reward.steel = reward.at("steel").template get<float>();
			}


			newSpec.meshAssetPath = mob.at("mesh").template get<std::string>();
			if (mob.contains("skeleton"))
				newSpec.skeletonAssetPath = mob.at("skeleton").template get<std::string>();

			if (mob.contains("runPoseAsset"))
				newSpec.runPoseAssetPath = mob.at("runPoseAsset").template get<std::string>();

			if (mob.contains("diePoseAsset"))
				newSpec.diePoseAssetPath = mob.at("diePoseAsset").template get<std::string>();

			//if (entity.contains("script"))
			//{
			//	chaiscript::ChaiScript* script = ctx->game()->scriptEngine();
			//	try
			//	{
			//		newSpec.scriptInterface = script->eval_file<TurnbasedScriptInterface>(entity.at("script").template get<std::string>());

			//	}
			//	catch (chaiscript::exception::eval_error& e)
			//	{
			//		LogError("chai: evaluation failed: {}", e.pretty_print());
			//	}
			//	catch (chaiscript::exception::bad_boxed_cast& e)
			//	{
			//		LogError("chai: casting return-type from script to native failed: {}", e.what());
			//	}
			//	catch (std::exception& e)
			//	{
			//		LogError("{}", e.what());
			//	}
			//}

			::mobSpecifications.push_back(newSpec);
			::mobSpecificationIndex[newSpec.id] = ::mobSpecifications.size() - 1;
		}

	}

}

void e2::MobSpecification::finalizeSpecifications(e2::Context* ctx)
{
	e2::AssetManager* am = ctx->assetManager();
	for (e2::MobSpecification& spec : ::mobSpecifications)
	{
		spec.meshAsset = am->get(spec.meshAssetPath).cast<e2::Mesh>();

		if (spec.skeletonAssetPath.size() > 0)
			spec.skeletonAsset = am->get(spec.skeletonAssetPath).cast<e2::Skeleton>();

		spec.runPoseAsset = am->get(spec.runPoseAssetPath).cast<e2::Animation>();
		spec.diePoseAsset = am->get(spec.diePoseAssetPath).cast<e2::Animation>();
	}
}

void e2::MobSpecification::destroySpecifications()
{
	mobSpecifications.clear();
	mobSpecificationIndex.clear();
}

e2::MobSpecification* e2::MobSpecification::specificationById(e2::Name id)
{
	auto finder = ::mobSpecificationIndex.find(id);
	if (finder == ::mobSpecificationIndex.end())
	{
		return nullptr;
	}

	return specification(finder->second);
}

e2::MobSpecification* e2::MobSpecification::specification(size_t index)
{
	return &::mobSpecifications[index];
}

size_t e2::MobSpecification::specificationCount()
{
	return ::mobSpecifications.size();
}

e2::Mob::Mob()
{
	meshTargetRotation = glm::angleAxis(0.0f, glm::vec3(e2::worldUp()));
	meshRotation = meshTargetRotation;
}

e2::Mob::Mob(e2::GameContext* ctx, e2::MobSpecification* spec, e2::Wave* wave)
	: e2::Mob::Mob()
{
	postConstruct(ctx, spec, wave);
}

e2::Mob::~Mob()
{
	destroyProxy();
	if (m_runPose)
		e2::destroy(m_runPose);
	if (m_diePose)
		e2::destroy(m_diePose);
}

bool e2::Mob::scriptEqualityPtr(Mob* lhs, Mob* rhs)
{
	return lhs == rhs;
}

e2::Mob* e2::Mob::scriptAssignPtr(Mob*& lhs, Mob* rhs)
{
	lhs = rhs;
	return lhs;
}

void e2::Mob::postConstruct(e2::GameContext* ctx, e2::MobSpecification* spec, e2::Wave* wave)
{
	m_wave = wave;
	m_game = ctx->game();

	specification = spec;

	health = spec->maxHealth;


}

void e2::Mob::initialize()
{
	if (specification->meshAsset && !meshProxy)
	{
		e2::MeshProxyConfiguration proxyConf{};
		e2::MeshLodConfiguration lod;

		lod.mesh = specification->meshAsset;
		proxyConf.lods.push(lod);

		meshProxy = e2::create<e2::MeshProxy>(gameSession(), proxyConf);
		meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
		meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale);

		meshProxy->modelMatrixDirty = true;
	}

	if (specification->meshAsset && specification->skeletonAsset && !skinProxy)
	{
		e2::SkinProxyConfiguration conf;
		conf.mesh = specification->meshAsset;
		conf.skeleton = specification->skeletonAsset;
		skinProxy = e2::create<e2::SkinProxy>(game()->gameSession(), conf);
		meshProxy->skinProxy = skinProxy;
		meshProxy->invalidatePipeline();

		m_mainPose = e2::create<e2::Pose>(specification->skeletonAsset);


		if(specification->runPoseAsset)
			m_runPose = e2::create<e2::AnimationPose>(specification->skeletonAsset, specification->runPoseAsset, true);

		if(specification->diePoseAsset)
			m_diePose = e2::create<e2::AnimationPose>(specification->skeletonAsset, specification->diePoseAsset, true);
	}

}

void e2::Mob::onHit(e2::TurnbasedEntity* instigator, float damage)
{
	if (health <= 0.0f)
		return;

	health -= damage;
}

void e2::Mob::updateAnimation(double seconds)
{

	meshRotation = glm::slerp(meshRotation, meshTargetRotation, glm::min(1.0f, float(10.0f * seconds)));

	if (skinProxy)
	{
		if (health > 0.0f && m_runPose)
			m_runPose->updateAnimation(seconds, !inView);
		else if(m_diePose)
			m_diePose->updateAnimation(seconds, !inView);

		m_mainPose->applyBindPose();

		if (health > 0.0f && m_runPose)
			m_mainPose->applyPose(m_runPose); 
		else if (m_diePose)
			m_mainPose->applyPose(m_diePose);

		m_mainPose->updateSkin();

		skinProxy->applyPose(m_mainPose);
	}

	if (meshProxy)
	{
		meshProxy->modelMatrix = glm::translate(glm::mat4(1.0), meshPosition + glm::vec3(e2::worldUp()) * specification->meshHeightOffset);
		meshProxy->modelMatrix = meshProxy->modelMatrix * glm::toMat4(meshRotation) * glm::scale(glm::mat4(1.0f), specification->meshScale);
		meshProxy->modelMatrixDirty = true;
	}
}

void e2::Mob::update(double seconds)
{
	if (!meshProxy->enabled())
		meshProxy->enable();

	m_unitMoveDelta += float(seconds) * specification->moveSpeed;


	uint64_t pathSize = m_wave->path.size();

	while (m_unitMoveDelta > 1.0f)
	{
		m_unitMoveDelta -= 1.0;
		m_unitMoveIndex++;

		if (m_unitMoveIndex >= pathSize - 1)
		{
			meshProxy->disable();

			m_game->applyDamage(m_wave->target, m_wave->hive, specification->attackStrength);

			m_wave->destroyMob(this);


			return;
		}
	}

	e2::Hex currHex = m_wave->path[m_unitMoveIndex];
	e2::Hex nextHex = m_wave->path[m_unitMoveIndex + 1];

	glm::vec3 currHexPos = currHex.localCoords();
	glm::vec3 nextHexPos = nextHex.localCoords();

	glm::vec3 newPos;
	float angle;

	if (specification->moveType == EntityMoveType::Linear)
	{
		newPos = glm::mix(currHexPos, nextHexPos, m_unitMoveDelta);
		angle = radiansBetween(nextHexPos, currHexPos);

	}
	else if (specification->moveType == EntityMoveType::Smooth)
	{
		glm::vec3 prevHexPos = m_unitMoveIndex > 0 ? m_wave->path[m_unitMoveIndex - 1].localCoords() : currHexPos - (nextHexPos - currHexPos);
		glm::vec3 nextNextHexPos = m_unitMoveIndex + 2 < pathSize ? m_wave->path[m_unitMoveIndex + 2].localCoords() : nextHexPos + (nextHexPos - currHexPos);


		newPos = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta);
		glm::vec3 newPos2 = glm::catmullRom(prevHexPos, currHexPos, nextHexPos, nextNextHexPos, m_unitMoveDelta + 0.01f);
		angle = radiansBetween(newPos2, newPos);
	}

	setMeshTransform(newPos, angle);
}

glm::vec2 e2::Mob::meshPlanarCoords()
{
	return glm::vec2(meshPosition.x, meshPosition.z);
}

void e2::Mob::setMeshTransform(glm::vec3 const& pos, float angle)
{
	meshTargetRotation = glm::angleAxis(angle, glm::vec3(e2::worldUp()));
	meshPosition = pos;
}

e2::Game* e2::Mob::game()
{
	return m_game;
}

void e2::Mob::destroyProxy()
{
	if (skinProxy)
		e2::destroy(skinProxy);

	if (meshProxy)
		e2::destroy(meshProxy);

	meshProxy = nullptr;
	skinProxy = nullptr;
}
