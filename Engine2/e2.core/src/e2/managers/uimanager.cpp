
#include "e2/managers/uimanager.hpp"

#include "e2/e2.hpp"

#include "e2/ui/uicontext.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/rhi/databuffer.hpp"
#include "e2/rhi/texture.hpp"

//#include <windows.h>

// Quad
static char const* quad_vertexSource = R"SRC(
#version 460 core
in vec4 vertexPosition;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
};

void main()
{
	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * quadSize) + quadPosition;
	vertPos.xy = vertPos.xy * (1.0 / surfaceSize);
	vertPos.xy = vertPos.xy * 2.0 - 1.0;	

	// Apply the fixed Z
	vertPos.z = quadZ;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}
)SRC";


static char const* quad_fragSource = R"SRC(
#version 460 core

out vec4 outColor;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
};

void main()
{
	outColor = quadColor;
}
)SRC";



// Textured Quad
static char const* texturedQuad_vertexSource = R"SRC(
#version 460 core

#extension GL_EXT_nonuniform_qualifier : enable

in vec4 vertexPosition;
out vec2 fragmentUv;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	vec2 uvOffset;
	vec2 uvSize;
	float quadZ; // z index of the quad
	int textureIndex;
	int type;
};

layout(set = 0, binding = 0) uniform sampler quadSampler;
layout(set = 0, binding = 1) uniform texture2D quadTextures[];

void main()
{
	fragmentUv = vertexPosition.xy * uvSize + uvOffset;
	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * quadSize) + quadPosition;
	vertPos.xy = vertPos.xy * (1.0 / surfaceSize);
	vertPos.xy = vertPos.xy * 2.0 - 1.0;	

	// Apply the fixed Z
	vertPos.z = quadZ;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}
)SRC";


static char const* texturedQuad_fragSource = R"SRC(
#version 460 core

#extension GL_EXT_nonuniform_qualifier : enable

in vec2 fragmentUv;
out vec4 outColor;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	vec2 uvOffset;
	vec2 uvSize;
	float quadZ; // z index of the quad
	int textureIndex;
	int type;
};

layout(set = 0, binding = 0) uniform sampler quadSampler;
layout(set = 0, binding = 1) uniform texture2D quadTextures[];

void main()
{
	// original
	if(type == 0)
	{
		outColor = texture(sampler2D(quadTextures[textureIndex], quadSampler), fragmentUv) * quadColor;
	}
	// raster text 
	else if(type == 1)
	{
		outColor.rgb = quadColor.rgb;
		outColor.a = texture(sampler2D(quadTextures[textureIndex], quadSampler), fragmentUv).r * quadColor.a;
	}
	// sdf 
	else
	{
		float mask = texture(sampler2D(quadTextures[textureIndex], quadSampler), fragmentUv).r;

		float treshValue = 180.0;

		float treshCoeff = (treshValue / 255.0);
		float padding = 8.0;

		float smoothPixels = 2.0f;
		float pixelDistScale = treshValue / padding; 
		float glyphMapSize = 512.0;
		float smoothCoeff = (smoothPixels*pixelDistScale) / glyphMapSize;
		outColor.rgb = quadColor.rgb;
		outColor.a = smoothstep(treshCoeff - smoothCoeff, treshCoeff, mask) * quadColor.a;
	}
}
)SRC";







// Fancy Quad 
static char const* fancyQuad_vertexSource = R"SRC(
#version 460 core
in vec4 vertexPosition;
out vec2 vertexUv;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
	float cornerRadius;
	float bevelStrength;
	uint type;
	float pixelScale;
};

void main()
{
	vertexUv = vertexPosition.xy;

	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * quadSize) + quadPosition;
	vertPos.xy = vertPos.xy * (1.0 / surfaceSize);
	vertPos.xy = vertPos.xy * 2.0 - 1.0;	

	// Apply the fixed Z
	vertPos.z = quadZ;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}
)SRC";


static char const* fancyQuad_fragSource = R"SRC(
#version 460 core

in vec2 vertexUv;
out vec4 outColor;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
	float cornerRadius;
	float bevelStrength;
	uint type;
	float pixelScale;
};

float roundedMask( vec2 quadPosition, vec2 quadSize, float radius, float inset)
{
	// set alignment to 1.0 for a softer but less precise method when doing stacked outlines
	float alignment = 0.5;
	float iF =	(inset + alignment) * pixelScale;
	vec2 qpF = quadPosition + vec2(iF);
	vec2 qsF = quadSize - vec2(iF * 2.0);

	vec2 halfSize = qsF / 2.0;
	vec2 p = gl_FragCoord.xy - qpF - halfSize;// + vec2(0.5, 0.5);

	float actualRadius = radius - (inset * pixelScale);

    return 1.0 - clamp((length(max( abs(p) - halfSize + actualRadius,0.0)) - actualRadius), 0.0, 1.0);
}


void main()
{
	float a = 1.0;
	float b = 2.0;

	if(type == 0)
	{
		a = 0.5;
		b = 1.5;
	}

	float baseMask = roundedMask(quadPosition, quadSize, cornerRadius, 0.0);
	float innerMask1 = roundedMask(quadPosition, quadSize, cornerRadius, a);
	float innerMask2 = roundedMask(quadPosition, quadSize, cornerRadius, b);
	float bevelOuterCoeff = clamp(baseMask - innerMask1, 0.0, 1.0);
	float bevelInnerCoeff = clamp(innerMask1 - innerMask2 - bevelOuterCoeff, 0.0, 1.0);

	float innerOpacity = 0.75;
	float outerOpacity = 0.1;

	if(type == 0)
	{
		outerOpacity = 0.5;
		innerOpacity = 0.25;
	}

	vec4 baseColor = quadColor;
	// it may seem like we apply the opacities wrong, but thats incorrect
	// we want the outer bevel at low output opacity, as a simple soft outline
	// the same goes for inner, though this time for a subtle GI effect
	// it looks amazing!
	vec4 innerBevelColor = vec4(1.0, 1.0, 1.0, innerOpacity * bevelStrength);
	vec4 outerBevelColor = vec4(0.0, 0.0, 0.0, outerOpacity * bevelStrength);

	if(type == 0)
	{
		innerBevelColor = mix(baseColor, vec4(1.0, 1.0, 1.0, 1.0), bevelStrength * innerOpacity);
		//innerBevelColor = vec4(1.0, 0.0, 1.0, 1.0);
	}
		//vec4 innerBevelColor = vec4(1.0, 0.0, 1.0, 1.0);
	outColor = mix(outerBevelColor, innerBevelColor, innerMask1);
	outColor = mix(outColor, baseColor, innerMask2);

	//outColor = mix(baseColor, innerBevelColor, bevelInnerCoeff);
	//outColor = mix(outColor, outerBevelColor, bevelOuterCoeff);
	//if(type == 1)
		outColor.a = outColor.a * baseMask;
	//else 
	//	outColor.a = baseMask;
}
)SRC";


// Fancy Quad 
static char const* quadShadow_vertexSource = R"SRC(
#version 460 core
in vec4 vertexPosition;
out vec2 vertexUv;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
	float cornerRadius;
	float shadowStrength;
	float shadowSize;
};

void main()
{
	vertexUv = vertexPosition.xy;

	vec4 vertPos = vertexPosition;
	vertPos.xy = (vertPos.xy * quadSize) + quadPosition;
	vertPos.xy = vertPos.xy * (1.0 / surfaceSize);
	vertPos.xy = vertPos.xy * 2.0 - 1.0;	

	// Apply the fixed Z
	vertPos.z = quadZ;
	vertPos.w = 1.0;

	gl_Position = vertPos;
}
)SRC";


static char const* quadShadow_fragSource = R"SRC(
#version 460 core

in vec2 vertexUv;
out vec4 outColor;

layout(push_constant) uniform ConstantData
{
	vec4 quadColor; // quad color, rgba
	vec2 surfaceSize; // underlying surface size, in pixels
	vec2 quadPosition; // surface-relative quad position, in pixels
	vec2 quadSize; // quad size, in pixels
	float quadZ; // z index of the quad
	float cornerRadius;
	float shadowStrength;
	float shadowSize;
};

float shadowMask( vec2 quadPosition, vec2 quadSize, float radius)
{
	// set alignment to 1.0 for a softer but less precise method when doing stacked outlines
	float alignment = 0.5;
	float iF = alignment;
	vec2 qpF = quadPosition + vec2(iF);
	vec2 qsF = quadSize - vec2(iF * 2.0);

	vec2 halfSize = qsF / 2.0;
	vec2 p = gl_FragCoord.xy - qpF - halfSize;

    return (length(max( abs(p) - halfSize + radius,0.0)) - radius);
}


void main()
{
	outColor = quadColor;
	float scale = shadowSize;
	float baseMask = clamp(max(-shadowMask(quadPosition, quadSize, cornerRadius), 0.0) / scale, 0.0, 1.0);
	outColor.a = shadowStrength * baseMask;
}
)SRC";

e2::UIManager::UIManager(Engine* owner)
	: e2::Manager(owner)
{


}

e2::UIManager::~UIManager()
{

}

void e2::UIManager::onNewFrame(uint8_t frameIndex)
{

	//m_frameTextures.resize(0);
	m_frameTextureIndices.clear();
	m_frameTextureIndex = 0;

	for (e2::UIWindow* wnd : m_newQueue)
	{
		m_windows.insert(wnd);
	}
	m_newQueue.clear();

	for (e2::UIContext* ctx : m_contexts)
		ctx->newFrame();

}

void e2::UIManager::preDispatch(uint8_t frameIndex)
{

	for (e2::UIWindow* wnd : m_windows)
	{
		glm::uvec2 size = wnd->rhiWindow()->size();
		if (size.x > 0 && size.y > 0)
		{
			wnd->preSubmit();
		}
	}

	for(e2::UIContext* ctx : m_contexts)
		ctx->submitFrame();
}

void e2::UIManager::postDispatch(uint8_t frameIndex)
{
	for (e2::UIWindow* wnd : m_windows)
		wnd->rhiWindow()->present();

	for (e2::UIWindow* wnd : m_removeQueue)
	{
		m_windows.erase(wnd);
		e2::destroy(wnd);
	}

	m_removeQueue.clear();
}


void e2::UIManager::initialize()
{

	e2::UIVertex vertices[4] = {
	{{0.0f, 0.0f, 0.0f, 1.0f}},
	{{1.0f, 0.0f, 0.0f, 1.0f}},
	{{1.0f, 1.0f, 0.0f, 1.0f}},
	{{0.0f, 1.0f, 0.0f, 1.0f}},
	};

	e2::DataBufferCreateInfo bufferInfo{};
	bufferInfo.type = BufferType::VertexBuffer;
	bufferInfo.size = 4 * sizeof(e2::UIVertex);
	quadVertexBuffer = renderContext()->createDataBuffer(bufferInfo);
	quadVertexBuffer->upload(reinterpret_cast<uint8_t*>(vertices), 4 * sizeof(e2::UIVertex), 0, 0);


	uint32_t indices[6] = {
		0, 1, 2,
		0, 2, 3
	};

	bufferInfo.type = BufferType::IndexBuffer;
	bufferInfo.size = 6 * sizeof(uint32_t);
	quadIndexBuffer = renderContext()->createDataBuffer(bufferInfo);
	quadIndexBuffer->upload(reinterpret_cast<uint8_t*>(indices), 6 * sizeof(uint32_t), 0, 0);

	e2::VertexLayoutCreateInfo vertexLayoutInfo{};
	vertexLayoutInfo.attributes.push({0, e2::VertexFormat::Vec4, 0});
	vertexLayoutInfo.bindings.push({sizeof(glm::vec4), e2::VertexRate::PerVertex });
	quadVertexLayout = renderContext()->createVertexLayout(vertexLayoutInfo);

	// Quad 
	e2::ShaderCreateInfo shaderInfo{};
	shaderInfo.source = quad_vertexSource;
	shaderInfo.stage = ShaderStage::Vertex;
	quadPipeline.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.source = quad_fragSource;
	shaderInfo.stage = ShaderStage::Fragment;
	quadPipeline.fragmentShader = renderContext()->createShader(shaderInfo);

	e2::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.pushConstantSize = sizeof(UIQuadPushConstants);
	quadPipeline.layout = renderContext()->createPipelineLayout(layoutInfo);

	e2::PipelineCreateInfo pipelineInfo{};
	pipelineInfo.colorFormats.push(e2::TextureFormat::RGBA8);
	pipelineInfo.alphaBlending = true;
	pipelineInfo.depthFormat = TextureFormat::D16;
	pipelineInfo.layout = quadPipeline.layout;
	pipelineInfo.shaders.push(quadPipeline.vertexShader);
	pipelineInfo.shaders.push(quadPipeline.fragmentShader);
	quadPipeline.pipeline = renderContext()->createPipeline(pipelineInfo);

	// Textured Quad 
	shaderInfo.source = texturedQuad_vertexSource;
	shaderInfo.stage = ShaderStage::Vertex;
	m_texturedQuadPipeline.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.source = texturedQuad_fragSource;
	shaderInfo.stage = ShaderStage::Fragment;
	m_texturedQuadPipeline.fragmentShader = renderContext()->createShader(shaderInfo);

	e2::DescriptorSetLayoutCreateInfo setLayoutInfo{};
	setLayoutInfo.bindings.push({ e2::DescriptorBindingType::Sampler, 1 });
	setLayoutInfo.bindings.push({ e2::DescriptorBindingType::Texture, e2::maxConcurrentUITextures, true, true});
	m_texturedQuadSetLayout = renderContext()->createDescriptorSetLayout(setLayoutInfo);

	e2::SamplerCreateInfo samplerInfo{};
	samplerInfo.filter = SamplerFilter::Anisotropic;
	samplerInfo.wrap = SamplerWrap::Repeat;
	m_texturedQuadSampler = renderContext()->createSampler(samplerInfo);

	e2::DescriptorPoolCreateInfo poolInfo{};
	poolInfo.maxSets = 2;
	poolInfo.numTextures = e2::maxConcurrentUITextures * 2;
	poolInfo.numSamplers = 2;
	poolInfo.allowUpdateAfterBind = true;
	m_texturedQuadPool = mainThreadContext()->createDescriptorPool(poolInfo);
	m_texturedQuadSets[0] = m_texturedQuadPool->createDescriptorSet(m_texturedQuadSetLayout);
	m_texturedQuadSets[1] = m_texturedQuadPool->createDescriptorSet(m_texturedQuadSetLayout);

	layoutInfo.pushConstantSize = sizeof(UITexturedQuadPushConstants);
	layoutInfo.sets.push(m_texturedQuadSetLayout);
	m_texturedQuadPipeline.layout = renderContext()->createPipelineLayout(layoutInfo);
	layoutInfo.sets.clear();

	pipelineInfo.alphaBlending = true;
	pipelineInfo.layout = m_texturedQuadPipeline.layout;
	pipelineInfo.shaders.clear();
	pipelineInfo.shaders.push(m_texturedQuadPipeline.vertexShader);
	pipelineInfo.shaders.push(m_texturedQuadPipeline.fragmentShader);
	m_texturedQuadPipeline.pipeline = renderContext()->createPipeline(pipelineInfo);

	m_texturedQuadSets[0]->writeSampler(0, m_texturedQuadSampler);
	m_texturedQuadSets[1]->writeSampler(0, m_texturedQuadSampler);


	// Fancy Quad 
	shaderInfo.source = fancyQuad_vertexSource;
	shaderInfo.stage = ShaderStage::Vertex;
	m_fancyQuadPipeline.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.source = fancyQuad_fragSource;
	shaderInfo.stage = ShaderStage::Fragment;
	m_fancyQuadPipeline.fragmentShader = renderContext()->createShader(shaderInfo);

	layoutInfo.pushConstantSize = sizeof(UIFancyQuadPushConstants);
	m_fancyQuadPipeline.layout = renderContext()->createPipelineLayout(layoutInfo);

	pipelineInfo.alphaBlending = true;
	pipelineInfo.layout = m_fancyQuadPipeline.layout;
	pipelineInfo.shaders.clear();
	pipelineInfo.shaders.push(m_fancyQuadPipeline.vertexShader);
	pipelineInfo.shaders.push(m_fancyQuadPipeline.fragmentShader);
	m_fancyQuadPipeline.pipeline = renderContext()->createPipeline(pipelineInfo);




	// Quad Shadow
	shaderInfo.source = quadShadow_vertexSource;
	shaderInfo.stage = ShaderStage::Vertex;
	m_quadShadowPipeline.vertexShader = renderContext()->createShader(shaderInfo);

	shaderInfo.source = quadShadow_fragSource;
	shaderInfo.stage = ShaderStage::Fragment;
	m_quadShadowPipeline.fragmentShader = renderContext()->createShader(shaderInfo);

	layoutInfo.pushConstantSize = sizeof(UIQuadShadowPushConstants);
	m_quadShadowPipeline.layout = renderContext()->createPipelineLayout(layoutInfo);

	pipelineInfo.alphaBlending = true;
	pipelineInfo.layout = m_quadShadowPipeline.layout;
	pipelineInfo.shaders.clear();
	pipelineInfo.shaders.push(m_quadShadowPipeline.vertexShader);
	pipelineInfo.shaders.push(m_quadShadowPipeline.fragmentShader);
	m_quadShadowPipeline.pipeline = renderContext()->createPipeline(pipelineInfo);


	renderManager()->registerCallbacks(this);

}

void e2::UIManager::shutdown()
{
	std::unordered_set<e2::UIWindow*> tmpWnd = m_windows;
	for (e2::UIWindow* wnd : tmpWnd)
	{
		e2::destroy(wnd);
	}

	renderManager()->unregisterCallbacks(this);

	e2::destroy(m_texturedQuadSets[0]);
	e2::destroy(m_texturedQuadSets[1]);
	e2::destroy(m_texturedQuadPool);
	e2::destroy(m_texturedQuadSetLayout);
	e2::destroy(m_texturedQuadSampler);
	e2::destroy(m_texturedQuadPipeline.pipeline);
	e2::destroy(m_texturedQuadPipeline.fragmentShader);
	e2::destroy(m_texturedQuadPipeline.vertexShader);
	e2::destroy(m_texturedQuadPipeline.layout);


	e2::destroy(m_quadShadowPipeline.pipeline);
	e2::destroy(m_quadShadowPipeline.fragmentShader);
	e2::destroy(m_quadShadowPipeline.vertexShader);
	e2::destroy(m_quadShadowPipeline.layout);

	e2::destroy(m_fancyQuadPipeline.pipeline);
	e2::destroy(m_fancyQuadPipeline.fragmentShader);
	e2::destroy(m_fancyQuadPipeline.vertexShader);
	e2::destroy(m_fancyQuadPipeline.layout);

	e2::destroy(quadPipeline.pipeline);
	e2::destroy(quadPipeline.fragmentShader);
	e2::destroy(quadPipeline.vertexShader);
	e2::destroy(quadPipeline.layout);
	e2::destroy(quadVertexLayout);
	e2::destroy(quadIndexBuffer);
	e2::destroy(quadVertexBuffer);
}

void e2::UIManager::preUpdate(double deltaTime)
{

}

void e2::UIManager::update(double deltaTime)
{

	for (e2::UIWindow* wnd : m_windows)
	{
		glm::uvec2 size = wnd->rhiWindow()->size();
		if(size.x > 0 && size.y > 0)
			wnd->update(deltaTime); // update invokes buildUI by default
	}

}


uint32_t e2::UIManager::idFromTexture(e2::ITexture* tex)
{
	auto it = m_frameTextureIndices.find(tex);
	if (it != m_frameTextureIndices.end())
		return it->second;

	uint8_t frameIndex = renderManager()->frameIndex();

	//m_frameTextures.push(tex);
	m_texturedQuadSets[frameIndex]->writeTexture(uint32_t(1), tex, m_frameTextureIndex, 1);
	m_frameTextureIndices[tex] = m_frameTextureIndex;// m_frameTextures.size() - 1;
	return m_frameTextureIndex++;
}

void e2::UIManager::queueDestroy(e2::UIWindow* wnd)
{
	m_removeQueue.insert(wnd);
}

e2::UIWindow::UIWindow(e2::Context* ctx, UIWindowFlags flags)
	: m_engine(ctx->engine())
	, m_flags(flags)
{
	e2::WindowCreateInfo windowInfo{};
	windowInfo.customDecorations = true;
	windowInfo.resizable = flags & WF_Resizable;
	windowInfo.topMost = flags & WF_TopMost;
	windowInfo.tool = flags & WF_Transient;

	m_rhiWindow = mainThreadContext()->createWindow(windowInfo);
	m_uiContext = e2::create<e2::UIContext>(this, m_rhiWindow);

	uiManager()->m_newQueue.insert(this);
	m_rhiWindow->registerInputHandler(m_uiContext);
}

e2::UIWindow::~UIWindow()
{
	m_rhiWindow->unregisterInputHandler(m_uiContext);
	uiManager()->m_windows.erase(this);

	e2::destroy(m_uiContext);
	e2::destroy(m_rhiWindow);
}

e2::Engine* e2::UIWindow::engine()
{
	return m_engine;
}





void e2::UIWindow::update(double deltaTime)
{
	m_hasChangedCursor = false;

	bool hasTitlebar = (m_flags & WF_Titlebar);
	bool hasCloseButton = hasTitlebar && (m_flags & WF_Closeable);
	bool hasSystemMenu = hasTitlebar && (m_flags & WF_SystemMenu);
	bool isTransient = m_flags& WF_Transient;

	e2::UIStyle& style = uiManager()->workingStyle();
	e2::UIMouseState & mouseState = uiContext()->mouseState();
	e2::UIMouseButtonState & leftButton = mouseState.buttons[size_t(e2::MouseButton::Left)];

	float scaledRadius = style.windowCornerRadius * style.scale;
	if (isTransient)
	{
		scaledRadius = scaledRadius * 0.5f;
	}

	glm::vec2 scaledRadiusVec2 = glm::vec2(scaledRadius);
	float scaledMargin = scaledRadius / 4.0f;
	glm::vec2 scaledMarginVec2 = glm::vec2(scaledMargin);

	glm::vec2 shadowOffset = glm::vec2(8.0f) * style.scale;
	if (isTransient)
	{
		shadowOffset = shadowOffset * 0.5f;
	}

	glm::vec2 windowPosition = m_rhiWindow->position();
	glm::vec2 windowSize = glm::vec2(m_rhiWindow->size());

	// Calculate and set client area 
	bool maximized = m_rhiWindow->isMaximized();
	glm::vec2 clientOffset, clientSize;
	if (maximized )
	{
		clientOffset = scaledMarginVec2;
		clientSize = windowSize - scaledMarginVec2 * 2.0f;
	}
	else
	{
		clientOffset = scaledMarginVec2 * 2.0f;
		clientSize = windowSize - scaledMarginVec2 * 4.0f - shadowOffset;
	}

	glm::vec2 titlebarPosition = clientOffset;
	glm::vec2 titlebarSize = glm::vec2(clientSize.x, scaledRadius);
	
	if (hasTitlebar)
	{
		clientOffset.y += titlebarSize.y + scaledMargin;
		clientSize.y -= titlebarSize.y + scaledMargin;
	}


	uiContext()->setClientArea(clientOffset, clientSize);




	// Render window base
	if (!maximized)
	{
		float shadowStrength = 0.25f;
		float shadowSize = 16.0f;

		if (isTransient)
		{
			shadowSize = shadowSize * 0.25f;
			shadowStrength = 0.075f;
		}

		uiContext()->drawQuadShadow(shadowOffset, windowSize - shadowOffset, scaledRadius, shadowStrength, shadowSize * style.scale);
		uiContext()->drawQuadFancy(glm::vec2(0.0f), windowSize - shadowOffset, style.windowBgColor, scaledRadius, 1.0f, !isTransient);
	}
	else
	{
		uiContext()->drawQuad({}, windowSize, style.windowBgColor);
	}

	// debug client rect
	//uiContext()->drawQuad(clientOffset, clientSize, style.accents[UIAccent_Red]);


	{
		bool closeButtonHovered{};
		bool iconHovered{};

		// Close button behaviour and parameters
		if (hasCloseButton)
		{
			glm::vec2 closeButtonPosition = titlebarPosition + glm::vec2(titlebarSize.x, 4.0f * style.scale) - (glm::vec2(20.0f, 0.0f) - glm::vec2(4.f, 0.0f)) * style.scale;
			glm::vec2 closeButtonSize = glm::vec2(12.f) * style.scale;
			e2::UIColor closeButtonColor = style.windowBgColor;
			closeButtonHovered = e2::intersect(mouseState.relativePosition, closeButtonPosition, closeButtonSize);

			if (closeButtonHovered)
			{
				closeButtonColor = style.accents[UIAccent_Red];
			}

			uiContext()->drawQuadFancy(closeButtonPosition, closeButtonSize, closeButtonColor, 6.f * style.scale, 1.0f);

			if (closeButtonHovered && leftButton.clicked && !m_sizing && !m_moving)
			{
				m_rhiWindow->wantsClose(true);
			}
		}


		// Draw titlebar icon
		if (hasSystemMenu)
		{
			glm::vec2 iconPosition = titlebarPosition + glm::vec2(4.f) * style.scale;
			glm::vec2 iconSize = glm::vec2(12.f) * style.scale;
			e2::UIColor iconColor = style.windowBgColor;
			float iconBevel = 1.0f;
			iconHovered = e2::intersect(mouseState.relativePosition, iconPosition, iconSize);
			if (iconHovered)
			{
				iconColor = style.buttonColor;
				iconBevel = 1.0f;
			}
			uiContext()->drawQuadFancy(iconPosition, iconSize, iconColor, 6.f * style.scale, iconBevel);

			if (iconHovered && leftButton.clicked && !m_sizing && !m_moving)
			{
				onSystemButton();
			}
		}

		// Draw title text
		if (hasTitlebar)
		{
			glm::vec2 titlePosition = titlebarPosition + glm::vec2(25.0f * style.scale, titlebarSize.y / 2.0f);
			e2::EngineMetrics& m = engine()->metrics();
			std::string str = std::format("**e2 editor** ^m^5{:.1f}ms^- / ^5{:.1f}ms^- / ^7{:.1f}us^- / ^7{:.1f}us^-", m.frameTimeMsMean, m.frameTimeMsHigh, m.gpuWaitTimeUsMean, m.gpuWaitTimeUsHigh);
			//std::string str = std::format("**e2 editor**");
			uiContext()->drawRasterText(e2::FontFace::Sans, uint8_t(11 * style.scale), 0x000000FF, titlePosition, str);



			bool titlebarHovered = !iconHovered && !closeButtonHovered && e2::intersect(mouseState.relativePosition, titlebarPosition, titlebarSize);

			if (titlebarHovered && leftButton.doubleClicked && (m_flags & WF_Resizable))
			{
				if (m_rhiWindow->isMaximized())
					m_rhiWindow->restore();
				else
					m_rhiWindow->maximize();


				leftButton.ignorePress = true;
			}
			else
			{
				// Move behaviour
				if (leftButton.pressed && titlebarHovered)
				{
					m_wantsMove = true;
				}

				if (leftButton.released && m_wantsMove)
				{
					m_wantsMove = false;
				}

				if (m_wantsMove && leftButton.maxDragDistance > 4.0f)
				{
					if (maximized)
					{
						glm::vec2 oldPosition = m_rhiWindow->position();
						glm::vec2 relativeMouseOld = mouseState.relativePosition;

						m_rhiWindow->restore();

						glm::vec2 newSize = glm::vec2(m_rhiWindow->size());
						glm::vec2 newPosition = m_rhiWindow->position();
						glm::vec2 relativeMouseNew = mouseState.position - newPosition;


						float xOffset = relativeMouseOld.x;
						if (xOffset > newSize.x)
							xOffset = newSize.x / 2.0f;

						m_rhiWindow->position(mouseState.position - glm::vec2(xOffset, relativeMouseOld.y));
					}
					m_moving = true;
				}

				if (leftButton.released && m_moving)
				{
					m_moving = false;
				}

				if (leftButton.held && m_moving && mouseState.moved)
				{
					glm::vec2 newPosition = m_rhiWindow->position() + mouseState.moveDelta;
					m_rhiWindow->position(newPosition);
				}
			}
		}

	}

	if (!maximized && hasTitlebar && (m_flags & WF_Resizable))
	{
		glm::vec2 cornerSize = scaledRadiusVec2 / 2.0f;
		glm::vec2 cornerSizeX  { cornerSize.x, 0.0f };
		glm::vec2 cornerSizeY  { 0.0f, cornerSize.y};
		glm::vec2 clientOffsetX  {clientOffset.x, 0.0f};
		glm::vec2 clientOffsetY  { 0.0f, clientOffset.y};
		glm::vec2 clientSizeX {clientSize.x, 0.0f};
		glm::vec2 clientSizeY {0.0f, clientSize.y};
		windowResizer(UIResizerType::W, cornerSizeY, cornerSizeX + clientSizeY);
		windowResizer(UIResizerType::E, clientOffsetX + clientSizeX + cornerSizeY, cornerSizeX + clientSizeY);
		windowResizer(UIResizerType::SE, clientOffset + clientSize, cornerSize);
		windowResizer(UIResizerType::SW, clientOffsetY + clientSizeY, cornerSize);

		windowResizer(UIResizerType::N, cornerSizeX, clientSizeX + cornerSizeY);
		windowResizer(UIResizerType::S, cornerSizeX + clientOffsetY + clientSizeY, clientSizeX + cornerSizeY);

		windowResizer(UIResizerType::NW, {}, cornerSize);
		windowResizer(UIResizerType::NE, clientOffsetX + clientSizeX, cornerSize);
	}

	if (m_flags & WF_Transient)
	{
		if (!m_rhiWindow->isFocused())
		{
			destroy();
		}

	}
}

void e2::UIWindow::onSystemButton()
{

}

void e2::UIWindow::cursor(e2::CursorShape newCursor)
{
	m_rhiWindow->cursor(newCursor);
	m_hasChangedCursor = true;
}

e2::IWindow* e2::UIWindow::rhiWindow()
{
	return m_rhiWindow;
}

e2::UIContext* e2::UIWindow::uiContext()
{
	return m_uiContext;
}

void e2::UIWindow::preSubmit()
{
	// int convert to get rid of fractions
	glm::vec2 rhiSize = m_rhiWindow->size();
	glm::vec2 clientSize = m_uiContext->baseState().size;
	glm::vec2 usedClientSize = m_uiContext->renderedSize();
	glm::vec2 padding = rhiSize - clientSize;

	m_minimumSize = glm::ivec2(usedClientSize + padding) + glm::ivec2(1, 1);


	if (m_flags & WF_ScaleToFit)
	{

		if (rhiSize != m_minimumSize)
		{
			m_rhiWindow->size(m_minimumSize);
		}

	}

	if (!m_hasChangedCursor)
	{
		m_rhiWindow->cursor(e2::CursorShape::Default);
	} 
}

void e2::UIWindow::destroy()
{
	uiManager()->queueDestroy(this);
}

void e2::UIWindow::windowResizer(UIResizerType type, glm::vec2 position, glm::vec2 size)
{
	e2::UIStyle& style = uiManager()->workingStyle();
	e2::UIMouseState const& mouseState = uiContext()->mouseState();
	e2::UIMouseButtonState const& leftButton = mouseState.buttons[size_t(e2::MouseButton::Left)];

	constexpr glm::vec2 shadowOffset = glm::vec2(8.0f);
	glm::vec2 windowPosition = m_rhiWindow->position();
	glm::vec2 windowSize = glm::vec2(m_rhiWindow->size());

	const e2::CursorShape shapes[9] = {
		e2::CursorShape::Default, // None,
		e2::CursorShape::ResizeNWSE, // NW,
		e2::CursorShape::ResizeNS, // N,
		e2::CursorShape::ResizeNESW,// NE,
		e2::CursorShape::ResizeWE,// E,
		e2::CursorShape::ResizeNWSE,// SE,
		e2::CursorShape::ResizeNS,// S,
		e2::CursorShape::ResizeNESW,// SW,
		e2::CursorShape::ResizeWE// W
	};

	bool hovered = e2::intersect(mouseState.relativePosition, position, size);

	if (!m_wantsSize && hovered && leftButton.pressed)
	{
		uiContext()->unsetActive();
		m_wantsSize = true;
		m_sizingType = type;
	}

	// bail out if another sizer is working 
	if (m_wantsSize && m_sizingType != type)
	{
		return;
	}

	if (leftButton.released && m_wantsSize)
	{
		m_wantsSize = false;
	}

	if (m_wantsSize && leftButton.maxDragDistance > 4.0f)
	{
		m_sizing = true;
	}

	if (leftButton.released && m_sizing)
	{
		m_sizing = false;
	}

	glm::vec2 moveModifier{0.0f, 0.0f};
	glm::vec2 sizeModifier{0.0f, 0.0f};

	switch (type)
	{
	case UIResizerType::NW:
		sizeModifier = { -1.0f, -1.0f};
		moveModifier = {1.0f, 1.0f};
		break;
	case UIResizerType::N:
		sizeModifier = { 0.0f, -1.0f };
		moveModifier = { 0.0f, 1.0f };
		break;
	case UIResizerType::NE:
		sizeModifier = { 1.0f, -1.0f };
		moveModifier = { 0.0f, 1.0f };
		break;
	case UIResizerType::E:
		sizeModifier = { 1.0f, 0.0f };
		break;
	default:
	case UIResizerType::SE:
		sizeModifier = { 1.0f, 1.0f };
		break;
	case UIResizerType::S:
		sizeModifier = { 0.0f, 1.0f };
		break;
	case UIResizerType::SW:
		sizeModifier = { -1.0f, 1.0f };
		moveModifier = { 1.0f, 0.0f };
		break;
	case UIResizerType::W:
		sizeModifier = { -1.0f, 0.0f };
		moveModifier = { 1.0f, 0.0f };
		break;
	}

	auto applyNewParams = [this, &mouseState, &moveModifier, &sizeModifier]() {

		glm::vec2 currentSize = glm::vec2(m_rhiWindow->size());
		glm::vec2 newSize = currentSize + (mouseState.moveDelta * sizeModifier);
		newSize.x = glm::max(newSize.x, 200.0f);
		newSize.y = glm::max(newSize.y, 128.0f);

		if (e2::nearlyEqual(newSize.x, currentSize.x, 0.4999f))
			moveModifier.x = 0.0f;

		if (e2::nearlyEqual(newSize.y, currentSize.y, 0.4999f))
			moveModifier.y = 0.0f;

		m_rhiWindow->size(newSize);

		glm::vec2 newPosition = m_rhiWindow->position() + (mouseState.moveDelta * moveModifier);
		m_rhiWindow->position(newPosition);
	};

	if (leftButton.held && m_sizing && mouseState.moved)
	{
		applyNewParams();
	}

	if (hovered)
	{
		cursor(shapes[size_t(type)]);
		//uiContext()->drawQuadFancy(position, size, 0xFF0000FF, 0.1f, 1.0f);
	}
}
