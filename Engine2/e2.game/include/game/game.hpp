
#pragma once 

#include <e2/application.hpp>
#include "game/hex.hpp"
#include "game/gamecontext.hpp"
#include "game/resources.hpp"
#include "game/gameunit.hpp"
#include "game/empire.hpp"
#include "game/shared.hpp"

namespace e2
{

	enum class GlobalState : uint8_t
	{
		Menu,
		Game
	};

	enum class CursorMode : uint8_t
	{
		Select,
		UnitMove,
		UnitAttack
	};

	enum class GameState : uint8_t
	{
		TurnPreparing,
		Turn,
		TurnEnding,
	};

	enum class TurnState : uint8_t
	{
		Unlocked,
		UnitAction_Move,
		UnitAction_Attack,

	};

	class GameUnit;
	class GameStructure;

	/** @tags(arena, arenaSize=4096) */
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
	class PathFindingAccelerationStructure : public e2::Object
	{
		ObjectDeclaration();
	public:
		PathFindingAccelerationStructure();
		PathFindingAccelerationStructure(e2::GameContext* ctx, e2::Hex const& start, uint64_t range, bool ignoreVisibility = false);
		PathFindingAccelerationStructure(e2::GameUnit* unit);
		~PathFindingAccelerationStructure();

		std::vector<e2::Hex> find(e2::Hex const& target);

		e2::PathFindingHex* origin{};
		std::unordered_map<glm::ivec2, e2::PathFindingHex*> hexIndex;
		
	};

	class Game : public e2::Application, public e2::GameContext
	{
	public:

		Game(e2::Context* ctx);
		virtual ~Game();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;
		void updateGame(double seconds);
		void updateMenu(double seconds);


		void startGame();



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

		void updateUnitAttack();
		void updateUnitMove();

		void endTurn();
		void onTurnPreparingBegin();
		void onTurnPreparingEnd();
		void onStartOfTurn();
		void onEndOfTurn();
		void onTurnEndingBegin();
		void onTurnEndingEnd();

		void drawUI();
		void drawResourceIcons();
		void drawStatusUI();
		void drawUnitUI();
		void drawMinimapUI();
		void drawDebugUI();
		void drawFinalUI();

		void onNewCursorHex();

		e2::RenderView calculateRenderView(glm::vec2 const& viewOrigin);



		e2::MeshPtr cursorMesh()
		{
			return m_cursorMesh;
		}



		GameUnit* unitAtHex(glm::ivec2 const& hex);
		GameStructure* structureAtHex(glm::ivec2 const& hex);

		e2::MeshPtr getUnitMesh(e2::GameUnitType type);
		e2::MeshPtr getStructureMesh(e2::GameStructureType type);




	protected:

		// shared resources
		e2::StackVector<e2::MeshPtr, (uint64_t)e2::GameUnitType::Count> m_unitMeshes;
		e2::StackVector<e2::MeshPtr, (uint64_t)e2::GameStructureType::Count> m_structureMeshes;


		e2::Texture2DPtr m_irradianceMap;
		e2::Texture2DPtr m_radianceMap;


		GlobalState m_globalState{ GlobalState::Menu };


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


		// game economy
		void updateResources();
		GameResources m_resources;
		e2::Texture2DPtr m_uiTextureResources;

		CursorMode m_cursorMode{ CursorMode::Select };
		glm::vec2 m_cursor; // mouse position in pixels, from topleft corner
		glm::vec2 m_cursorUnit; // mouse position scaled between 0.0 - 1.0
		glm::vec2 m_cursorNdc; // mouse position scaled between -1.0 and 1.0
		glm::vec2 m_cursorPlane; // mouse position as projected on to the world xz plane
		e2::Hex m_cursorHex; // mouse position as projected upon a hex
		e2::Hex m_prevCursorHex;
		e2::MeshPtr m_cursorMesh;
		e2::MeshProxy* m_cursorProxy{};

		bool m_uiHovered{};
		bool m_viewDragging{};

		// empirees 

	public:
		EmpireId spawnEmpire();
		void destroyEmpire(EmpireId empireId);

		e2::GameEmpire* localEmpire()
		{
			return m_localEmpire;
		}

	protected:
		e2::EmpireId m_localEmpireId{};
		e2::GameEmpire* m_localEmpire{};
		e2::StackVector<e2::GameEmpire*, e2::maxNumEmpires> m_empires;

	public:

		void deselect();

		// game units 
		void selectUnit(e2::GameUnit* unit);
		void deselectUnit();
		void moveSelectedUnitTo(e2::Hex const& to);

		template<typename UnitType>
		UnitType* spawnUnit(e2::Hex const& location, EmpireId empire)
		{
			glm::ivec2 coords = location.offsetCoords();
			if (m_unitIndex.find(coords) != m_unitIndex.end())
				return nullptr;


			UnitType* newUnit = e2::create<UnitType>(this, coords, empire);
			newUnit->spreadVisibility();

			m_units.insert(newUnit);
			m_unitIndex[coords] = newUnit;

			if (m_empires[empire])
				m_empires[empire]->units.insert(newUnit);

			return newUnit;
		}

		void destroyUnit(e2::Hex const& location);

	protected:
		// move this to empirecontroller (GamePlayer and GameAI)
		e2::PathFindingAccelerationStructure *m_unitAS;
		std::vector<e2::Hex> m_unitHoverPath;
		std::vector<e2::Hex> m_unitMovePath;
		uint32_t m_unitMoveIndex{};
		float m_unitMoveDelta{};


		GameUnit* m_selectedUnit{};
		std::unordered_set<GameUnit*> m_units;
		std::unordered_map<glm::ivec2, GameUnit*> m_unitIndex;


	public:
		void selectStructure(e2::GameStructure* structure);
		void deselectStructure();

		template<typename UnitType>
		UnitType* spawnStructure(e2::Hex const& location, EmpireId empire)
		{
			glm::ivec2 coords = location.offsetCoords();
			if (m_structureIndex.find(coords) != m_structureIndex.end())
				return nullptr;


			UnitType* newStructure = e2::create<UnitType>(this, coords, empire);
			newStructure->spreadVisibility();

			m_structures.insert(newStructure);
			m_structureIndex[coords] = newStructure;

			if (m_empires[empire])
				m_empires[empire]->structures.insert(newStructure);

			return newStructure;
		}

		void destroyStructure(e2::Hex const& location);

	protected:
		e2::GameStructure* m_selectedStructure{};
		std::unordered_set<e2::GameStructure*> m_structures;
		std::unordered_map<glm::ivec2, e2::GameStructure*> m_structureIndex;





		// camera stuff 
		e2::RenderView m_view;

		// stuff to navigate camera main view by dragging
		glm::vec2 m_cursorDragOrigin;
		glm::vec2 m_viewDragOrigin;
		e2::RenderView m_dragView;

		e2::Viewpoints2D m_viewPoints;
		glm::vec2 m_viewOrigin{ 0.0f, 0.0f };
		float m_viewZoom{0.0f};
		glm::vec2 m_viewVelocity{};
		
		// alt view lets you fly around freely, without affecting world streaming
		bool m_altView = false;
		glm::vec3 m_altViewOrigin{ 0.0f, 0.0f, 0.0f };
		float m_altViewYaw = 0.0f;
		float m_altViewPitch = 0.0f;
		// end camera stuff
private:

	};
}

#include "game.generated.hpp"