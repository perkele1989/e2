
#pragma once 



#include <e2/application.hpp>
#include <e2/assets/sound.hpp>
#include <e2/managers/audiomanager.hpp>
#include "game/hex.hpp"
#include "game/gamecontext.hpp"
#include "game/resources.hpp"
#include "game/entity.hpp"
#include "game/mob.hpp"
#include "game/empire.hpp"
#include "game/shared.hpp"
#include "game/playerstate.hpp"
#include "game/script.hpp"
#include "game/radionmanager.hpp"

#include <chaiscript/chaiscript.hpp>

namespace e2
{
	class CollisionComponent;
	template<typename T>
	class ScriptRef 
	{
	public:

		ScriptRef() = default;

		ScriptRef(ScriptRef<T> const& other)
			: value(other.value)
		{
			
		}

		ScriptRef(T* v)
			: value(v)
		{

		}

		ScriptRef<T>& operator=(ScriptRef<T> const &other)
		{
			value = other.value;
			return *this;
		}


		static bool equals(ScriptRef<T> const& lhs, ScriptRef<T> const& rhs)
		{
			return lhs.value == rhs.value;
		}

		void clear()
		{
			value = nullptr;
		}

		bool isNull()
		{
			return value == nullptr;
		}

		T* value{};
	};


	/** @tags(arena, arenaSize=16384*128) */
	class PathFindingHex : public e2::Object
	{
		ObjectDeclaration();
	public:
		PathFindingHex(e2::Hex const& index);
		virtual ~PathFindingHex();

		e2::Hex index;

		e2::Entity* grugTarget{};
		e2::PathFindingHex* towardsOrigin{};

		uint32_t stepsFromOrigin{};

		bool isBegin{};
		bool hexHasTarget{};
		bool instantlyReachable{};
	};

	//uint32_t maxAutoMoveRange = 128; // 128*128 = 16384 pathfindinghex




	/** @tags(arena, arenaSize=4096) */
	class PathFindingAS : public e2::Object
	{
		ObjectDeclaration();
	public:
		PathFindingAS();
		PathFindingAS(e2::Game* game, e2::Hex const& start, uint64_t range, bool ignoreVisibility = false, e2::PassableFlags passableFlags = PassableFlags::Land, bool onlyWaveRelevant = false, e2::Hex *stopWhenFound = nullptr);
		PathFindingAS(e2::TurnbasedEntity* unit);
		// for long range targets
		//PathFindingAS(e2::GameEntity* unit, glm::ivec2 const& target);
		~PathFindingAS();

		std::vector<e2::Hex> find(e2::Hex const& target);

		// lowest health target directly attackable from origin hex , 
		bool grugCanMove{};
		e2::TurnbasedEntity* grugTarget{};
		e2::Hex grugTargetMoveHex;
		uint32_t grugTargetMovePoints{};

		e2::PathFindingHex* origin{};
		std::unordered_map<glm::ivec2, e2::PathFindingHex*> hexIndex;

		/** targets directly attackable from origin hex */
		std::unordered_set<e2::TurnbasedEntity*> targetsInRange;

		/** Maps attackable entities to the hex that is closest to move to in order to attack it */
		std::unordered_map<e2::TurnbasedEntity*, std::pair<e2::Hex, uint32_t>> targetsInMoveRange;

		
		
	};

	class Game;

	class Entity;
	class TurnbasedEntity;
	class PlayerEntity;



	class Game : public e2::Application, public e2::GameContext
	{

	protected:
		e2::AudioChannel m_menuMusic;
	public:


		SaveMeta saveSlots[e2::numSaveSlots];

		Game(e2::Context* ctx);
		virtual ~Game();

		void saveGame(uint8_t slot);
		e2::SaveMeta readSaveMeta(uint8_t slot);
		void readAllSaveMetas();
		void loadGame(uint8_t slot);

		// thsi function inits game specific stuff (as opposed to resources). Creates grid etc
		void setupGame();

		// this function nukes shit from setupgame 
		void nukeGame();

		void exitToMenu();

		void finalizeBoot();

		void initializeScriptEngine();
		void destroyScriptEngine();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;
		void updateGame(double seconds);
		void updateMenu(double seconds);
		void updateInGameMenu(double seconds);

		void pauseWorldStreaming();
		void resumeWorldStreaming();
		void forceStreamLocation(glm::vec2 const& planarCoords);
		void beginStartGame();
		bool findStartLocation(glm::ivec2 const& offset, glm::ivec2 const& rangeSize, glm::ivec2& outLocation, bool forAi);
		void startGame();

		void addScreenShake(float intensity);
	protected:
		float m_shakeIntensity{};
	public:


		glm::vec2 worldToPixels(glm::vec3 const& world);

		virtual ApplicationType type() override;

		virtual e2::Game* game() override;

		void updateCamera(double seconds);
		void updateMainCamera(double seconds);
		void updateAltCamera(double seconds);
		void updateAnimation(double seconds);

		void updateGameState();
		void updateTurn();
		void updateWavePreparing();
		void updateWave();
		void updateWaveEnding();
		//void updateAuto();

	protected:
		uint32_t m_waveDeforestIndex{};
		float m_waveDeforestTimer{};
	public:
		void updateRealtime();
		void updateTurnLocal();
		//void updateTurnAI();

		//void updateUnitAttack();


		void endTurn();
		void onTurnPreparingBegin();
		void onTurnPreparingEnd();
		void onStartOfTurn();
		void onEndOfTurn();
		void onTurnEndingBegin();
		void onTurnEndingEnd();

		void drawUI();
		void drawResourceIcons();
		void drawHitLabels();
		void drawStatusUI();
		void drawUnitUI();
		void drawMinimapUI();
		void drawDebugUI();
		void drawFinalUI();
		void drawCrosshair();

		void onNewCursorHex();

		void spawnHitLabel(glm::vec3 const& worldLocation, std::string const& text);

		e2::RenderView calculateRenderView(glm::vec2 const& viewOrigin);

		double timeDelta()
		{
			return m_timeDelta;
		}

		//void discoverEmpire(EmpireId empireId);


		inline uint64_t turn() const
		{
			return m_turn;
		}

		inline TurnState getTurnState() const
		{
			return m_turnState;
		}


	protected:

		std::vector<e2::Hex> m_proximityHexes;
		float m_waterProximity{};
		e2::AudioChannel m_waterChannel;

		e2::ALJTicket m_bootTicket;

		e2::Texture2DPtr m_irradianceMap;
		e2::Texture2DPtr m_radianceMap;

		e2::Moment m_bootBegin;
		GlobalState m_globalState{ GlobalState::Boot };
		MainMenuState m_mainMenuState{ MainMenuState::Main };

		InGameMenuState m_inGameMenuState{ InGameMenuState::Main };

		bool m_showGrid{ false };
		bool m_showPhysics{ false };

		friend class GameContext;

		e2::GameSession* m_session{};

		double m_timeDelta{};
		GameState m_state{ GameState::TurnPreparing };
		uint64_t m_turn{};

		// which empire has current turn
		//EmpireId m_empireTurn{};

		e2::ScriptGraph* m_currentGraph{};
		e2::ScriptExecutionContext* m_scriptExecutionContext{};

		TurnState m_turnState{ TurnState::Unlocked };
		TurnState m_moveTurnStateFallback{ TurnState::Unlocked };

		// main world grid
		e2::HexGrid* m_hexGrid{};

		glm::ivec2 m_startLocation;
		bool m_haveBegunStart{};
		bool m_haveStreamedStart{};
		e2::Moment m_beginStartTime;
		e2::Moment m_beginStreamTime;

		CursorMode m_cursorMode{ CursorMode::Select };
		glm::vec2 m_cursor; // mouse position in pixels, from topleft corner
		glm::vec2 m_cursorUnit; // mouse position scaled between 0.0 - 1.0
		glm::vec2 m_cursorNdc; // mouse position scaled between -1.0 and 1.0
		glm::vec2 m_cursorPlane; // mouse position as projected on to the world xz plane
		glm::ivec2 m_cursorHex; // mouse position as projected upon a hex
		glm::ivec2 m_prevCursorHex;
		e2::TileData* m_cursorTile{};
		bool m_hexChanged{};

		bool m_uiHovered{};
		bool m_viewDragging{};

		// empirees 

	public:

		inline glm::vec2 const& cursorPlane() const
		{
			return m_cursorPlane;
		}

		inline glm::ivec2 const& cursorHex() const
		{
			return m_cursorHex;
		}

		EmpireId spawnEmpire();
		void destroyEmpire(EmpireId empireId);

		//void spawnAIEmpire();

		e2::GameEmpire* localEmpire()
		{
			return m_localEmpire;
		}

		e2::EmpireId localEmpireId()
		{
			return m_localEmpireId;
		}

		e2::GameEmpire* nomadEmpire()
		{
			return m_nomadEmpire;
		}

		e2::EmpireId nomadEmpireId()
		{
			return m_nomadEmpireId;
		}


		e2::GameEmpire* empireById(EmpireId id);

		void resolveLocalEntity();
		void nextLocalEntity();

		e2::Name getCityName();

		inline bool isRealtime() 
		{
			return m_turnState == e2::TurnState::Realtime;
		}

	protected:
		e2::EmpireId m_localEmpireId{};
		e2::GameEmpire* m_localEmpire{};

		std::unordered_set<e2::TurnbasedEntity*> m_localTurnEntities;

		e2::EmpireId m_nomadEmpireId{};
		e2::GameEmpire* m_nomadEmpire{};
		e2::StackVector<e2::GameEmpire*, e2::maxNumEmpires> m_empires;

		std::vector<e2::Name> m_cityNames;

		std::unordered_map<e2::TurnbasedEntity*, e2::City*> m_entityToCity;

	public:

		void harvestWood(glm::ivec2 const& location, EmpireId empire);
		void removeWood(glm::ivec2 const& location);

		void removeResource(glm::ivec2 const& location);

		e2::RenderView const& view()
		{
			return m_view;
		}

		e2::Viewpoints2D viewPoints()
		{
			return m_viewPoints;
		}

		void applyDamage(e2::TurnbasedEntity* entity, e2::TurnbasedEntity* instigator, float damage);

		void resolveSelectedEntity();
		void unresolveSelectedEntity();

		// game units 
		e2::TurnbasedEntity* getSelectedEntity();
		void selectEntity(e2::TurnbasedEntity* entity);
		void deselectEntity();

		void moveSelectedEntityTo(glm::ivec2 const& to);
		void updateUnitMove();

		void beginCustomAction();
		void endCustomAction();
		void updateCustomAction();

		bool attemptBeginWave(e2::TurnbasedEntity* hiveEntity, std::string const& prefabPath);


	public:

		void beginTargeting();
		void endTargeting();
		void updateTarget();

		e2::Entity* spawnEntity(e2::Name entityId, glm::vec3 const& worldPosition, glm::quat const& worldRotation = glm::identity<glm::quat>());
		e2::Entity* spawnCustomEntity(e2::Name specificationId, e2::Name entityType, glm::vec3 const& worldPosition, glm::quat const& worldRotation = glm::identity<glm::quat>());
		void destroyEntity(e2::Entity* entity);
		void queueDestroyEntity(e2::Entity* entity);

		e2::TurnbasedEntity* spawnTurnbasedEntity(e2::Name entityId, glm::ivec2 const& tileIndex, EmpireId empireId);
		void queueDestroyTurnbasedEntity(e2::TurnbasedEntity* entity);
		void destroyTurnbasedEntity(e2::TurnbasedEntity* entity);

		e2::TurnbasedEntity* entityAtHex(e2::EntityLayerIndex layerIndex, glm::ivec2 const& hex);

		inline e2::PathFindingAS* selectedUnitAS()
		{
			return m_unitAS;
		}

		int32_t grugNumAttackMovePoints();
		e2::TurnbasedEntity* grugAttackTarget();
		glm::ivec2 grugAttackMoveLocation();
		glm::ivec2 grugMoveLocation();

		bool entityRelevantForPlay(e2::TurnbasedEntity* entity);

		e2::Wave* wave();


		virtual void onMobSpawned(e2::Mob* mob);
		virtual void onMobDestroyed(e2::Mob* mob);

	protected:

		e2::Wave* m_wave{};

		e2::PathFindingAS* m_unitAS{};
		std::vector<e2::Hex> m_unitHoverPath;
		std::vector<e2::Hex> m_unitMovePath;
		uint32_t m_unitMoveIndex{};
		float m_unitMoveDelta{};
		bool m_ffwMove = false;

		std::unordered_set<e2::Entity*> m_entities;//all entities
		std::unordered_set<e2::Entity*> m_entitiesInView; // all entities in view
		std::unordered_set<e2::TurnbasedEntity*> m_turnbasedEntitiesInView;

		std::unordered_set<e2::Entity*> m_realtimeEntities;
		std::unordered_set<e2::Entity*> m_entitiesPendingDestroy; // entities needing destroy-o

		std::unordered_set<e2::TurnbasedEntity*> m_turnbasedEntities; // subset of entities, all the turnbased ones 
		e2::TurnbasedEntity* m_selectedEntity{}; // selected turnbased entity
		std::unordered_set<e2::TurnbasedEntity*> m_waveEntities; // wave-relevant turnbased entities
		std::array<EntityLayer, size_t(EntityLayerIndex::Count)> m_entityLayers; // turnbased entity layers
		std::unordered_set<e2::TurnbasedEntity*> m_turnbasedEntitiesPendingDestroy; // turnbased entities needing destroy-o
		std::unordered_set<e2::TurnbasedEntity*> m_dyingEntities; // turnbased entities currently dying

		//e2::PlayerEntity* m_playerEntity{}; // current player entity
		e2::PlayerState m_playerState;

	public:
		inline e2::PlayerState& playerState()
		{
			return m_playerState;
		}

		inline e2::PlayerEntity* playerEntity()
		{
			return m_playerState.entity;
		}
	protected:

		// anim stuff 
		double m_accumulatedAnimationTime{};

		// camera stuff 
		e2::RenderView m_view;

		// stuff to navigate camera main view by dragging
		glm::vec2 m_cursorDragOrigin;
		glm::vec2 m_viewDragOrigin;
		e2::RenderView m_dragView;

		e2::Viewpoints2D m_viewPoints;
		glm::vec2 m_startViewOrigin{};
		glm::vec2 m_viewOrigin{ 0.0f, 0.0f };
		glm::vec2 m_targetViewOrigin{ 0.0f, 0.0f };
		float m_targetViewZoom{ 0.0f };

	public:
		inline void setZoom(float newZoom)
		{
			m_targetViewZoom = newZoom;
		}


	protected:

		float m_viewZoom{0.0f};
		glm::vec2 m_viewVelocity{};
		
		// alt view lets you fly around freely, without affecting world streaming
		bool m_altView = false;
		glm::vec3 m_altViewOrigin{ 0.0f, 0.0f, 0.0f };
		float m_altViewYaw = 0.0f;
		float m_altViewPitch = 0.0f;
		// end camera stuff

	public:
	protected:
		e2::StackVector<HitLabel, e2::maxNumHitLabels> m_hitLabels;
		uint32_t m_hitLabelIndex{};

	public:
		inline chaiscript::ChaiScript* scriptEngine()
		{
			return m_scriptEngine;
		}

	protected:
		chaiscript::ChaiScript* m_scriptEngine{};
		chaiscript::ModulePtr m_scriptModule;

		float m_sunStrength{ 6.0f };
		float m_iblStrength{ 3.00f };
		float m_sunAngleA{ 36.0f };
		float m_sunAngleB{ 41.25f };
		float m_exposure{ 1.0f };
		float m_whitepoint{5.0f};
		float m_fog{0.0f};

	public:
		e2::Sprite* getUiSprite(e2::Name name);
		e2::Sprite* getIconSprite(e2::Name name);
		e2::Sprite* getUnitSprite(e2::Name name);
	protected:
		e2::SpritesheetPtr m_uiIconsSheet;
		e2::SpritesheetPtr m_uiIconsSheet2;
		e2::SpritesheetPtr m_uiUnitsSheet;


	protected:

		void initializeSpecifications(e2::ALJDescription &alj);
		void finalizeSpecifications();

		std::unordered_map<e2::Name, e2::EntitySpecification*> m_entitySpecifications;

		std::unordered_map<e2::Name, e2::ItemSpecification*> m_itemSpecifications;

		void initializeScriptGraphs();


		std::unordered_map<e2::Name, e2::ScriptGraph*> m_scriptGraphs;

	public:

		bool scriptRunning();

		void runScriptGraph(e2::Name id);

		e2::ItemSpecification* getItemSpecification(e2::Name name);
		e2::EntitySpecification* getEntitySpecification(e2::Name name);

		void registerCollisionComponent(e2::CollisionComponent* component, glm::ivec2 const& index);
		void unregisterCollisionComponent(e2::CollisionComponent* component, glm::ivec2 const& index);

		void populateCollisions(glm::ivec2 const& coordinate, CollisionType mask, std::vector<e2::Collision>& outCollisions, bool includeNeighbours = true);

	protected:
		std::unordered_map<glm::ivec2, std::unordered_set<e2::CollisionComponent*>> m_collisionComponents;


		e2::RadionManager m_radionManager;

	public:
		e2::RadionManager* radionManager();


	};
	

}



#include "game.generated.hpp"