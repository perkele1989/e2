
#pragma once 

#include <e2/export.hpp>
#include <e2/utils.hpp>

namespace e2
{
	class IPhysicsContext;

	class E2_API PhysicsResource : public e2::Object
	{
		ObjectDeclaration()
	public:
		PhysicsResource(IPhysicsContext* context);
		virtual ~PhysicsResource();

		IPhysicsContext* physicsContext() const;

		template<typename ContextType>
		ContextType* physicsContext() const
		{
			return static_cast<ContextType*>(m_physicsContext);
		}

	protected:
		IPhysicsContext* m_physicsContext{};
	};

	class IPhysicsWorld;

	class E2_API PhysicsWorldResource : public e2::PhysicsResource
	{
		ObjectDeclaration()
	public:
		PhysicsWorldResource(IPhysicsWorld* world);
		virtual ~PhysicsWorldResource();

		IPhysicsWorld* physicsWorld() const;

		template<typename ContextType>
		ContextType* physicsWorld() const
		{
			return static_cast<ContextType*>(m_physicsWorld);
		}

	protected:
		IPhysicsWorld* m_physicsWorld{};
	};


}

#include "physicsresource.generated.hpp"