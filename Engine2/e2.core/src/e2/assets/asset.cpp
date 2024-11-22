
#include "e2/assets/asset.hpp"

#include "e2/managers/assetmanager.hpp"

e2::Asset::~Asset()
{
	//LogNotice("Asset DESTROY: {}", uuid.string());
}

void e2::Asset::postConstruct(e2::Context* ctx, e2::Name const& inName)
{
	m_engine = ctx->engine();
	name = inName;
}

e2::Engine* e2::Asset::engine()
{
	return m_engine;
}

void e2::Asset::write(e2::IStream& destination) const
{

}

bool e2::Asset::read(e2::IStream& source)
{
	return true;
}

bool e2::Asset::finalize()
{
	return true;
}

e2::Name e2::Asset::findDependencyByName(e2::Name name)
{
	for (uint8_t i = 0; i < dependencies.size(); i++)
	{
		if (dependencies[i].dependencyName == name)
			return dependencies[i].assetName;
	}

	return e2::Name("null");
}
