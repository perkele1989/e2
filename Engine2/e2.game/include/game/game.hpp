
#pragma once 

#include <e2/application.hpp>
#include <e2/hex/hex.hpp>
#include "game/gamecontext.hpp"
#include "game/resources.hpp"

namespace e2
{

	class GameUnit;

	class Game : public e2::Application, public e2::GameContext
	{
	public:

		Game(e2::Context* ctx);
		virtual ~Game();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;


		virtual ApplicationType type() override;

		virtual e2::Game* game() override;

		void drawStatusUI();
		void drawDebugUI();

		e2::RenderView calculateRenderView(glm::vec2 const& viewOrigin);

		void onTurnStart();
		void onTurnEnd();

		template<typename UnitType>
		UnitType* spawnUnit(e2::Hex const& location)
		{
			glm::ivec2 coords = location.offsetCoords();
			if (m_unitIndex.find(coords) != m_unitIndex.end())
				return nullptr;


			UnitType* newUnit = e2::create<UnitType>(this, coords);
			newUnit->spreadVisibility();

			m_units.insert(newUnit);
			m_unitIndex[coords] = newUnit;
			return newUnit;
		}

		void destroyUnit(e2::Hex const& location);

		e2::MeshPtr cursorMesh()
		{
			return m_cursorMesh;
		}


	protected:
		friend class GameContext;

		e2::GameSession* m_session{};

		// main world grid
		e2::HexGrid* m_hexGrid{};

		// better make it uint64, who knows how autistic my player-base will be
		uint64_t m_turn{};

		// game economy
		void updateResources();
		GameResources m_resources;
		e2::Texture2DPtr m_uiTextureResources;

		
		glm::vec2 m_cursor; // mouse position in pixels, from topleft corner
		glm::vec2 m_cursorUnit; // mouse position scaled between 0.0 - 1.0
		glm::vec2 m_cursorNdc; // mouse position scaled between -1.0 and 1.0
		glm::vec2 m_cursorPlane; // mouse position as projected on to the world xz plane
		e2::Hex m_cursorHex; // mouse position as projected upon a hex
		e2::MeshPtr m_cursorMesh;
		e2::MeshProxy* m_cursorProxy{};

		// game units 

		std::unordered_set<GameUnit*> m_units;
		std::unordered_map<glm::ivec2, GameUnit*> m_unitIndex;

		// camera stuff 
		void updateMainCamera(double seconds);
		e2::RenderView m_view;

		// stuff to navigate camera main view by dragging
		glm::vec2 m_cursorDragOrigin;
		glm::vec2 m_viewDragOrigin;
		e2::RenderView m_dragView;

		e2::Viewpoints2D m_viewPoints;
		glm::vec2 m_viewOrigin{ 0.0f, 0.0f };
		float m_viewZoom{0.0f};
		glm::vec2 m_viewVelocity{};
		void updateAltCamera(double seconds);
		// alt view lets you fly around freely, without affecting world streaming
		bool m_altView = false;
		glm::vec3 m_altViewOrigin{ 0.0f, 0.0f, 0.0f };
		float m_altViewYaw = 0.0f;
		float m_altViewPitch = 0.0f;
		// end camera stuff
	};
}

#include "game.generated.hpp"