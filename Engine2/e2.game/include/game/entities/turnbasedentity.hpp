
#pragma once

#include <e2/utils.hpp>

#include "game/entity.hpp"
#include "game/resources.hpp"
#include "game/components/skeletalmeshcomponent.hpp"
#include "game/components/fogcomponent.hpp"

#include <chaiscript/chaiscript.hpp>

namespace e2
{


	class MeshProxy;
	class UIContext;

	class Mob;
	class TurnbasedEntity;
	using scriptFunc_drawUI = std::function<void(e2::TurnbasedEntity*, e2::UIContext*)>;
	using scriptFunc_update = std::function<void(e2::TurnbasedEntity*, double)>;
	using scriptFunc_updateAnimation = std::function<void(e2::TurnbasedEntity*, double)>;
	using scriptFunc_grugTick = std::function<bool(e2::TurnbasedEntity*, double)>;
	using scriptFunc_playerRelevant = std::function<bool(e2::TurnbasedEntity*)>;
	using scriptFunc_grugRelevant = std::function<bool(e2::TurnbasedEntity*)>;
	using scriptFunc_fiscal = std::function<void(e2::TurnbasedEntity*, e2::ResourceTable&)>;
	using scriptFunc_onHit = std::function<void(e2::TurnbasedEntity*, e2::TurnbasedEntity*, float)>;
	using scriptFunc_onTargetChanged = std::function<void(e2::TurnbasedEntity*, glm::ivec2 const&)>;
	using scriptFunc_onTargetClicked = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_updateCustomAction = std::function<void(e2::TurnbasedEntity*, double)>;
	using scriptFunc_onActionTrigger = std::function<void(e2::TurnbasedEntity*, std::string, std::string)>;
	using scriptFunc_onTurnStart = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onTurnEnd = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onBeginMove = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onEndMove = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_createState = std::function<chaiscript::Boxed_Value(e2::TurnbasedEntity*)>;

	using scriptFunc_onSelected = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onDeselected = std::function<void(e2::TurnbasedEntity*)>;

	using scriptFunc_onSpawned = std::function<void(e2::TurnbasedEntity*)>;

	using scriptFunc_onWaveUpdate = std::function<void(e2::TurnbasedEntity*, double)>;
	using scriptFunc_onWaveStart = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onWaveEnd = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onWavePreparing = std::function<void(e2::TurnbasedEntity*)>;
	using scriptFunc_onWaveEnding = std::function<void(e2::TurnbasedEntity*)>;

	using scriptFunc_onMobSpawned = std::function<void(e2::TurnbasedEntity*, e2::Mob*)>;
	using scriptFunc_onMobDestroyed = std::function<void(e2::TurnbasedEntity*, e2::Mob*)>;



	struct TurnbasedScriptInterface
	{
		TurnbasedScriptInterface() = default;

		chaiscript::Boxed_Value invokeCreateState(e2::TurnbasedEntity* entity);
		void invokeDrawUI(e2::TurnbasedEntity* entity, e2::UIContext* ui);

		void invokeUpdate(e2::TurnbasedEntity* entity, double seconds);

		void invokeUpdateAnimation(e2::TurnbasedEntity* entity, double seconds);
		bool invokePlayerRelevant(e2::TurnbasedEntity* entity);
		bool invokeGrugRelevant(e2::TurnbasedEntity* entity);
		bool invokeGrugTick(e2::TurnbasedEntity* entity, double seconds);
		void invokeCollectRevenue(e2::TurnbasedEntity* entity, e2::ResourceTable& outTable);
		void invokeCollectExpenditure(e2::TurnbasedEntity* entity, e2::ResourceTable& outTable);
		void invokeOnHit(e2::TurnbasedEntity* entity, e2::TurnbasedEntity* instigator, float dmg);
		void invokeOnTargetChanged(e2::TurnbasedEntity* entity, glm::ivec2 const& hex);
		void invokeOnTargetClicked(e2::TurnbasedEntity* entity);
		void invokeUpdateCustomAction(e2::TurnbasedEntity* entity, double seconds);
		void invokeOnActionTrigger(e2::TurnbasedEntity* entity, e2::Name action, e2::Name trigger);
		void invokeOnTurnStart(e2::TurnbasedEntity* entity);
		void invokeOnTurnEnd(e2::TurnbasedEntity* entity);
		void invokeOnBeginMove(e2::TurnbasedEntity* entity);
		void invokeOnEndMove(e2::TurnbasedEntity* entity);

		void invokeOnWaveUpdate(e2::TurnbasedEntity* entity, double seconds);

		void invokeOnWavePreparing(e2::TurnbasedEntity* entity);
		void invokeOnWaveStart(e2::TurnbasedEntity* entity);
		void invokeOnWaveEnding(e2::TurnbasedEntity* entity);
		void invokeOnWaveEnd(e2::TurnbasedEntity* entity);

		void invokeOnSpawned(e2::TurnbasedEntity* entity);
		void invokeOnMobSpawned(e2::TurnbasedEntity* entity, e2::Mob* mob);
		void invokeOnMobDestroyed(e2::TurnbasedEntity* entity, e2::Mob* mob);

		void invokeOnSelected(e2::TurnbasedEntity* entity);
		void invokeOnDeselected(e2::TurnbasedEntity* entity);

		void setCreateState(scriptFunc_createState func);
		void setDrawUI(scriptFunc_drawUI func);
		void setUpdate(scriptFunc_update func);
		void setUpdateAnimation(scriptFunc_updateAnimation func);
		void setPlayerRelevant(scriptFunc_playerRelevant func);
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
		void setOnWaveUpdate(scriptFunc_onWaveUpdate func);

		void setOnSpawned(scriptFunc_onSpawned func);

		void setOnWavePreparing(scriptFunc_onWavePreparing func);
		void setOnWaveStart(scriptFunc_onWaveStart func);
		void setOnWaveEnding(scriptFunc_onWaveEnding func);
		void setOnWaveEnd(scriptFunc_onWaveEnd func);



		void setOnMobSpawned(scriptFunc_onMobSpawned func);
		void setOnMobDestroyed(scriptFunc_onMobDestroyed func);

		void setOnSelected(scriptFunc_onSelected func);
		void setOnDeselected(scriptFunc_onDeselected func);

		bool hasCreateState();
		bool hasUpdate();
		bool hasDrawUI();
		bool hasUpdateAnimation();
		bool hasPlayerRelevant();
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

		bool hasOnWaveUpdate();
		bool hasOnWavePreparing();
		bool hasOnWaveStart();
		bool hasOnWaveEnding();
		bool hasOnWaveEnd();

		bool hasOnSpawned();


		bool hasOnMobSpawned();
		bool hasOnMobDestroyed();

		bool hasOnSelected();
		bool hasOnDeselected();

	private:
		scriptFunc_createState createState;
		scriptFunc_drawUI drawUI;
		scriptFunc_updateAnimation updateAnimation;
		scriptFunc_update update;
		scriptFunc_playerRelevant playerRelevant;
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

		scriptFunc_onSpawned onSpawned;

		scriptFunc_onWavePreparing onWavePreparing;
		scriptFunc_onWaveStart onWaveStart;
		scriptFunc_onWaveEnding onWaveEnding;
		scriptFunc_onWaveEnd onWaveEnd;
		scriptFunc_onWaveUpdate onWaveUpdate;

		scriptFunc_onMobSpawned onMobSpawned;
		scriptFunc_onMobDestroyed onMobDestroyed;

		scriptFunc_onSelected onSelected;
		scriptFunc_onDeselected onDeselected;
	};

	/** @tags(dynamic) */
	class TurnbasedSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		TurnbasedSpecification();
		virtual ~TurnbasedSpecification();
		virtual void populate(e2::GameContext *ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		/** The badge id. Wil ldefault to id if empty */
		e2::Name badgeId;

		/** true if this entity is relevant in waves (otherwise wont be rendered during waves, unless on structure layer. also wont get onWaveUpdate() called without this) */
		bool waveRelevant{ false };

		/** The layer index for this entity, only one entity of each layer can fit on a given tile index */
		EntityLayerIndex layerIndex = EntityLayerIndex::Unit;

		/** The move type for this entity */
		EntityMoveType moveType{ EntityMoveType::Static };

		/** The move speed */
		float moveSpeed{ 1.0f };

		/** The passable flags, i.e. what can this entity traverse. */
		e2::PassableFlags passableFlags{ e2::PassableFlags::None };

		/** Starting max. health */
		float maxHealth{ 100.0f };

		/**
		 * Attack strength.
		 * If A attacks B, then A will deal max(0, x) where x is:
		 *	A.attackStrength - (B.attackStrength*defensiveModifier)
		 * and B will deal max(0, x) where x is:
		 *	(B.attackStrength * B.retaliatoryModifier) - (A.attackStrength * A.defensiveModifier)
		 */
		float attackStrength{ 0.0f };
		float defensiveModifier{ 1.0f };
		float retaliatoryModifier{ 0.0f }; // if >0, this unit will retaliate when attacked, with attackStrength*retaliatoryModifier

		/** Sight range */
		int32_t sightRange{ 2 };

		int32_t attackRange{ 2 };

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

		/** Whether to show move points in unit UI */
		bool showMovePoints{};

		/** Number of attack points */
		int32_t attackPoints{};

		/** Whether to show attack points in unit UI */
		bool showAttackPoints{};

		/** Number of build points */
		int32_t buildPoints{};

		/** Whether to show attack points in unit UI */
		bool showBuildPoints{};

		TurnbasedScriptInterface scriptInterface;

		e2::SkeletalMeshSpecification mesh;
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
		UnitBuildAction(e2::TurnbasedEntity* inOwner, e2::TurnbasedSpecification const* spec);

		// Build action for building units 
		UnitBuildResult tick();

		// The owning entity
		e2::TurnbasedEntity* owner{};

		// The specification for the entity we are building 
		e2::TurnbasedSpecification const* specification;

		int32_t buildTurnsLeft{};

		// how many was total built, useful for grug
		int32_t totalBuilt{};

		// the turn this was last built, useful for grug
		int32_t turnLastBuilt{};
	};


	/**
	 * A turnbased entity, i.e. a unit, structure, etc.
	 * @tags(dynamic, arena, arenaSize=4096)
	 */
	class TurnbasedEntity : public e2::Entity, public e2::IFiscalStream
	{
		ObjectDeclaration();
	public:
		TurnbasedEntity();
		virtual ~TurnbasedEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition) override;
		void setupTurnbased(glm::ivec2 const& tile, EmpireId empire);

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;
		virtual void updateAnimation(double seconds) override;
		virtual void update(double seconds) override;


		static bool scriptEqualityPtr(TurnbasedEntity* lhs, TurnbasedEntity* rhs);
		static TurnbasedEntity* scriptAssignPtr(TurnbasedEntity*& lhs, TurnbasedEntity* rhs);

		virtual void drawUI(e2::UIContext* ctx);



		virtual bool playerRelevant();
		virtual bool grugRelevant();
		virtual bool grugTick(double seconds);
		virtual void collectRevenue(ResourceTable& outRevenueTable) override;
		virtual void collectExpenditure(ResourceTable& outExpenditureTable) override;
		virtual void onHit(e2::TurnbasedEntity* instigator, float damage);
		virtual void onTargetChanged(glm::ivec2 const& location);
		virtual void onTargetClicked();
		virtual void updateCustomAction(double seconds);
		virtual void onWavePreparing();
		virtual void onWaveStart();
		virtual void onWaveUpdate(double seconds);
		virtual void onWaveEnding();
		virtual void onWaveEnd();
		virtual void onMobSpawned(e2::Mob* mob);
		virtual void onMobDestroyed(e2::Mob* mob);
		virtual void onTurnEnd();
		virtual void onTurnStart();
		virtual void onActionTrigger(e2::Name action, e2::Name trigger);
		virtual void onBeginMove();
		virtual void onEndMove();
		virtual void onSpawned();

		void turnTowards(glm::ivec2 const& hex);
		void turnTowardsPlanar(glm::vec3 const& planar);


		void setMeshTransform(glm::vec3 const& pos, float angle);

		/** Create a new build action specific to this instance */
		e2::UnitBuildAction createBuildAction(e2::Name unitId);

		/** Returns true if the given action can be built right now */
		bool isBuilding();
		bool canBuild(e2::UnitBuildAction& action);
		void build(e2::UnitBuildAction& action);
		void cancelBuild();



		// these two are updated 
		void updateGrugVariables();
		bool grugCanMove{};
		bool grugCanAttack{};

		e2::Mob* closestMobWithinRange(float range);

		inline e2::EmpireId getEmpireId()
		{
			return m_empireId;
		}

		inline void setTileIndex(glm::ivec2 const& newIndex)
		{
			m_tileIndex = newIndex;
		}

		inline glm::ivec2 const& getTileIndex()
		{
			return m_tileIndex;
		}

		bool isLocal();

		inline float getHealth()
		{
			return m_health;
		}

		inline int32_t getMovePointsLeft()
		{
			return m_movePointsLeft;
		}

		inline void subtractMovePoints(int32_t count)
		{
			m_movePointsLeft -= count;
		}

		inline int32_t getAttackPointsLeft()
		{
			return m_attackPointsLeft;
		}

		inline void subtractAttackPoints(int32_t count)
		{
			m_attackPointsLeft -= count;
		}

		inline int32_t getBuildPointsLeft()
		{
			return m_buildPointsLeft;
		}

		inline void subtractBuildPoints(int32_t count)
		{
			m_buildPointsLeft -= count;
		}


		inline e2::SkeletalMeshComponent* meshComponent()
		{
			return m_mesh;
		}

		inline e2::TurnbasedSpecification* turnbasedSpecification()
		{
			return m_turnbasedSpecification;
		}

	protected:
		e2::Game* m_game{};
		e2::TurnbasedSpecification* m_turnbasedSpecification{};

		e2::SkeletalMeshComponent* m_mesh{};

		chaiscript::Boxed_Value m_scriptState;

		e2::EmpireId m_empireId{};
		glm::ivec2 m_tileIndex;

		float m_health{};
		int32_t m_movePointsLeft{};
		int32_t m_attackPointsLeft{};
		int32_t m_buildPointsLeft{};
		glm::quat m_targetRotation{};

		std::string m_buildMessage;
		e2::UnitBuildAction* m_currentlyBuilding{};

		bool m_sleeping{};

		e2::FogComponent m_fog;
	};


}


#include "turnbasedentity.generated.hpp"