
#include "e2/renderer/shadermodel.hpp"

#include "e2/renderer/shared.hpp"

#include <algorithm>


e2::ShaderModel::ShaderModel()
	: e2::ManagedObject()
{
	
}

e2::ShaderModel::~ShaderModel()
{
}

void e2::ShaderModel::postConstruct(e2::Context* ctx)
{
	m_engine = ctx->engine();
}

e2::Engine* e2::ShaderModel::engine()
{
	return m_engine;
}

e2::RenderLayer e2::ShaderModel::renderLayer()
{
	return e2::RenderLayer::Default;
}

bool e2::ShaderModel::supportsShadows()
{
	return false;
}

void e2::ShaderModel::active(bool newValue)
{
	m_active = newValue;
}

bool e2::ShaderModel::active()
{
	return m_active;
}
