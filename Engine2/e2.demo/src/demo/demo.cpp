
#include "demo/demo.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/assets/mesh.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/game/world.hpp"
#include "e2/game/entity.hpp"
#include "e2/game/meshcomponent.hpp"
#include "e2/game/cameracomponent.hpp"

e2::Demo::Demo(e2::Context* ctx)
	: e2::Application(ctx)
{

}

e2::Demo::~Demo()
{

}

void e2::Demo::initialize()
{
	e2::ALJDescription alj;
	assetManager()->prescribeALJ(alj, "assets/SM_HexBase.e2a");
	assetManager()->queueWaitALJ(alj);
	e2::MeshPtr demoMesh = assetManager()->get("assets/SM_HexBase.e2a").cast<e2::Mesh>();

	e2::World* world = gameSession()->persistentWorld();
	e2::Entity* demoEntity = world->spawnEntity<e2::Entity>("DemoEntity");
	e2::MeshComponent* demoMeshCmp = demoEntity->spawnComponent<e2::MeshComponent>("DemoMeshCmp");
	demoMeshCmp->mesh(demoMesh);

	//glm::quat rot = glm::rotate(glm::identity<glm::quat>(), glm::radians(90.0f), e2::worldRight());
	//demoMeshCmp->getTransform()->setRotation(rot, e2::TransformSpace::World);
	//demoMeshCmp->getTransform()->setTranslation(e2::worldForward() * -3.0f, TransformSpace::World);

	e2::Entity* camEntity = world->spawnEntity<e2::Entity>("CameraEntity");
	e2::CameraComponent* camComponent = camEntity->spawnComponent<e2::CameraComponent>("CameraCmp");




}

void e2::Demo::shutdown()
{

}

void e2::Demo::update(double seconds)
{

}


e2::ApplicationType e2::Demo::type()
{
	return e2::ApplicationType::Game;
}

int main(int argc, char** argv)
{

	e2::Engine engine;
	e2::Demo demo(&engine);

	engine.run(&demo);

	return 0;
}