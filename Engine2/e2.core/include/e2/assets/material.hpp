
#pragma once

#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/assets/asset.hpp>


#include <map>

namespace e2
{

	class ShaderModel;
	class MaterialProxy;


	/** @tags(dynamic, arena, arenaSize=2048) */
	class E2_API Material : public e2::Asset
	{
		ObjectDeclaration()
	public:
		Material() = default;
		virtual ~Material();


		virtual void write(Buffer& destination) const override;
		virtual bool read(Buffer& source) override;

		virtual bool finalize() override;


		inline e2::ShaderModel* model() const
		{
			return m_model;
		}

		void overrideModel(e2::ShaderModel* model);

	protected:
		e2::ShaderModel* m_model{};

	};

}

#include "material.generated.hpp"