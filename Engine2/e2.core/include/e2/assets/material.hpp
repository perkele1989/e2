
#pragma once

#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <e2/assets/asset.hpp>
#include "e2/assets/texture2d.hpp"

#include <unordered_set>
#include <map>

namespace e2
{

	class ShaderModel;
	class MaterialProxy;
	class Texture2D;
	class Session;

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

		glm::vec4 getVec4(e2::Name key, glm::vec4 const& fallback);
		e2::Ptr<e2::Texture2D> getTexture(e2::Name key, e2::Ptr<e2::Texture2D> fallback);
		e2::Name getDefine(e2::Name key, e2::Name fallback);
		bool hasDefine(e2::Name key);

		std::unordered_set<e2::Session*> sessions;

	protected:
		e2::ShaderModel* m_model{};
		

		std::unordered_map<e2::Name, e2::Ptr<e2::Texture2D>> m_textureIndex;
		std::unordered_map<e2::Name, glm::vec4> m_vectorIndex;
		std::unordered_map<e2::Name, e2::Name> m_defines;

	};

}

#include "material.generated.hpp"