
#include "e2/renderer/renderer.hpp"

#include "e2/managers/rendermanager.hpp"
//#include "e2/render/command.hpp"
//#include "e2/render/framebuffer.hpp"
#include "e2/renderer/shadermodel.hpp"
#include "e2/game/session.hpp"
#include "e2/rhi/pipeline.hpp"
#include "e2/transform.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/intersect.hpp>

#include <vector>



static char const* lineVertexSource = R"SRC(
	#version 460 core

	out vec4 fragmentPosition;

	// Push constants
	layout(push_constant) uniform ConstantData
	{
		vec4 vertexColor;
		vec4 vertexPositions[2];
	};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 time; // t, sin(t), cos(t), tan(t)
    vec4 sun1; // sun dir.xyz, ???
    vec4 sun2; // sun color.rgb, sun strength
    vec4 ibl1; // ibl strength, ??, ?? ,??
    vec4 cameraPosition; // pos.xyz, ??
} renderer;

layout(set = 0, binding = 1) uniform sampler clampSampler;
layout(set = 0, binding = 2) uniform sampler repeatSampler;
layout(set = 0, binding = 3) uniform sampler equirectSampler;
layout(set = 0, binding = 4) uniform texture2D integratedBrdf;

layout(set = 0, binding = 5) uniform texture2D irradianceCube;
layout(set = 0, binding = 6) uniform texture2D radianceCube;

layout(set = 0, binding = 7) uniform texture2D frontBufferColor;
layout(set = 0, binding = 8) uniform texture2D frontBufferPosition;
layout(set = 0, binding = 9) uniform texture2D frontBufferDepth;
// End Set0

	void main()
	{
		gl_Position = renderer.projectionMatrix * renderer.viewMatrix * vertexPositions[gl_VertexIndex];
		fragmentPosition = vertexPositions[gl_VertexIndex];
	}
)SRC";

static char const* lineFragmentSource = R"SRC(
	#version 460 core

	in vec4 fragmentPosition;

	// Push constants
	layout(push_constant) uniform ConstantData
	{
		vec4 vertexColor;
		vec4 vertexPositions[2];
	};

// Begin Set0: Renderer
layout(set = 0, binding = 0) uniform RendererData
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
    vec4 time; // t, sin(t), cos(t), tan(t)
} renderer;

layout(set = 0, binding = 1) uniform sampler clampSampler;
layout(set = 0, binding = 2) uniform sampler repeatSampler;
layout(set = 0, binding = 3) uniform sampler equirectSampler;
layout(set = 0, binding = 4) uniform texture2D integratedBrdf;

layout(set = 0, binding = 5) uniform texture2D irradianceCube;
layout(set = 0, binding = 6) uniform texture2D radianceCube;

layout(set = 0, binding = 7) uniform texture2D frontBufferColor;
layout(set = 0, binding = 8) uniform texture2D frontBufferPosition;
layout(set = 0, binding = 9) uniform texture2D frontBufferDepth;
// End Set0

	out vec4 outColor;
	out vec4 outPosition;

	void main()
	{
		outColor = vec4(1.0, 1.0, 1.0, 1.0) * vertexColor;
		outPosition = fragmentPosition;
	}
)SRC";






e2::Renderer::Renderer(e2::Session* session, glm::uvec2 const& resolution)
	: m_session(session)
	, m_resolution(resolution)
{
	// Setup command buffers 
	e2::CommandBufferCreateInfo commandBufferInfo{};
	m_commandBuffers[0] = renderManager()->framePool(0)->createBuffer(commandBufferInfo);
	m_commandBuffers[1] = renderManager()->framePool(1)->createBuffer(commandBufferInfo);

	e2::DataBufferCreateInfo bufferCreateInfo{};
	bufferCreateInfo.dynamic = true;
	bufferCreateInfo.size = sizeof(RendererData);
	bufferCreateInfo.type = BufferType::UniformBuffer;

	m_rendererBuffers[0] = renderContext()->createDataBuffer(bufferCreateInfo);
	m_rendererBuffers[1] = renderContext()->createDataBuffer(bufferCreateInfo);

	m_rendererBuffers[0]->upload(reinterpret_cast<uint8_t const*>(&m_rendererData), sizeof(RendererData), 0, 0);
	m_rendererBuffers[1]->upload(reinterpret_cast<uint8_t const*>(&m_rendererData), sizeof(RendererData), 0, 0);


	m_radiance = renderManager()->defaultTexture()->handle();
	m_irradiance = renderManager()->defaultTexture()->handle();

	// Setup render buffers
	for (uint8_t i = 0; i < 2; i++)
	{
		uint8_t other_i = i == 0 ? 1 : 0;
		e2::TextureCreateInfo textureCreateInfo{};
		textureCreateInfo.resolution.x = resolution.x;
		textureCreateInfo.resolution.y = resolution.y;
		textureCreateInfo.resolution.z = 1;
		textureCreateInfo.mips = 1;
		textureCreateInfo.arrayLayers = 1;
		textureCreateInfo.type = TextureType::Texture2D;
		textureCreateInfo.format = TextureFormat::RGBA8;

		textureCreateInfo.initialLayout = TextureLayout::ShaderRead; // we use shader read as default for this, and then swithc when rendering
		m_renderBuffers[i].colorTexture = renderContext()->createTexture(textureCreateInfo);

		textureCreateInfo.format = TextureFormat::RGBA32;
		m_renderBuffers[i].positionTexture = renderContext()->createTexture(textureCreateInfo);

		textureCreateInfo.format = TextureFormat::D32;
		textureCreateInfo.initialLayout = TextureLayout::ShaderRead; // we use shader read as default for this, and then swithc when rendering
		m_renderBuffers[i].depthTexture = renderContext()->createTexture(textureCreateInfo);

		e2::RenderTargetCreateInfo renderTargetInfo{};
		renderTargetInfo.areaExtent = resolution;

		e2::RenderAttachment colorAttachment{};
		colorAttachment.target = m_renderBuffers[i].colorTexture;
		colorAttachment.clearMethod = ClearMethod::ColorFloat;
		colorAttachment.clearValue.clearColorf32 = { 0.f, 0.f, 0.f, 1.0f };
		colorAttachment.loadOperation = LoadOperation::Load;
		colorAttachment.storeOperation = StoreOperation::Store;
		renderTargetInfo.colorAttachments.push(colorAttachment);

		colorAttachment.target = m_renderBuffers[i].positionTexture;
		renderTargetInfo.colorAttachments.push(colorAttachment);

		renderTargetInfo.depthAttachment.target = m_renderBuffers[i].depthTexture;
		renderTargetInfo.depthAttachment.clearMethod = ClearMethod::DepthStencil;
		renderTargetInfo.depthAttachment.clearValue.depth = 1.0f;
		renderTargetInfo.depthAttachment.loadOperation = LoadOperation::Load;
		renderTargetInfo.depthAttachment.storeOperation = StoreOperation::Store;

		m_renderBuffers[i].renderTarget = renderContext()->createRenderTarget(renderTargetInfo);


		m_renderBuffers[i].sets[0] = renderManager()->rendererPool()->createDescriptorSet(renderManager()->rendererSetLayout());
		m_renderBuffers[i].sets[1] = renderManager()->rendererPool()->createDescriptorSet(renderManager()->rendererSetLayout());
	}

	// bind interdependencies 
	for (uint8_t i = 0; i < 2; i++)
	{
		uint8_t other_i = i == 0 ? 1 : 0;
		m_renderBuffers[i].sets[0]->writeUniformBuffer(0, m_rendererBuffers[0], sizeof(RendererData), 0);
		m_renderBuffers[i].sets[0]->writeSampler(1, renderManager()->clampSampler());
		m_renderBuffers[i].sets[0]->writeSampler(2, renderManager()->repeatSampler());
		m_renderBuffers[i].sets[0]->writeSampler(3, renderManager()->equirectSampler());
		m_renderBuffers[i].sets[0]->writeTexture(4, renderManager()->integratedBrdf()->handle());
		m_renderBuffers[i].sets[0]->writeTexture(5, renderManager()->defaultTexture()->handle());
		m_renderBuffers[i].sets[0]->writeTexture(6, renderManager()->defaultTexture()->handle());
		m_renderBuffers[i].sets[0]->writeTexture(7, m_renderBuffers[other_i].colorTexture);
		m_renderBuffers[i].sets[0]->writeTexture(8, m_renderBuffers[other_i].positionTexture);
		m_renderBuffers[i].sets[0]->writeTexture(9, m_renderBuffers[other_i].depthTexture);

		m_renderBuffers[i].sets[1]->writeUniformBuffer(0, m_rendererBuffers[1], sizeof(RendererData), 0);
		m_renderBuffers[i].sets[1]->writeSampler(1, renderManager()->clampSampler());
		m_renderBuffers[i].sets[1]->writeSampler(2, renderManager()->repeatSampler());
		m_renderBuffers[i].sets[1]->writeSampler(3, renderManager()->equirectSampler());
		m_renderBuffers[i].sets[1]->writeTexture(4, renderManager()->integratedBrdf()->handle());
		m_renderBuffers[i].sets[1]->writeTexture(5, renderManager()->defaultTexture()->handle());
		m_renderBuffers[i].sets[1]->writeTexture(6, renderManager()->defaultTexture()->handle());
		m_renderBuffers[i].sets[1]->writeTexture(7, m_renderBuffers[other_i].colorTexture);
		m_renderBuffers[i].sets[1]->writeTexture(8, m_renderBuffers[other_i].positionTexture);
		m_renderBuffers[i].sets[1]->writeTexture(9, m_renderBuffers[other_i].depthTexture);
	}




	m_defaultSettings.frontFace = FrontFace::CCW;

	e2::ShaderCreateInfo lineVertexInfo;
	lineVertexInfo.stage = ShaderStage::Vertex;
	lineVertexInfo.source = lineVertexSource;
	m_lineVertexShader = renderContext()->createShader(lineVertexInfo);

	e2::ShaderCreateInfo lineFragInfo;
	lineFragInfo.stage = ShaderStage::Fragment;
	lineFragInfo.source = lineFragmentSource;
	m_lineFragmentShader = renderContext()->createShader(lineFragInfo);

	e2::PipelineCreateInfo lineCreateInfo{};
	lineCreateInfo.layout = renderManager()->linePipelineLayout();
	lineCreateInfo.topology = PrimitiveTopology::Line;
	lineCreateInfo.colorFormats = { TextureFormat::RGBA8 , e2::TextureFormat::RGBA32 };
	lineCreateInfo.depthFormat = TextureFormat::D32;
	lineCreateInfo.shaders.push(m_lineVertexShader);
	lineCreateInfo.shaders.push(m_lineFragmentShader);
	m_linePipeline = renderContext()->createPipeline(lineCreateInfo);



}

e2::Renderer::~Renderer()
{
	e2::discard(m_linePipeline);
	e2::discard(m_lineVertexShader);
	e2::discard(m_lineFragmentShader);
	e2::discard(m_rendererBuffers[0]);
	e2::discard(m_rendererBuffers[1]);
	e2::discard(m_renderBuffers[0].renderTarget);
	e2::discard(m_renderBuffers[0].colorTexture);
	e2::discard(m_renderBuffers[0].positionTexture);
	e2::discard(m_renderBuffers[0].depthTexture);
	e2::discard(m_renderBuffers[0].sets[0]);
	e2::discard(m_renderBuffers[0].sets[1]);
	e2::discard(m_renderBuffers[1].renderTarget);
	e2::discard(m_renderBuffers[1].colorTexture);
	e2::discard(m_renderBuffers[1].positionTexture);
	e2::discard(m_renderBuffers[1].depthTexture);
	e2::discard(m_renderBuffers[1].sets[0]);
	e2::discard(m_renderBuffers[1].sets[1]);

	e2::discard(m_commandBuffers[1]);
	e2::discard(m_commandBuffers[0]);
}

e2::Engine* e2::Renderer::engine()
{
	return m_session->engine();
}

void e2::Renderer::recordFrame(double deltaTime)
{
	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];

	e2::IDescriptorSet* modelSet = m_session->getModelSet(frameIndex);

	std::map<e2::RenderLayer, std::unordered_set<MeshProxySubmesh>> const& submeshIndex = m_session->submeshIndex();

	m_rendererData.projectionMatrix = m_view.calculateProjectionMatrix(m_resolution);
	m_rendererData.viewMatrix = m_view.calculateViewMatrix(); 
	
	m_rendererData.time.x += (float)deltaTime;
	m_rendererData.time.y = glm::sin((float)m_rendererData.time.x);
	m_rendererData.time.z = glm::cos((float)m_rendererData.time.x);
	m_rendererData.time.w = glm::tan((float)m_rendererData.time.x);

	m_rendererData.sun1 = glm::vec4(m_sunDirection, 0.0f);
	m_rendererData.sun2 = glm::vec4(m_sunColor, m_sunStrength);

	m_rendererData.ibl1 = glm::vec4(m_iblStrength, 0.0f, 0.0f, 0.0f);
	m_rendererData.cameraPosition = glm::vec4(glm::vec3(m_view.origin), 0.0f);
	
	m_rendererBuffers[frameIndex]->upload(reinterpret_cast<uint8_t const*>(&m_rendererData), sizeof(RendererData), 0, 0);

	if (m_irradiance)
	{
		m_renderBuffers[0].sets[frameIndex]->writeTexture(5, m_irradiance);
		m_renderBuffers[1].sets[frameIndex]->writeTexture(5, m_irradiance);
	}

	if (m_radiance)
	{
		m_renderBuffers[0].sets[frameIndex]->writeTexture(6, m_radiance);
		m_renderBuffers[1].sets[frameIndex]->writeTexture(6, m_radiance);
	}

	// Begin command buffer
	buff->beginRecord(true, m_defaultSettings);
	{
		buff->useAsAttachment(m_renderBuffers[0].colorTexture);
		buff->useAsAttachment(m_renderBuffers[0].positionTexture);
		buff->useAsDepthAttachment(m_renderBuffers[0].depthTexture);
		buff->beginRender(m_renderBuffers[0].renderTarget);
		buff->clearColor(0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		buff->clearColor(1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		buff->clearDepth(1.0f);
		buff->endRender();
		buff->useAsDefault(m_renderBuffers[0].colorTexture);
		buff->useAsDefault(m_renderBuffers[0].positionTexture);
		buff->useAsDefault(m_renderBuffers[0].depthTexture);

		buff->useAsAttachment(m_renderBuffers[1].colorTexture);
		buff->useAsAttachment(m_renderBuffers[1].positionTexture);
		buff->useAsDepthAttachment(m_renderBuffers[1].depthTexture);
		buff->beginRender(m_renderBuffers[1].renderTarget);
		buff->clearColor(0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		buff->clearColor(1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
		buff->clearDepth(1.0f);
		buff->endRender();
		buff->useAsDefault(m_renderBuffers[1].colorTexture);
		buff->useAsDefault(m_renderBuffers[1].positionTexture);
		buff->useAsDefault(m_renderBuffers[1].depthTexture);

		// iterate all render layers, and render their respective submeshes 
		for (auto& pair : submeshIndex)
		{
			e2::RenderLayer renderLayer = pair.first;
			std::unordered_set<e2::MeshProxySubmesh> const& submeshSet = pair.second;

			uint8_t frontBuffIndex = frontBuffer();
			auto& backBuff = m_renderBuffers[m_backBuffer];
			auto& frontBuff = m_renderBuffers[frontBuffIndex];

			e2::IDescriptorSet* rendererSet = backBuff.sets[frameIndex];

			buff->useAsAttachment(backBuff.colorTexture);
			buff->useAsAttachment(backBuff.positionTexture);
			buff->useAsDepthAttachment(backBuff.depthTexture);
			buff->beginRender(backBuff.renderTarget);
			buff->clearColor(0, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
			buff->clearColor(1, glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
			buff->clearDepth(1.0f);
			buff->endRender();
			buff->useAsDefault(backBuff.colorTexture);
			buff->useAsDefault(backBuff.positionTexture);
			buff->useAsDefault(backBuff.depthTexture);

			buff->useAsTransferDst(backBuff.colorTexture);
			buff->useAsTransferSrc(frontBuff.colorTexture);
			buff->blit(backBuff.colorTexture, frontBuff.colorTexture, 0, 0);
			buff->useAsDefault(frontBuff.colorTexture);
			buff->useAsAttachment(backBuff.colorTexture);

			buff->useAsTransferDst(backBuff.positionTexture);
			buff->useAsTransferSrc(frontBuff.positionTexture);
			buff->blit(backBuff.positionTexture, frontBuff.positionTexture, 0, 0);
			buff->useAsDefault(frontBuff.positionTexture);
			buff->useAsAttachment(backBuff.positionTexture);

			buff->useAsTransferDst(backBuff.depthTexture);
			buff->useAsTransferSrc(frontBuff.depthTexture);
			buff->blit(backBuff.depthTexture, frontBuff.depthTexture, 0, 0);
			buff->useAsDefault(frontBuff.depthTexture);
			buff->useAsDepthAttachment(backBuff.depthTexture);
		
			// Setup render target states
			buff->beginRender(backBuff.renderTarget);
			for (e2::MeshProxySubmesh const& meshProxySubmesh : submeshSet)
			{
				e2::MeshProxy* meshProxy = meshProxySubmesh.proxy;
				uint8_t submeshIndex = meshProxySubmesh.submesh;

				if (!meshProxy->pipelines[submeshIndex])
					continue;

				buff->bindPipeline(meshProxy->pipelines[submeshIndex]);

				e2::MeshPtr mesh = meshProxy->asset;
				e2::IPipelineLayout* pipelineLayout = meshProxy->pipelineLayouts[submeshIndex];

				e2::SubmeshSpecification const& meshSpec = mesh->specification(submeshIndex);

				// Push constant data
				e2::PushConstantData pushConstantData;
				pushConstantData.normalMatrix = glm::transpose(glm::inverse(meshProxy->modelMatrix));
				pushConstantData.resolution =m_resolution;
				buff->pushConstants(pipelineLayout, 0, sizeof(e2::PushConstantData), reinterpret_cast<uint8_t*>(&pushConstantData));

				// Bind vertex states
				buff->bindVertexLayout(meshSpec.vertexLayout);
				buff->bindIndexBuffer(meshSpec.indexBuffer);
				for (uint8_t i = 0; i < meshSpec.vertexAttributes.size(); i++)
					buff->bindVertexBuffer(i, meshSpec.vertexAttributes[i]);

				buff->bindDescriptorSet(pipelineLayout, 0, rendererSet);

				//if (meshProxy->skinProxy)
				{
					uint32_t skinId = meshProxy->skinProxy ? meshProxy->skinProxy->id : 0;
					// Bind descriptor sets (0 is renderer, 1 is model, 2 is material, 3 is reserved)
					uint32_t offsets[2] = {
						renderManager()->paddedBufferSize(sizeof(glm::mat4)) * meshProxy->id,
						renderManager()->paddedBufferSize(sizeof(glm::mat4) * e2::maxNumSkeletonBones) * skinId
					};

					buff->bindDescriptorSet(pipelineLayout, 1, modelSet, 2, &offsets[0]);
				}
				/*else
				{
					// Bind descriptor sets (0 is renderer, 1 is model, 2 is material, 3 is reserved)
					uint32_t offsets[1] = {
						renderManager()->paddedBufferSize(sizeof(glm::mat4)) * meshProxy->id
					};

					buff->bindDescriptorSet(pipelineLayout, 1, modelSet, 1, &offsets[0]);
				}
				*/
				meshProxy->materialProxies[submeshIndex]->bind(buff, frameIndex);

				// Issue drawcall
				buff->draw(meshSpec.indexCount, 1);
						
			}
			
			buff->endRender();

			buff->useAsDefault(backBuff.colorTexture);
			buff->useAsDefault(backBuff.positionTexture);
			buff->useAsDefault(backBuff.depthTexture);

			swapRenderBuffers();
		}



		if (m_debugLines.size() == 0)
		{
			//LogWarning("No debug lines");
		}
		else
		{
			uint8_t frontBuffIndex = frontBuffer();
			auto& backBuff = m_renderBuffers[m_backBuffer];
			auto& frontBuff = m_renderBuffers[frontBuffIndex];

			e2::IDescriptorSet* rendererSet = m_renderBuffers[m_backBuffer].sets[frameIndex];


			buff->useAsTransferDst(backBuff.colorTexture);
			buff->useAsTransferSrc(frontBuff.colorTexture);
			buff->blit(backBuff.colorTexture, frontBuff.colorTexture, 0, 0);
			buff->useAsDefault(frontBuff.colorTexture);
			buff->useAsAttachment(backBuff.colorTexture);

			buff->useAsTransferDst(backBuff.depthTexture);
			buff->useAsTransferSrc(frontBuff.depthTexture);
			buff->blit(backBuff.depthTexture, frontBuff.depthTexture, 0, 0);
			buff->useAsDefault(frontBuff.depthTexture);
			buff->useAsDepthAttachment(backBuff.depthTexture);


			// Setup render target states
			buff->beginRender(backBuff.renderTarget);


			buff->setDepthTest(false);
			buff->bindPipeline(m_linePipeline);
			buff->nullVertexLayout();
			buff->bindDescriptorSet(renderManager()->linePipelineLayout(), 0, rendererSet);
			for (uint32_t i = 0; i < m_debugLines.size(); i++)
			{
				glm::vec4 lineConstants[3] = {
					{m_debugLines[i].color, 1.0f},
					{m_debugLines[i].start, 1.0f},
					{m_debugLines[i].end, 1.0f},
				};

				buff->pushConstants(renderManager()->linePipelineLayout(), 0, sizeof(glm::vec4) * 3, reinterpret_cast<uint8_t*>(&lineConstants[0]));
				buff->drawNonIndexed(2, 1);
			}


			buff->endRender();

			buff->useAsDefault(backBuff.colorTexture);
			buff->useAsDefault(backBuff.depthTexture);

			swapRenderBuffers();
		}

		buff->endRecord();
	}

	renderManager()->queue(buff, nullptr, nullptr);
	m_debugLines.clear();
}

e2::Session* e2::Renderer::session() const
{
	return m_session;
}

void e2::Renderer::setView(e2::RenderView const& renderView)
{
	m_view = renderView;

	m_viewPoints = e2::Viewpoints2D(m_resolution, m_view);

}

glm::dmat4 e2::RenderView::calculateViewMatrix() const
{
	glm::dmat4 translateMatrix = glm::translate(glm::identity<glm::dmat4>(), origin);
	glm::dmat4 rotateMatrix = glm::mat4_cast(orientation);
	return glm::inverse(translateMatrix * rotateMatrix);
}

glm::dmat4 e2::RenderView::calculateProjectionMatrix( glm::dvec2 const& resolution) const
{
	return glm::perspectiveFov(glm::radians(fov), resolution.x, resolution.y, clipPlane.x, clipPlane.y);
}

glm::dvec3 e2::RenderView::findWorldspaceViewRayFromNdc(glm::dvec2 const& resolution, glm::dvec2 const& ndc) const
{
	glm::dmat4 projectionMatrix = calculateProjectionMatrix(resolution);
	glm::dmat4 viewMatrix = calculateViewMatrix();

	glm::dvec3 origin = glm::dvec3(glm::inverse(viewMatrix) * glm::dvec4(0.0, 0.0, 0.0, 1.0));
	glm::dvec3 far = glm::unProject(glm::dvec3(ndc.x, ndc.y, 1.0), viewMatrix, projectionMatrix, glm::dvec4(-1.0, -1.0, 2.0, 2.0));
	glm::dvec3 ray = glm::normalize(far - origin);
	return ray;
}

glm::dvec2 e2::RenderView::unprojectWorldPlane(glm::dvec2 const& resolution, glm::dvec2 const& xyCoords) const
{
	//if(glm::abs(xyCoords.x) != 1.0f && glm::abs(xyCoords.y) != 1.0f)
	//	LogNotice("mouse {}", xyCoords);
	glm::dmat4 projectionMatrix = calculateProjectionMatrix(resolution);
	glm::dmat4 viewMatrix = calculateViewMatrix();

	glm::dvec3 origin = glm::dvec3(glm::inverse(viewMatrix) * glm::dvec4(0.0, 0.0, 0.0, 1.0));

	//glm::vec3 near = glm::unProject<float>(glm::vec3(xyCoords.x, xyCoords.y, 0.0f), viewMatrix, projectionMatrix, glm::vec4(-1.0f, -1.0f, 2.0f, 2.0f));
	glm::dvec3 far = glm::unProject(glm::dvec3(xyCoords.x, xyCoords.y, 1.0), viewMatrix, projectionMatrix, glm::dvec4(-1.0, -1.0, 2.0, 2.0));
	glm::dvec3 viewRay = glm::normalize(far - origin);

	constexpr double renderDistance = 100.0;
	double rayDistanceToPlane = 0.0f;
	if (!glm::intersectRayPlane(origin, viewRay, { 0.0, 0.0, 0.0 }, e2::worldUp(), rayDistanceToPlane))
	{
		rayDistanceToPlane = renderDistance;
		//LogNotice("failed");
	}

	
	glm::dvec3 offset = origin + viewRay * (rayDistanceToPlane);
	return { offset.x, offset.z};
}

e2::Viewpoints2D const & e2::Renderer::viewpoints() const
{
	return m_viewPoints;
}

void e2::Renderer::debugLine(glm::vec3 const& color, glm::vec3 const& start, glm::vec3 const& end)
{
	m_debugLines.push_back({color, start, end });
}

void e2::Renderer::setEnvironment(e2::ITexture* irradiance, e2::ITexture* radiance)
{
	m_irradiance = irradiance;
	m_radiance = radiance;
}

void e2::Renderer::setSun(glm::vec3 const& dir, glm::vec3 const& color, float strength)
{
	m_sunColor = color;
	m_sunDirection = glm::normalize(dir);
	m_sunStrength = strength;
}

void e2::Renderer::swapRenderBuffers()
{
	m_backBuffer = frontBuffer();
}

uint8_t e2::Renderer::frontBuffer() const
{
	return m_backBuffer == 0 ? 1 : 0;
}

void e2::Renderer::debugLine(glm::vec3 const& color, glm::vec2 const& start, glm::vec2 const& end)
{
	debugLine(color, { start.x, 0.0f, start.y }, { end.x, 0.0f, end.y });
}

e2::Viewpoints2D::Viewpoints2D(glm::vec2 const& _resolution, e2::RenderView const& _view)
	: view(_view)
	, resolution(_resolution)
{
	topLeft = view.unprojectWorldPlane(resolution, { -1.0f, -1.0f });
	topRight = view.unprojectWorldPlane(resolution, { 1.0f, -1.0f });
	bottomRight = view.unprojectWorldPlane(resolution, { 1.0f,  1.0f });
	bottomLeft = view.unprojectWorldPlane(resolution, { -1.0f,  1.0f });

	calculateDerivatives();
}


e2::ConvexShape2D e2::Viewpoints2D::combine(Viewpoints2D const& other)
{
	e2::ConvexShape2D output;

	
	return output;
	/*

	// S is the set of points
	// P will be the set of points which form the convex hull. Final set size is i.

	algorithm jarvis(S) is

	pointOnHull = leftmost point in S // which is guaranteed to be part of the CH(S)
	i := 0
	repeat
		P[i] := pointOnHull
		endpoint := S[0]      // initial endpoint for a candidate edge on the hull
		for j from 0 to |S| do
			// endpoint == pointOnHull is a rare case and can happen only when j == 1 and a better endpoint has not yet been set for the loop
			if (endpoint == pointOnHull) or (S[j] is on left of line from P[i] to endpoint) then
				endpoint := S[j]   // found greater left turn, update endpoint
		i := i + 1
		pointOnHull = endpoint
	until endpoint = P[0]      // wrapped around to first hull point
	
	*/
}

e2::Viewpoints2D::Viewpoints2D()
{

}

bool e2::Viewpoints2D::isWithin(glm::vec2 const& point) const
{
	if (!leftRay.edgeTest(point))
		return false;
	if (!rightRay.edgeTest(point))
		return false;
	if (!topRay.edgeTest(point))
		return false;
	if (!bottomRay.edgeTest(point))
		return false;
	return true;
}

bool e2::Viewpoints2D::test(e2::Aabb2D const& aabb) const
{
	glm::vec2 aabbPoints[4] = { aabb.min, {aabb.max.x, aabb.min.y}, aabb.max, {aabb.min.x, aabb.max.y} };
	for (uint8_t i = 0; i < 4; i++)
	{
		if (isWithin(aabbPoints[i]))
			return true;
	}

	if (aabb.isWithin(topLeft))
		return true;
	if (aabb.isWithin(topRight))
		return true;
	if (aabb.isWithin(bottomLeft))
		return true;
	if (aabb.isWithin(bottomRight))
		return true;


	e2::Line2D aabbLines[4] = {
		{aabbPoints[0], aabbPoints[1]},
		{aabbPoints[1], aabbPoints[2]},
		{aabbPoints[2], aabbPoints[3]},
		{aabbPoints[3], aabbPoints[0]}
	};

	e2::Line2D viewLines[4] = {
		{topLeft, topRight},
		{topRight, bottomRight},
		{bottomRight, bottomLeft},
		{bottomLeft, topLeft}
	};

	for (int32_t x = 0; x < 4; x++)
		for (int32_t y = 0; y < 4; y++)
			if (viewLines[x].intersects(aabbLines[y]))
				return true;

	return false;
}

void e2::Viewpoints2D::calculateDerivatives()
{
	center = topLeft + topRight + bottomLeft + bottomRight / 4.0f;

	// first calculate positions and parallels, making sure parallel signs go clockwise around 
	leftRay.position = bottomLeft;
	leftRay.parallel = glm::normalize(topLeft - bottomLeft);
	leftRay.perpendicular = { leftRay.parallel.y,  -leftRay.parallel.x };

	topRay.position = topLeft;
	topRay.parallel = glm::normalize(topRight - topLeft);
	topRay.perpendicular = { topRay.parallel.y,  -topRay.parallel.x };

	rightRay.position = topRight;
	rightRay.parallel = glm::normalize(bottomRight - topRight);
	rightRay.perpendicular = { rightRay.parallel.y,  -rightRay.parallel.x };

	bottomRay.position = bottomRight;
	bottomRay.parallel = glm::normalize(bottomLeft - bottomRight);
	bottomRay.perpendicular = { bottomRay.parallel.y,  -bottomRay.parallel.x };
}

e2::Aabb2D e2::Viewpoints2D::toAabb() const
{
	constexpr float inf = std::numeric_limits<float>::infinity();

	e2::Aabb2D returner;
	returner.min = { inf, inf };
	returner.max = { -inf, -inf };

	for (uint8_t i = 0; i < 4; i++)
	{
		if (corners[i].x < returner.min.x)
			returner.min.x = corners[i].x;
		if (corners[i].y < returner.min.y)
			returner.min.y = corners[i].y;

		if (corners[i].x > returner.max.x)
			returner.max.x = corners[i].x;
		if (corners[i].y > returner.max.y)
			returner.max.y = corners[i].y;
	}

	return returner;
}

bool e2::isCounterClockwise(glm::vec2 const& a, glm::vec2 const& b, glm::vec2 const& c)
{
	return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
}

E2_API bool e2::boxRayIntersection2D(Aabb2D box, Ray2D ray)
{
	float tmin = -std::numeric_limits<float>::infinity();
	float tmax = std::numeric_limits<float>::infinity();

	if (ray.perpendicular.x != 0.0)
	{
		float tx1 = (box.min.x - ray.position.x) / ray.perpendicular.x;
		float tx2 = (box.max.x - ray.position.x) / ray.perpendicular.x;

		tmin = glm::max(tmin, glm::min(tx1, tx2));
		tmax = glm::min(tmax, glm::max(tx1, tx2));
	}

	if (ray.perpendicular.y != 0.0)
	{
		float ty1 = (box.min.y - ray.position.y) / ray.perpendicular.y;
		float ty2 = (box.max.y - ray.position.y) / ray.perpendicular.y;

		tmin = glm::max(tmin, glm::min(ty1, ty2));
		tmax = glm::min(tmax, glm::max(ty1, ty2));
	}

	return tmax >= tmin;
}

bool e2::Ray2D::edgeTest(glm::vec2 const& point) const
{
	return glm::dot(perpendicular, point - position) < 0.0f;
}

e2::StackVector<glm::vec2, 4> e2::Aabb2D::points()
{
	e2::StackVector<glm::vec2, 4> p = {
		min,
		{max.x, min.y},
		max,
		{min.x, max.y},
	};

	return p;
}

void e2::Aabb2D::push(glm::vec2 const& point)
{
	if (point.x < min.x)
		min.x = point.x;
	if (point.y < min.y)
		min.y = point.y;

	if (point.x > max.x)
		max.x = point.x;
	if (point.y > max.y)
		max.y = point.y;
}

bool e2::Aabb2D::isWithin(glm::vec2 const& point) const
{
	return point.x >= min.x && point.x < max.x && point.y >= min.y && point.y < max.y;
}

bool e2::Line2D::intersects(Line2D const& other)
{
	return e2::isCounterClockwise(start, other.start, other.end) != e2::isCounterClockwise(end, other.start, other.end)
		&& e2::isCounterClockwise(start, end, other.start) != e2::isCounterClockwise(start, end, other.end);
}
