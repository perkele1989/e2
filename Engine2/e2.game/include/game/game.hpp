
#pragma once 



#include <e2/application.hpp>
#include <e2/assets/sound.hpp>
#include "game/hex.hpp"
#include "game/gamecontext.hpp"
#include "game/resources.hpp"
#include "game/gameentity.hpp"
#include "game/empire.hpp"
#include "game/shared.hpp"

#include <chaiscript/chaiscript.hpp>

namespace e2
{


	/** @tags(arena, arenaSize=16384) */
	class PathFindingHex : public e2::Object
	{
		ObjectDeclaration();
	public:
		PathFindingHex(e2::Hex const& index);
		virtual ~PathFindingHex();

		bool isBegin{};
		e2::Hex index;

		e2::PathFindingHex* towardsOrigin{};
		uint32_t stepsFromOrigin{};
	};

	/** @tags(arena, arenaSize=4096) */
	class PathFindingAS : public e2::Object
	{
		ObjectDeclaration();
	public:
		PathFindingAS();
		PathFindingAS(e2::GameContext* ctx, e2::Hex const& start, uint64_t range, bool ignoreVisibility = false, e2::PassableFlags passableFlags = PassableFlags::Land);
		PathFindingAS(e2::GameEntity* unit);
		~PathFindingAS();

		std::vector<e2::Hex> find(e2::Hex const& target);

		e2::PathFindingHex* origin{};
		std::unordered_map<glm::ivec2, e2::PathFindingHex*> hexIndex;
		
	};


	class Game : public e2::Application, public e2::GameContext
	{
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
		bool findStartLocation(glm::ivec2 const& offset, glm::ivec2 const& rangeSize, glm::ivec2 &outLocation, bool forAi);
		void startGame();



		glm::vec2 worldToPixels(glm::vec3 const& world);

		virtual ApplicationType type() override;

		virtual e2::Game* game() override;

		void updateCamera(double seconds);
		void updateMainCamera(double seconds);
		void updateAltCamera(double seconds);
		void updateAnimation(double seconds);

		void updateGameState();
		void updateTurn();

		void updateTurnLocal();
		void updateTurnAI();

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

		void onNewCursorHex();

		void spawnHitLabel(glm::vec3 const& worldLocation, std::string const& text);

		e2::RenderView calculateRenderView(glm::vec2 const& viewOrigin);

		double timeDelta()
		{
			return m_timeDelta;
		}

		void discoverEmpire(EmpireId empireId);


		inline uint64_t turn() const
		{
			return m_turn;
		}

	protected:

		e2::ALJTicket m_bootTicket;

		e2::Texture2DPtr m_irradianceMap;
		e2::Texture2DPtr m_radianceMap;

		e2::Moment m_bootBegin;
		GlobalState m_globalState{ GlobalState::Boot };
		MainMenuState m_mainMenuState{ MainMenuState::Main };

		InGameMenuState m_inGameMenuState{ InGameMenuState::Main };


		friend class GameContext;

		e2::GameSession* m_session{};

		double m_timeDelta{};

		GameState m_state{ GameState::TurnPreparing };
		uint64_t m_turn{};

		// which empire has current turn
		EmpireId m_empireTurn{};

		TurnState m_turnState{ TurnState::Unlocked };

		// main world grid
		e2::HexGrid* m_hexGrid{};

		glm::ivec2 m_startLocation;
		bool m_haveBegunStart{};
		bool m_haveStreamedStart{};
		e2::Moment m_beginStartTime;
		e2::Moment m_beginStreamTime;

		// game economy
		//GameResources m_resources;
		e2::Texture2DPtr m_uiTextureResources;

		CursorMode m_cursorMode{ CursorMode::Select };
		glm::vec2 m_cursor; // mouse position in pixels, from topleft corner
		glm::vec2 m_cursorUnit; // mouse position scaled between 0.0 - 1.0
		glm::vec2 m_cursorNdc; // mouse position scaled between -1.0 and 1.0
		glm::vec2 m_cursorPlane; // mouse position as projected on to the world xz plane
		e2::Hex m_cursorHex; // mouse position as projected upon a hex
		e2::Hex m_prevCursorHex;
		e2::TileData* m_cursorTile{};
		bool m_hexChanged{};

		bool m_uiHovered{};
		bool m_viewDragging{};

		// empirees 

	public:
		EmpireId spawnEmpire();
		void destroyEmpire(EmpireId empireId);

		void spawnAIEmpire();

		e2::GameEmpire* localEmpire()
		{
			return m_localEmpire;
		}

		e2::GameEmpire* nomadEmpire()
		{
			return m_nomadEmpire;
		}
		

		e2::GameEmpire* empireById(EmpireId id);

	protected:
		e2::EmpireId m_localEmpireId{};
		e2::GameEmpire* m_localEmpire{};

		e2::EmpireId m_nomadEmpireId{};
		e2::GameEmpire* m_nomadEmpire{};
		e2::StackVector<e2::GameEmpire*, e2::maxNumEmpires> m_empires;
		std::unordered_set<e2::GameEmpire*> m_aiEmpires;

		std::unordered_set<e2::GameEmpire*> m_discoveredEmpires;
		std::unordered_set<e2::GameEmpire*> m_undiscoveredEmpires;

	public:

		void harvestWood(glm::ivec2 const& location, EmpireId empire);
		void removeWood(glm::ivec2 const& location);

		e2::RenderView const& view()
		{
			return m_view;
		}

		e2::Viewpoints2D viewPoints()
		{
			return m_viewPoints;
		}

		void applyDamage(e2::GameEntity* entity, e2::GameEntity* instigator, float damage);

		void resolveSelectedEntity();
		void unresolveSelectedEntity();

		// game units 
		void selectEntity(e2::GameEntity* entity);
		void deselectEntity();
		
		void moveSelectedEntityTo(e2::Hex const& to);
		void updateUnitMove();

		void beginCustomAction();
		void endCustomAction();
		void updateCustomAction();

		void beginEntityTargeting();
		void endEntityTargeting();
		void updateEntityTarget();
		
		e2::GameEntity* spawnEntity(e2::Name entityId, e2::Hex const& location, EmpireId empire);
		void destroyEntity(e2::GameEntity* entity);
		void queueDestroyEntity(e2::GameEntity* entity);

		e2::GameEntity* entityAtHex(e2::EntityLayerIndex layerIndex, glm::ivec2 const& hex);

	protected:

		e2::PathFindingAS *m_unitAS;
		std::vector<e2::Hex> m_unitHoverPath;
		std::vector<e2::Hex> m_unitMovePath;
		uint32_t m_unitMoveIndex{};
		float m_unitMoveDelta{};

		GameEntity* m_selectedEntity{};
		std::unordered_set<GameEntity*> m_entities;
		std::array<EntityLayer, size_t(EntityLayerIndex::Count)> m_entityLayers;
		std::unordered_set<GameEntity*> m_entitiesPendingDestroy;



	protected:

		// anim stuff 
		double m_accumulatedAnimationTime{};

		e2::SoundPtr m_testSound;

		// camera stuff 
		e2::RenderView m_view;

		// stuff to navigate camera main view by dragging
		glm::vec2 m_cursorDragOrigin;
		glm::vec2 m_viewDragOrigin;
		e2::RenderView m_dragView;

		e2::Viewpoints2D m_viewPoints;
		glm::vec2 m_startViewOrigin{};
		glm::vec2 m_viewOrigin{ 0.0f, 0.0f };
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

	};


}


#include "game.generated.hpp"