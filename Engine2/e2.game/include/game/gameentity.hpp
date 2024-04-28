
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

	constexpr uint32_t maxNumTriggersPerAction = 4;

	class GameEntity;

	struct EntityPoseSpecification
	{
		std::string animationAssetPath;
		e2::AnimationPtr animationAsset;
		float blendTime{0.0f};
	};

	struct EntityActionTriggerSpecification
	{
		e2::Name id;
		double time {};
	};

	struct EntityActionSpecification
	{
		std::string animationAssetPath;
		e2::AnimationPtr animationAsset;
		float blendInTime{ 0.0f };
		float blendOutTime{ 0.0f };
		float speed{ 1.0f };

		e2::StackVector<e2::EntityActionTriggerSpecification, e2::maxNumTriggersPerAction> triggers;

	};

	enum class EntityMoveType : uint8_t
	{
		Static,
		Linear,
		Smooth
	};


	using scriptFunc_drawUI = std::function<void(e2::GameEntity*, e2::UIContext*)>;
	using scriptFunc_update = std::function<void(e2::GameEntity*, double)>;
	using scriptFunc_updateAnimation = std::function<void(e2::GameEntity*, double)>;
	using scriptFunc_grugTick = std::function<bool(e2::GameEntity*, double)>;
	using scriptFunc_grugRelevant = std::function<bool(e2::GameEntity*)>;
	using scriptFunc_fiscal = std::function<void(e2::GameEntity*, e2::ResourceTable&)>;
	using scriptFunc_onHit = std::function<void(e2::GameEntity*, e2::GameEntity*, float)>;
	using scriptFunc_onTargetChanged = std::function<void(e2::GameEntity*, glm::ivec2 const&)>;
	using scriptFunc_onTargetClicked = std::function<void(e2::GameEntity*)>;
	using scriptFunc_updateCustomAction = std::function<void(e2::GameEntity*, double)>;
	using scriptFunc_onActionTrigger = std::function<void(e2::GameEntity*, std::string, std::string)>;
	using scriptFunc_onTurnStart = std::function<void(e2::GameEntity*)>;
	using scriptFunc_onTurnEnd = std::function<void(e2::GameEntity*)>;
	using scriptFunc_onBeginMove = std::function<void(e2::GameEntity*)>;
	using scriptFunc_onEndMove = std::function<void(e2::GameEntity*)>;
	using scriptFunc_createState = std::function<chaiscript::Boxed_Value(e2::GameEntity*)>;

	struct EntityScriptInterface
	{
		EntityScriptInterface() = default;

		chaiscript::Boxed_Value invokeCreateState(e2::GameEntity* entity);
		void invokeDrawUI(e2::GameEntity* entity, e2::UIContext* ui);

		void invokeUpdate(e2::GameEntity* entity, double seconds);

		void invokeUpdateAnimation(e2::GameEntity* entity, double seconds);
		bool invokeGrugRelevant(e2::GameEntity* entity);
		bool invokeGrugTick(e2::GameEntity* entity, double seconds);
		void invokeCollectRevenue(e2::GameEntity* entity, e2::ResourceTable &outTable);
		void invokeCollectExpenditure(e2::GameEntity* entity, e2::ResourceTable& outTable);
		void invokeOnHit(e2::GameEntity* entity, e2::GameEntity* instigator, float dmg);
		void invokeOnTargetChanged(e2::GameEntity* entity, glm::ivec2 const& hex);
		void invokeOnTargetClicked(e2::GameEntity* entity);
		void invokeUpdateCustomAction(e2::GameEntity* entity, double seconds);
		void invokeOnActionTrigger(e2::GameEntity* entity, e2::Name action, e2::Name trigger);
		void invokeOnTurnStart(e2::GameEntity* entity);
		void invokeOnTurnEnd(e2::GameEntity* entity);
		void invokeOnBeginMove(e2::GameEntity* entity);
		void invokeOnEndMove(e2::GameEntity* entity);

		void setCreateState(scriptFunc_createState func);
		void setDrawUI(scriptFunc_drawUI func);
		void setUpdate(scriptFunc_update func);
		void setUpdateAnimation(scriptFunc_updateAnimation func);
		void setGrugRelevant(scriptFunc_grugRelevant func);
		void setGrugTick(scriptFunc_grugTick func);
		void setCollectRevenue(scriptFunc_fiscal func);
		void setCollectExpenditure(scriptFunc_fiscal func);
		void setOnHit(scriptFunc_onHit func);
		void setOnTargetChanged(scriptFunc_onTargetChanged func);
		void setOnTargetClicked(scriptFunc_onTargetClicked func);
		void setUpdateCustomAction(scriptFunc_updateCustomAction func);
		void setOnActionTrigger(scriptFunc_onActionTrigger func);
		void setOnTurnStart(scriptFunc_onTurnStart func);
		void setOnTurnEnd(scriptFunc_onTurnEnd func);
		void setOnBeginMove(scriptFunc_onBeginMove func);
		void setOnEndMove(scriptFunc_onEndMove func);

		bool hasCreateState();
		bool hasUpdate();
		bool hasDrawUI();
		bool hasUpdateAnimation();
		bool hasGrugRelevant();
		bool hasGrugTick();
		bool hasCollectRevenue();
		bool hasCollectExpenditure();
		bool hasOnHit();
		bool hasOnTargetChanged();
		bool hasOnTargetClicked();
		bool hasUpdateCustomAction();
		bool hasOnActionTrigger();
		bool hasOnTurnStart();
		bool hasOnTurnEnd();
		bool hasOnBeginMove();
		bool hasOnEndMove();

	private:
		scriptFunc_createState createState;
		scriptFunc_drawUI drawUI;
		scriptFunc_updateAnimation updateAnimation;
		scriptFunc_update update;
		scriptFunc_grugRelevant grugRelevant;
		scriptFunc_grugTick grugTick;
		scriptFunc_fiscal collectRevenue;
		scriptFunc_fiscal collectExpenditure;
		scriptFunc_onHit onHit;
		scriptFunc_onTargetChanged onTargetChanged;
		scriptFunc_onTargetClicked onTargetClicked;
		scriptFunc_updateCustomAction updateCustomAction;
		scriptFunc_onActionTrigger onActionTrigger;
		scriptFunc_onTurnStart onTurnStart;
		scriptFunc_onTurnEnd onTurnEnd;
		scriptFunc_onBeginMove onBeginMove;
		scriptFunc_onEndMove onEndMove;
	};

	struct EntitySpecification
	{
		static void initializeSpecifications(e2::GameContext* ctx);
		static void finalizeSpecifications(e2::Context* ctx);
		static void destroySpecifications();
		static e2::EntitySpecification* specificationById(e2::Name id);
		static e2::EntitySpecification* specification(size_t index);
		static size_t specificationCount();

		/** The global identifier for this entity, i.e. its identifiable name */
		e2::Name id;

		/** The badge id. Wil ldefault to id if empty */
		e2::Name badgeId;

		/** C++ type to instantiate when creating an entity of this specification */
		e2::Type* type{};

		/** Display name */
		std::string displayName;

		/** The layer index for this entity, only one entity of each layer can fit on a given tile index */
		EntityLayerIndex layerIndex = EntityLayerIndex::Unit;

		/** The move type for this entity */
		EntityMoveType moveType{ EntityMoveType::Static };

		/** The move speed */
		float moveSpeed{ 1.0f };

		/** The passable flags, i.e. what can this entity traverse. */
		e2::PassableFlags passableFlags{ e2::PassableFlags::None };

		/** Starting max. health */
		float maxHealth{100.0f};

		/** 
		 * Attack strength.
		 * If A attacks B, then A will deal max(0, x) where x is:
		 *	A.attackStrength - (B.attackStrength*defensiveModifier)
		 * and B will deal max(0, x) where x is:
		 *	(B.attackStrength * B.retaliatoryModifier) - (A.attackStrength * A.defensiveModifier)
		 */
		float attackStrength{ 0.0f };
		float defensiveModifier { 1.0f };
		float retaliatoryModifier{ 0.0f }; // if >0, this unit will retaliate when attacked, with attackStrength*retaliatoryModifier

		/** Sight range */
		int32_t sightRange{ 2 };

		bool revenueByAbundance = false;

		/** Revenue from this entity */
		ResourceTable revenue;

		/** Upkeep costs */
		ResourceTable upkeep;

		/** Build costs */
		ResourceTable cost;

		/** Number of turns it takes to build this entity (when built as a unit mostly, since structures get built instantly) */
		int32_t buildTurns{};

		/** Number of movement points */
		int32_t movePoints{};

		/** Number of attack points */
		int32_t attackPoints{};

		/** Number of build points */
		int32_t buildPoints{};

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

		/** Animation poses */
		std::unordered_map<e2::Name, EntityPoseSpecification> poses;

		/** Animation actions*/
		std::unordered_map<e2::Name, EntityActionSpecification> actions;

		EntityScriptInterface scriptInterface;
	};


	struct UnitBuildResult
	{
		std::string buildMessage;
		bool didSpawn{};
		glm::ivec2 spawnLocation;
	};

	struct UnitBuildAction
	{
		UnitBuildAction() = default;
		UnitBuildAction(e2::GameEntity* inOwner, e2::EntitySpecification const* spec);

		// Build action for building units 
		UnitBuildResult tick();

		// The owning entity
		e2::GameEntity* owner{};

		// The specification for the entity we are building 
		e2::EntitySpecification const* specification;

		int32_t buildTurnsLeft{};
		
		// how many was total built, useful for grug
		int32_t totalBuilt{};

		// the turn this was last built, useful for grug
		int32_t turnLastBuilt{};
	};

	struct EntityAnimationPose
	{
		e2::Name id;
		e2::AnimationPose* pose{};
		float blendTime{};
	};



	struct EntityAnimationActionTrigger
	{
		e2::Name id;
		double time{};

		bool triggered{};
	};

	struct EntityAnimationAction
	{
		e2::Name id;
		e2::AnimationPose* pose{};
		float blendInTime{};
		float blendOutTime{};
		float speed{1.0f};

		e2::StackVector<e2::EntityAnimationActionTrigger, e2::maxNumTriggersPerAction> triggers;
	};




	enum class GameEntityAliveState : uint8_t
	{
		Alive,
		Dying,
		Dead
	};


	constexpr uint32_t maxNumPosesPerEntity = 8;
	constexpr uint32_t maxNumActionsPerEntity = 8;

	constexpr uint64_t maxNumGameEntities = 32768;

	/** 
	 * A game entity, i.e. a unit, structure, etc.
	 * @tags(dynamic, arena, arenaSize=e2::maxNumGameEntities)
	 */
	class GameEntity : public e2::Object, public e2::GameContext, public e2::IFiscalStream
	{
		ObjectDeclaration();
	public:
		GameEntity();
		GameEntity(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::ivec2 const& tile, EmpireId empireId);
		virtual ~GameEntity();

		static bool scriptEqualityPtr(GameEntity* lhs, GameEntity* rhs);
		static GameEntity* scriptAssignPtr(GameEntity*& lhs, GameEntity* rhs);

		void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::ivec2 const& tile, EmpireId empireId);

		virtual void drawUI(e2::UIContext* ctx);

		virtual void writeForSave(e2::Buffer& toBuffer);
		virtual void readForSave(e2::Buffer& fromBuffer);

		/** returns true if this entity has actions available to them, or false if they can't do shit this round */
		virtual bool grugRelevant();

		/** control this entity with ai, returns true if it still wants control, returns false when it has nothing more to do this turn */
		virtual bool grugTick(double seconds);

		virtual void collectRevenue(ResourceTable& outRevenueTable) override;
		virtual void collectExpenditure(ResourceTable& outExpenditureTable) override;

		virtual void initialize();

		virtual void onHit(e2::GameEntity* instigator, float damage);

		virtual void onTargetChanged(glm::ivec2 const& location);
		virtual void onTargetClicked();

		virtual void updateCustomAction(double seconds);

		virtual void onTurnEnd();
		virtual void onTurnStart();

		virtual void updateAnimation(double seconds);

		/** WARNING: This is run on EVERY ENTITY EVERY FRAME. DONT put heavy code here!! */
		virtual void update(double seconds);

		virtual void onActionTrigger(e2::Name action, e2::Name trigger);

		virtual void onBeginMove();
		virtual void onEndMove();

		void turnTowards(glm::ivec2 hex);

		void spreadVisibility();
		void rollbackVisibility();

		glm::vec2 meshPlanarCoords();

		void setMeshTransform(glm::vec3 const& pos, float angle);

		bool isLocal();

		e2::EntitySpecification* specification{};
		
		/** Create a new build action specific to this instance */
		e2::UnitBuildAction createBuildAction(e2::Name unitId);

		/** Returns true if the given action can be built right now */
		bool isBuilding();
		bool canBuild(e2::UnitBuildAction& action);
		void build(e2::UnitBuildAction& action);
		void cancelBuild();

		/** Updated automatically by the game, if true this entity is in view. */
		bool inView{};

		glm::ivec2 tileIndex;
		EmpireId empireId;

		float health{};
		int32_t movePointsLeft{};
		int32_t attackPointsLeft{};
		int32_t buildPointsLeft{};

		e2::MeshProxy* meshProxy{};
		e2::SkinProxy* skinProxy{};

		glm::quat meshRotation{};
		glm::quat meshTargetRotation{};
		glm::vec3 meshPosition{};

		chaiscript::Boxed_Value scriptState;

		void setPose(e2::Name poseName);
		void playAction(e2::Name actionName);
		bool isActionPlaying(e2::Name actionName);
		bool isAnyActionPlaying();

		void setPose2(e2::Pose* pose, double lerpTime);
		void playAction2(e2::AnimationPose* anim, double blendIn = 0.2f, double blendOut = 0.2f, double speed = 1.0);

		virtual e2::Game* game() override;

		std::string buildMessage;
		e2::UnitBuildAction* currentlyBuilding{};

	protected:
		e2::Game* m_game{};

		void destroyProxy();

		//  -- internal animation states
		e2::Pose* m_mainPose{};

		double m_lerpTime{};
		e2::Moment m_lastChangePose;
		e2::Pose* m_currentPose{};
		e2::Pose* m_oldPose{};

		e2::AnimationPose* m_actionPose{};
		double m_actionBlendInTime = 0.2;
		double m_actionBlendOutTime = 0.2;
		double m_actionSpeed = 1.0;
		double m_lastActionTime = 0.0;
		e2::EntityAnimationAction* m_currentAction{};

		e2::StackVector<EntityAnimationPose, e2::maxNumPosesPerEntity> m_animationPoses;
		e2::StackVector<EntityAnimationAction, e2::maxNumActionsPerEntity> m_animationActions;


	};

	constexpr uint64_t entitySizeBytes = sizeof(e2::GameEntity);
}

#include "gameentity.generated.hpp"