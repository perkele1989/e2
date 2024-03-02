
#include "e2/manager.hpp"

e2::Manager::Manager(Engine* owner)
	: m_owner(owner)
{

}

e2::Manager::~Manager()
{

}

e2::Engine* e2::Manager::engine()
{
	return m_owner;
}
