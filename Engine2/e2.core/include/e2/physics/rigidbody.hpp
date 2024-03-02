
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/physics/physicsresource.hpp>


namespace e2
{

	class IShape;
	struct E2_API RigidBodyCreateInfo
	{
		glm::mat4 initialTransform;
		glm::mat4 comOffset;

		double mass{ 0.0f };

		e2::IShape* shape{};

		// @todo integrate these 
		glm::vec3 localInertia;
		double linearDamping;
		double angularDamping;
		double friction;
		double rollingFriction;
		double spinningFriction;
		double restitution;
		double linearSleepingThreshold;
		double angularSleepingThreshold;


		int32_t group{};
		int32_t mask{};
	};

	class E2_API IRigidBody : public e2::PhysicsWorldResource
	{
		ObjectDeclaration()
	public:
		IRigidBody(IPhysicsWorld* wrld, RigidBodyCreateInfo const& createInfo);
		virtual ~IRigidBody();

		virtual void position(glm::vec3 const& newValue) = 0;
		virtual glm::vec3 position() = 0;

		virtual void rotation(glm::quat const& newRotation) = 0;
		virtual glm::quat rotation() = 0;

	protected:

	};
}

#include "rigidbody.generated.hpp"