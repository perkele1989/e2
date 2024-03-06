
#include "e2/context.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/gamemanager.hpp"

e2::Context::~Context()
{

}

e2::Application* e2::Context::application()
{
	return engine()->application();
}

e2::Config* e2::Context::config()
{
	return engine()->m_config;
}

e2::IRenderContext* e2::Context::renderContext()
{
	return renderManager()->renderContext();
}

e2::IThreadContext* e2::Context::mainThreadContext()
{
	return renderManager()->mainThreadContext();
}

//e2::IRenderContext* e2::Context::renderContext()
//{
//	return renderManager()->context();
//}

e2::RenderManager* e2::Context::renderManager()
{
	return engine()->m_renderManager;
}

e2::AssetManager* e2::Context::assetManager()
{
	return engine()->m_assetManager;
}

e2::AsyncManager* e2::Context::asyncManager()
{
	return engine()->m_asyncManager;
}

e2::GameManager* e2::Context::gameManager()
{
	return engine()->m_gameManager;
}

e2::TypeManager* e2::Context::typeManager()
{
	return engine()->m_typeManager;
}

e2::UIManager* e2::Context::uiManager()
{
	return engine()->m_uiManager;
}

