
#pragma once 

#include <e2/utils.hpp>
#include "game/entity.hpp"

namespace e2
{
	constexpr uint64_t maxNumRadionPins = 4;
	constexpr uint64_t maxNumRadionConnections = 4;

	enum class RadionPinType : uint8_t
	{
		Input,
		Output
	};

	struct RadionPin
	{
		e2::Name name;
		e2::RadionPinType type;
		glm::vec3 offset;
		
	};

	class RadionEntity;

	struct RadionConnection
	{
		e2::RadionEntity* otherEntity{};
		e2::Name otherPin;
	};

	inline bool operator== (RadionConnection const& lhs, RadionConnection const& rhs) noexcept
	{
		return lhs.otherEntity == rhs.otherEntity && lhs.otherPin == rhs.otherPin;
	}

	struct RadionSlot
	{
		e2::StackVector<e2::RadionConnection, maxNumRadionConnections> connections;
	};

	/** @tags(dynamic) */
	class RadionEntitySpecification : public e2::EntitySpecification
	{
		ObjectDeclaration();
	public:
		RadionEntitySpecification();
		virtual ~RadionEntitySpecification();

		virtual void populate(e2::GameContext* ctx, nlohmann::json& obj) override;
		virtual void finalize() override;

		int32_t pinIndexFromName(e2::Name name);

		e2::StackVector<e2::RadionPin, maxNumRadionPins> pins;
		e2::StaticMeshSpecification mesh;
		e2::CollisionSpecification collision;

		uint32_t radionCost{ 4 };
	};

	struct ConnectionMesh
	{
		uint32_t pinIndex;
		e2::MeshPtr generatedMesh;
		e2::MeshProxy* meshProxy{};
	};

	/** @tags(dynamic) */
	class RadionEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		RadionEntity()=default;
		virtual ~RadionEntity();

		virtual void radionTick();

		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		virtual void update(double seconds) override;
		virtual void updateAnimation(double seconds) override;

		virtual void updateVisibility() override;

		void connectOutputPin(e2::Name outputPinName, e2::RadionEntity* inputEntity, e2::Name inputPinName);
		void disconnectInputPin(e2::Name pinName);
		void disconnectOutputPin(e2::Name pinName);

		float getInputRadiance(e2::Name pinName);
		float getOutputRadiance(e2::Name pinName);
		void setOutputRadiance(e2::Name pinName, float newRadiance);

		bool anyOutputConnected();

		void destroyConnectionMeshes();
		void updateConnectionMeshes();

		e2::StackVector<RadionSlot, e2::maxNumRadionPins> slots;
		e2::StackVector<float, e2::maxNumRadionPins> outputRadiance;
		e2::StackVector<ConnectionMesh, e2::maxNumRadionPins * e2::maxNumRadionConnections> connectionMeshes;

		e2::RadionEntitySpecification* radionSpecification{};

	protected:
		e2::CollisionComponent* m_collision{};
		e2::StaticMeshComponent* m_mesh{};
	};



	/** @tags(dynamic) */
	class RadionPreviewEntity : public e2::Entity
	{
		ObjectDeclaration();
	public:
		RadionPreviewEntity() = default;
		virtual ~RadionPreviewEntity();

		virtual void postConstruct(e2::GameContext* ctx, e2::EntitySpecification* spec, glm::vec3 const& worldPosition, glm::quat const& worldRotation) override;

		e2::RadionEntitySpecification* radionSpecification{};
		e2::StaticMeshComponent* mesh{};
	};






	constexpr float radionDecay = 0.985f;
	constexpr float radionPowerTreshold = 4.2f;
	constexpr float radionSignalTreshold = 2.5f;
	constexpr float radionHighRadiance = 5.0f;
	constexpr float radionLowRadiance = 0.0f;


	/** @tags(dynamic) */
	class RadionPowerSource : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionPowerSource() = default;
		virtual ~RadionPowerSource() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionWirePost : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionWirePost() = default;
		virtual ~RadionWirePost() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionSplitter : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionSplitter() = default;
		virtual ~RadionSplitter() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) basically a repeater (charges to 5V by adding input, once 5V sets output to 5V, otherwise sets output to 0V*/
	class RadionCapacitor : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionCapacitor() = default;
		virtual ~RadionCapacitor() = default;

		virtual void radionTick() override;

	};

	/** @tags(dynamic) sets output to input if state is true, otherwise sets output to 0V */
	class RadionSwitch : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionSwitch() = default;
		virtual ~RadionSwitch() = default;

		virtual void radionTick() override;



		virtual void writeForSave(e2::IStream& toBuffer) override;
		virtual void readForSave(e2::IStream& fromBuffer) override;

		virtual bool interactable() { return true; }
		virtual void onInteract(e2::Entity* interactor);
		virtual std::string interactText()override { return "Press"; };

		bool state{ false };
	};

	/** @tags(dynamic) alternates output between input V and 0V every tick */
	class RadionCrystal : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionCrystal() = default;
		virtual ~RadionCrystal()=default;

		virtual void radionTick() override;

		bool state{ false };
	};

	/** @tags(dynamic) */
	class RadionGateNOT : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateNOT() = default;
		virtual ~RadionGateNOT() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionGateAND : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateAND() = default;
		virtual ~RadionGateAND() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionGateOR : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateOR() = default;
		virtual ~RadionGateOR() = default;

		virtual void radionTick() override;
	};


	/** @tags(dynamic) */
	class RadionGateXOR : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateXOR() = default;
		virtual ~RadionGateXOR() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionGateNAND : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateNAND() = default;
		virtual ~RadionGateNAND() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionGateNOR : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateNOR() = default;
		virtual ~RadionGateNOR() = default;

		virtual void radionTick() override;
	};

	/** @tags(dynamic) */
	class RadionGateXNOR : public e2::RadionEntity
	{
		ObjectDeclaration();
	public:
		RadionGateXNOR() = default;
		virtual ~RadionGateXNOR() = default;

		virtual void radionTick() override;
	};
}

#include "radionentity.generated.hpp"