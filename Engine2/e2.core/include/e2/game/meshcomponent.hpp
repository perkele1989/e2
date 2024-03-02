
#pragma once 

#include <e2/game/scenecomponent.hpp>

#include <e2/assets/material.hpp>
#include <e2/assets/mesh.hpp>
#include <e2/renderer/meshproxy.hpp>

namespace e2
{

	class MeshProxy;

	/** @tags(arena, arenaSize=8192, dynamic) */
	class E2_API MeshComponent : public e2::SceneComponent
	{
		ObjectDeclaration()
	public:
		MeshComponent();
		MeshComponent(e2::Entity* entity, e2::Name name);
		virtual ~MeshComponent();
		
		bool assertProxy();
		bool recreateProxy();
		void destroyProxy();

		virtual void onSpawned() override;
		virtual void onDestroyed() override;

		virtual void tick(double seconds) override;

		void mesh(e2::MeshPtr newMesh);
		e2::MeshPtr mesh() const;

		e2::StackVector<e2::MaterialProxy*, e2::maxNumSubmeshes>const & materialProxies() const;

		void setMaterial(uint8_t submeshIndex, e2::MaterialProxy* newMaterial);

	protected:
		e2::MeshProxy* m_proxy{};

		e2::MeshPtr m_mesh; 
		e2::StackVector<e2::MaterialProxy*, e2::maxNumSubmeshes> m_materialProxies;
	};
}

#include <meshcomponent.generated.hpp>