
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

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation);

		virtual void writeForSave(e2::IStream& toBuffer);
		virtual void readForSave(e2::IStream& fromBuffer);

		virtual void updateVisibility() {};

		virtual void updateAnimation(double seconds) {};

		virtual void update(double seconds) {};

		virtual bool interactable() { return false; }

		virtual void onInteract(e2::Entity* interactor) {}

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

		uint64_t uniqueId{};
		bool transient{};

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




}

#include "entity.generated.hpp"