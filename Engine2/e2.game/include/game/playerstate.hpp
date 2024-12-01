
#pragma once 

#include <e2/utils.hpp>


#include "game/components/skeletalmeshcomponent.hpp"
#include "game/components/staticmeshcomponent.hpp"
#include "game/components/audiocomponent.hpp"
#include "game/components/physicscomponent.hpp"
#include "game/components/fogcomponent.hpp"

#include <nlohmann/json.hpp>

namespace e2
{
	class ItemSpecification;
	class Game;
	class PlayerEntity;
	class GameContext;

	class PlayerEntity;
	class ItemSpecification;
	class RadionPreviewEntity;

	class WieldHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		WieldHandler() = default;
		virtual ~WieldHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps) = 0;
		virtual void finalize(e2::GameContext* ctx) = 0;

		virtual void setActive(e2::PlayerEntity* player) = 0;
		virtual void setInactive(e2::PlayerEntity* player) = 0;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) = 0;
		virtual void onRenderInventory(e2::PlayerEntity* player, double seconds) {};
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
	class IonizerHandler : public e2::WieldHandler
	{
		ObjectDeclaration();
	public:

		IonizerHandler() = default;
		virtual ~IonizerHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps) override;
		virtual void finalize(e2::GameContext* ctx) override;

		virtual void setActive(e2::PlayerEntity* player) override;
		virtual void setInactive(e2::PlayerEntity* player) override;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) override;
		virtual void onRenderInventory(e2::PlayerEntity* player, double seconds) override;


		virtual void onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName) override;

	protected:

		int32_t m_currentEntityIndex{};

		e2::StaticMeshSpecification m_meshSpecification;

		e2::StaticMeshComponent* m_mesh{};

		bool m_holding{};
		int32_t m_previewIndex{};
		e2::RadionPreviewEntity* m_previewEntity{};
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

	/** @tags(dynamic) */
	class ThrowableHandler : public e2::WieldHandler
	{
		ObjectDeclaration();
	public:

		ThrowableHandler() = default;
		virtual ~ThrowableHandler();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj, std::unordered_set<e2::Name>& deps) override;
		virtual void finalize(e2::GameContext* ctx) override;

		virtual void setActive(e2::PlayerEntity* player) override;
		virtual void setInactive(e2::PlayerEntity* player) override;

		virtual void onUpdate(e2::PlayerEntity* player, double seconds) override;
		virtual void onTrigger(e2::PlayerEntity* player, e2::Name actionName, e2::Name triggerName) override;

	protected:

		e2::Name m_entityName;
	};

	/** @tags(dynamic) */
	class UseHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		UseHandler() = default;

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) = 0;
		virtual void finalize(e2::GameContext* ctx) = 0;

		virtual void onUse(e2::PlayerEntity* player) = 0;

		e2::ItemSpecification* item{};
	protected:

	};

	/** @tags(dynamic) */
	class EquipHandler : public e2::Object
	{
		ObjectDeclaration();
	public:
		EquipHandler() = default;
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

	struct InventorySlot
	{
		e2::ItemSpecification* item{};
		uint32_t count;
	};

	struct PlayerState
	{

		PlayerState(e2::Game* g);

		bool give(e2::Name itemIdentifier);
		void drop(uint32_t slotIndex, uint32_t num);
		void take(uint32_t slotIndex, uint32_t num); // opposite of give, just makes items disappear
		void setActiveSlot(uint8_t newSlot);

		void update(double seconds);

		void renderInventory(double seconds);

		e2::Game* game{};
		e2::PlayerEntity* entity{};

		uint32_t activeSlot{}; // 0-7
		std::array<InventorySlot, 32> inventory; // 8x4, first row is ready to use 

	};
}

#include "playerstate.generated.hpp"