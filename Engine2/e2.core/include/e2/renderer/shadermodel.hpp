

#pragma once 

#include <e2/context.hpp>

#include <e2/renderer/renderer.hpp>
#include <e2/renderer/meshspecification.hpp>
#include <e2/renderer/meshproxy.hpp>

#include <e2/assets/material.hpp>
#include <e2/assets/mesh.hpp>

#include <e2/rhi/pipeline.hpp>
#include <e2/rhi/threadcontext.hpp>



#include <glm/glm.hpp>

#include <vector>

namespace e2
{
	class Engine;
	class MeshProxy;

	struct E2_API ShaderModelSpecification
	{
		bool isCompatible(e2::SubmeshSpecification const& mesh);

		e2::VertexAttributeFlags requiredAttributes { };
	};

	/** @tags() */
	class E2_API ShaderModel : public e2::Context, public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		ShaderModel();
		virtual ~ShaderModel();

		virtual void postConstruct(e2::Context* ctx);

		inline ShaderModelSpecification const& specification() const
		{
			return m_specification;
		}

		virtual Engine* engine() override;

		virtual e2::MaterialProxy* createMaterialProxy(e2::Session* session, e2::MaterialPtr material) = 0;

		virtual e2::IPipelineLayout* getOrCreatePipelineLayout(e2::MeshProxy* proxy, uint8_t submeshIndex, bool shadows) = 0;
		virtual e2::IPipeline* getOrCreatePipeline(e2::MeshProxy* proxy, uint8_t submeshIndex, e2::RendererFlags rendererFlags) = 0;

		virtual e2::RenderLayer renderLayer();

		virtual void invalidatePipelines()=0;

		virtual bool supportsShadows();

	protected:
		e2::Engine* m_engine{};

		ShaderModelSpecification m_specification{};



	public:

	};

}

#include "shadermodel.generated.hpp"