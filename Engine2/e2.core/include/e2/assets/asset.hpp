
#pragma once 

#include <e2/export.hpp>
#include <e2/context.hpp>
#include <e2/buffer.hpp>

#include <e2/utils.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <set>
#include <memory>

namespace e2
{
	class Asset;

	enum class AssetVersion : uint64_t
	{
		Zero = 0,
		// Versions start here
		Initial,

		// Optionally store multiple mips inside Texture2D assets, instead of just a basemip and then generating 
		AddMipsToTexture,

		// Add params to materiasl
		AddMaterialParameters,

		// Add skeletons 
		AddSkeletons,

		// 
		TextureImportRework,

		//
		AudioStream,

		// New versions above this line 
		End,
		Latest = End - 1
	};


	struct E2_API DependencySlot
	{
		/** Internal asset-specific name of this dependency, for example "albedoMap" or "runAnimation" */
		e2::Name dependencyName;

		/** The asset name for this dependency */
		e2::Name assetName;
	};

	/** */
	class E2_API Asset : public e2::Context, public e2::Data, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		Asset() = default;
		virtual ~Asset();

		virtual void postConstruct(e2::Context *ctx, e2::Name const& inUuid);

		virtual Engine* engine() override;

		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;
		virtual bool finalize();

		e2::Name findDependencyByName(e2::Name name);

		e2::Name name;
		e2::AssetVersion version;

		e2::StackVector<DependencySlot, e2::maxNumAssetDependencies> dependencies;

	protected:
		e2::Engine* m_engine{};

	};

}

#include "asset.generated.hpp"