
#include "e2/managers/typemanager.hpp"

#include "e2/game/component.hpp"

#include "init.inl"

e2::TypeManager::TypeManager(Engine* owner)
	: e2::Manager(owner)
{
	::registerGeneratedTypes();
}

e2::TypeManager::~TypeManager()
{

}

void e2::TypeManager::initialize()
{

}

void e2::TypeManager::shutdown()
{

}

void e2::TypeManager::preUpdate(double deltaTime)
{

}

void e2::TypeManager::update(double deltaTime)
{

}
