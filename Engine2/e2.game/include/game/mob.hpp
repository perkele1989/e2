
#pragma once 

#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/ui/uicontext.hpp>

#include <e2/renderer/meshproxy.hpp>
#include <game/gamecontext.hpp>
#include "game/shared.hpp"

#include "game/resources.hpp"

#include <chaiscript/chaiscript.hpp>
#include <string>


namespace e2
{
	class MeshProxy;
	class Wave;
	class Mob;


	struct MobSpecification
	{
		static void initializeSpecifications(e2::GameContext* ctx);
		static void finalizeSpecifications(e2::Context* ctx);
		static void destroySpecifications();
		static e2::MobSpecification* specificationById(e2::Name id);
		static e2::MobSpecification* specification(size_t index);
		static size_t specificationCount();

		/** The global identifier for this entity, i.e. its identifiable name */
		e2::Name id;

		/** Display name */
		std::string displayName;

		/** The layer index for this entity, only one entity of each layer can fit on a given tile index */
		EntityLayerIndex layerIndex = EntityLayerIndex::Unit;

		/** The move type for this entity */
		EntityMoveType moveType{ EntityMoveType::Smooth };

		/** The move speed */
		float moveSpeed{ 1.0f };

		/** The passable flags, i.e. what can this entity traverse. */
		e2::PassableFlags passableFlags{ e2::PassableFlags::None };

		/** Starting max. health */
		float maxHealth{ 100.0f };
		float attackStrength{ 0.0f };
		float defensiveStrength{ 0.0f };


		/** Reward from killing this mob */
		ResourceTable reward;

		/** Mesh scale */
		glm::vec3 meshScale{ 1.0f, 1.0f, 1.0f };

		/** Mesh height offset */
		float meshHeightOffset{ 0.0f };

		/** Mesh asset to use */
		std::string meshAssetPath;
		e2::MeshPtr meshAsset;

		/** Skeleton asset to use */
		std::string skeletonAssetPath;
		e2::SkeletonPtr skeletonAsset;

		std::string runPoseAssetPath;
		e2::AnimationPtr runPoseAsset;

		std::string diePoseAssetPath;
		e2::AnimationPtr diePoseAsset;
	};









	/**
	 * A mob, i.e. unit in a wave
	 * @tags(dynamic, arena, arenaSize=1024)
	 */
	class Mob : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		Mob();
		Mob(e2::GameContext* ctx, e2::MobSpecification* spec, e2::Wave* wave);
		virtual ~Mob();

		static bool scriptEqualityPtr(Mob* lhs, Mob* rhs);
		static Mob* scriptAssignPtr(Mob*& lhs, Mob* rhs);


		void postConstruct(e2::GameContext* ctx, e2::MobSpecification* spec, e2::Wave* wave);
		virtual void initialize();

		virtual void onHit(e2::GameEntity* instigator, float damage);

		virtual void updateAnimation(double seconds);

		virtual void update(double seconds);

		glm::vec2 meshPlanarCoords();

		void setMeshTransform(glm::vec3 const& pos, float angle);

		virtual e2::Game* game() override;

		e2::MobSpecification* specification{};

		/** Updated automatically by the game, if true this mob is in view. */
		bool inView{true};

		float health{};

		e2::MeshProxy* meshProxy{};
		e2::SkinProxy* skinProxy{};

		glm::quat meshRotation{};
		glm::quat meshTargetRotation{};
		glm::vec3 meshPosition{};

		uint32_t m_unitMoveIndex{};
		float m_unitMoveDelta{};

	protected:
		e2::Game* m_game{};
		e2::Wave* m_wave{};

		void destroyProxy();

		//  -- internal animation states
		e2::Pose* m_mainPose{};
		e2::AnimationPose* m_runPose{};
		e2::AnimationPose* m_diePose{};
	};
}

#include "mob.generated.hpp"