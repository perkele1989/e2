
#pragma once 

#include <e2/utils.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/ui/uicontext.hpp>
#include <e2/transform.hpp>
#include <e2/renderer/meshproxy.hpp>

#include <game/gamecontext.hpp>
#include "game/shared.hpp"
#include "game/components/staticmeshcomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/resources.hpp"

#include <nlohmann/json.hpp>

#include <string>



namespace e2
{
	class ItemSpecification;

	class PlayerEntity;

	class WieldHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		WieldHandler() = default;
		virtual ~WieldHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>&deps) = 0;
		virtual void finalize(e2::GameContext* ctx) = 0;

		virtual void setActive(e2::PlayerEntity* player) = 0;
		virtual void setInactive(e2::PlayerEntity* player) = 0;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) = 0;
		virtual void onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName) = 0;
		
		e2::ItemSpecification* item{};
	}; 

	/** @tags(dynamic) */
	class HatchetHandler : public e2::WieldHandler
	{
		ObjectDeclaration();
	public:

		HatchetHandler() = default;
		virtual ~HatchetHandler(); 

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps) override;
		virtual void finalize(e2::GameContext* ctx) override;

		virtual void setActive(e2::PlayerEntity* player) override;
		virtual void setInactive(e2::PlayerEntity* player) override;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) override;
		virtual void onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName) override;

	protected:
		
		
		bool m_actionBusy{};

		e2::StaticMeshSpecification m_meshSpecification;
		e2::RandomAudioSpecification m_axeSwingSpecification;
		e2::RandomAudioSpecification m_woodChopSpecification;

		e2::StaticMeshComponent* m_mesh{};
		e2::RandomAudioComponent* m_axeSwing{};
		e2::RandomAudioComponent* m_woodChop{};
	};



	/** @tags(dynamic) */
	class SwordHandler : public e2::WieldHandler
	{
		ObjectDeclaration();
	public:

		SwordHandler() = default;
		virtual ~SwordHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps) override;
		virtual void finalize(e2::GameContext* ctx) override;

		virtual void setActive(e2::PlayerEntity* player) override;
		virtual void setInactive(e2::PlayerEntity* player) override;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) override;
		virtual void onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName) override;

	protected:


		bool m_actionBusy{};

		e2::StaticMeshSpecification m_meshSpecification;
		e2::RandomAudioSpecification m_swordSwingSpecification;

		e2::StaticMeshComponent* m_mesh{};
		e2::RandomAudioComponent* m_swordSwing{};
	};


	class UseHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		UseHandler()=default;

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) = 0;
		virtual void finalize(e2::GameContext* ctx) = 0;

		virtual void onUse(e2::PlayerEntity* player) = 0;

		e2::ItemSpecification* item{};
	protected:
		
	};

	class EquipHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		EquipHandler()=default;
		virtual ~EquipHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) = 0;
		virtual void finalize(e2::GameContext* ctx) = 0;


		e2::ItemSpecification* item{};
	protected:
		
	};

	/** @tags(dynamic) */
	class ShieldHandler : public e2::EquipHandler
	{
		ObjectDeclaration();
	public:
		ShieldHandler() = default;
		virtual ~ShieldHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize(e2::GameContext* ctx) override;

		float knockback{ 0.0f };
		float stun{ 0.0f };
	};



	class EntitySpecification : public e2::Object
	{
		ObjectDeclaration();
	public:
		EntitySpecification();
		virtual ~EntitySpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj);
		virtual void finalize() = 0;

		e2::Game* game{};

		/** The global identifier for this entity, i.e. its identifiable name */
		e2::Name id;

		/** C++ type to instantiate when creating an entity of this specification */
		e2::Type* entityType{};

		/** Display name */
		std::string displayName{"Unnamed"};

		/** Asset dependencies, these are populated by virtual populate()... function, and read by system to load deps*/
		std::unordered_set<e2::Name> assets;
	};

	class Entity : public e2::Object, public e2::GameContext
	{
		ObjectDeclaration();
	public:
		Entity();
		virtual ~Entity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition);

		virtual void writeForSave(e2::IStream& toBuffer);
		virtual void readForSave(e2::IStream& fromBuffer);

		virtual void updateVisibility() {};

		virtual void updateAnimation(double seconds) {};

		virtual void update(double seconds) {};

		virtual void onHitEntity(e2::Entity* otherEntity) {};

		virtual void onMeleeDamage(e2::Entity* instigator, float dmg) {};

		virtual bool canBeDestroyed() {
			return true;
		};

		virtual e2::Game* game() override;

		void playSound(e2::Name assetName, float volume, float spatiality);

		glm::vec2 planarCoords();
		e2::Hex hex();

		inline bool isInView() const
		{
			return m_inView;
		}

		inline void setInView(bool v)
		{
			m_inView = v;
			updateVisibility();
		}

		inline e2::Transform* getTransform()
		{
			return m_transform;
		}

		template<typename T>
		T* getSpecificationAs()
		{
			return dynamic_cast<T*>(m_specification);
		}

		e2::EntitySpecification* getSpecification()
		{
			return m_specification;
		}

		bool pendingDestroy{};

	protected:
		e2::Game* m_game{};
		e2::EntitySpecification* m_specification{};
		bool m_inView{}; // update automatically by game

		e2::Transform* m_transform{};
	};

	enum class EquipSlot : uint8_t
	{
		Shield = 0,
		Helmet,
		Boots
	};

	class StaticMeshComponent;

	class ItemSpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		ItemSpecification();
		virtual ~ItemSpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		bool stackable{};
		uint32_t stackSize{ 1 };

		bool wieldable{};
		e2::Type* wieldHandlerType{};
		e2::WieldHandler* wieldHandler{}; // owning instance

		bool usable{};
		e2::Type* useHandlerType{};
		e2::UseHandler* useHandler{}; // owning instance

		bool equippable{};
		e2::EquipSlot equipSlot{ e2::EquipSlot::Shield };
		e2::Type* equipHandlerType{};
		e2::EquipHandler* equipHandler{};
		

		e2::Name iconSprite;

		StaticMeshSpecification equipMesh;
		StaticMeshSpecification dropMesh;
		CollisionSpecification collision;
		MovementSpecification movement;

	};


	/** @tags(dynamic) */
	class ItemEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		ItemEntity();
		virtual ~ItemEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition) override;

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void updateAnimation(double seconds) override;

		virtual void update(double seconds) override;

		virtual void updateVisibility() override;

		inline double getLifetime() {
			return m_time;
		}

		inline void setLifetime(double newTime)
		{
			m_time = newTime;
		}

	protected:
		e2::ItemSpecification* m_itemSpecification{};
		e2::StaticMeshComponent* m_mesh{};
		e2::CollisionComponent* m_collision{};
		e2::MovementComponent* m_movement{};

		double m_time{};
		float m_rotation{};

		std::vector<e2::Collision> m_collisionCache;
	};




}

#include "entity.generated.hpp"