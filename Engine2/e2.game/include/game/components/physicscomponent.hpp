#pragma once 

#include <e2/utils.hpp>
#include <e2/timer.hpp>
#include <e2/assets/mesh.hpp>
#include "game/shared.hpp"
#include <nlohmann/json.hpp>

namespace e2
{
	class GameContext;
	class Entity;

	struct CollisionSpecification
	{
		void populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps);
		void finalize(e2::GameContext* ctx);

		float radius{20.0f};

	};

	/** places an obstacle (collision) in the world, at entity location. @tags(arena, arenaSize=4096) */
	class CollisionComponent : public e2::Object
	{
		ObjectDeclaration();
	public:
		CollisionComponent(e2::CollisionSpecification* specification, e2::Entity* entity);
		virtual ~CollisionComponent();

		void invalidate();

		inline e2::Entity* entity()
		{
			return m_entity;
		}

		inline float radius()
		{
			return m_radius;
		}
		void setRadius(float newRadius);

	protected:

		e2::Entity* m_entity{};
		float m_radius{ 0.01f };
		e2::CollisionSpecification* m_specification{};
		glm::ivec2 m_currentIndex;
	};






	struct MovementSpecification
	{
		void populate(nlohmann::json& obj, std::unordered_set<e2::Name>& deps);
		void finalize(e2::GameContext* ctx);

		std::unordered_set<e2::Type const*> ignoredTypes;
	};

	/** movement solver. @tags(arena, arenaSize=4096) */
	class MovementComponent : public e2::Object
	{
		ObjectDeclaration();
	public:
		MovementComponent(e2::MovementSpecification* specification, e2::Entity* entity);
		virtual ~MovementComponent();

		// returns energy left
		float move(float radius, glm::vec2 const& dir, float energy, e2::CollisionType mask, std::vector<e2::Collision> const& collisions);
		void resolve(float radius, e2::CollisionType mask, std::vector<e2::Collision> const& collisions);

	protected:

		bool shouldIgnore(e2::Entity* entity);

		e2::Entity* m_entity{};
		e2::MovementSpecification* m_specification{};

		//e2::MovementSpecification* m_specification{};
	};


}


#include "physicscomponent.generated.hpp"