
#include "e2/optikus/optikusmodel.hpp"

e2::OptikusProxy::OptikusProxy(e2::Session* inSession, e2::MaterialPtr materialAsset)
 : e2::MaterialProxy(inSession, materialAsset)
{

}

e2::OptikusProxy::~OptikusProxy()
{

}

void e2::OptikusProxy::bind(e2::ICommandBuffer* buffer, uint8_t frameIndex, bool shadows)
{

}

void e2::OptikusProxy::invalidate(uint8_t frameIndex)
{

}

e2::OptikusModel::OptikusModel()
{

}

e2::OptikusModel::~OptikusModel()
{

}

void e2::OptikusModel::postConstruct(e2::Context* ctx)
{
	e2::ShaderModel::postConstruct(ctx);
}

e2::MaterialProxy* e2::OptikusModel::createMaterialProxy(e2::Session* session, e2::MaterialPtr material)
{
	return e2::create<e2::OptikusProxy>(session, material);
}

e2::IPipelineLayout* e2::OptikusModel::getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, bool shadows)
{
	return nullptr;
}

e2::IPipeline* e2::OptikusModel::getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t lodIndex, uint8_t submeshIndex, e2::RendererFlags rendererFlags)
{
	return nullptr;
}

void e2::OptikusModel::invalidatePipelines()
{

}
