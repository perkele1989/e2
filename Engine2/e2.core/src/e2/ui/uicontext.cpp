
#include "e2/ui/uicontext.hpp"

#include "e2/managers/rendermanager.hpp"
#include "e2/managers/uimanager.hpp"

#include <chrono>

namespace
{
	void UIContext_getSizeForNext(e2::UIRenderState &state, glm::vec2 const& minSize, glm::vec2 &outOffset, glm::vec2 &outSize)
	{
		outSize = state.size;
		outOffset = state.cursor;
	}
}

e2::UIContext::UIContext(e2::Context* ctx, e2::IWindow* win)
	: m_engine(ctx->engine())
	, m_window(win)
	, m_dataArena(e2::maxNumUIWidgetData)
{
	m_clearList.reserve(1024);

	// Setup command buffers 
	e2::CommandBufferCreateInfo commandBufferInfo{};
	m_commandBuffers[0] = renderManager()->framePool(0)->createBuffer(commandBufferInfo);
	m_commandBuffers[1] = renderManager()->framePool(1)->createBuffer(commandBufferInfo);

	uiManager()->m_contexts.insert(this);

	resize(glm::uvec2(64, 64));


	e2::UIRenderState baseState;
	baseState.getSizeForNext = &::UIContext_getSizeForNext;
	m_renderStates.push(baseState);

	//m_pipelineSettings.depthTest = false;
	//m_pipelineSettings.depthWrite = false;
}

e2::UIContext::~UIContext()
{
	renderContext()->waitIdle();

	if (m_renderTarget)
		e2::destroy(m_renderTarget);

	if (m_colorTexture)
		e2::destroy(m_colorTexture);

	if (m_depthTexture)
		e2::destroy(m_depthTexture);

	e2::destroy(m_commandBuffers[0]);
	e2::destroy(m_commandBuffers[1]);

	uiManager()->m_contexts.erase(this);
}

e2::Engine* e2::UIContext::engine()
{
	return m_engine;
}

void e2::UIContext::onKeyDown(Key key)
{
	size_t index = size_t(key);
	if (index > m_keyboardState.keys.size())
	{
		return;
	}

	m_keyboardState.keys[index].bufferedState = true;
}

void e2::UIContext::onKeyUp(Key key)
{
	size_t index = size_t(key);
	if (index > m_keyboardState.keys.size())
	{
		return;
	}

	m_keyboardState.keys[index].bufferedState = false;
}

void e2::UIContext::onMouseScroll(float scrollOffset)
{
	m_mouseState.bufferedScroll += scrollOffset;
}

void e2::UIContext::onMouseMove(glm::vec2 position)
{
	m_mouseState.bufferedPosition = position;
}

void e2::UIContext::onMouseDown(e2::MouseButton button)
{
	m_mouseState.buttons[size_t(button)].bufferedState = true;
}

void e2::UIContext::onMouseUp(e2::MouseButton button)
{
	m_mouseState.buttons[size_t(button)].bufferedState = false;
}

void e2::UIContext::onDrop(int32_t pathCount, char const* paths[])
{
	for (int32_t i = 0; i < pathCount; i++)
	{
		m_mouseState.bufferedDrops.push_back(paths[i]);
	}
}

void e2::UIContext::newFrame()
{

	
	glm::uvec2 rhiSize = m_window->size();
	bool rhiValid = rhiSize.x > 0 && rhiSize.y > 0;
	if (size() != rhiSize && rhiValid)
	{
		resize(m_window->size());
		m_window->source(colorTexture());
	}

	//if (!rhiValid)
	//	return;


	constexpr double dblClickDuration = 0.3;

	m_renderedSize = glm::vec2(0.0f);

	bool focused = m_window->isFocused();
	
	m_mouseState.drops.swap(m_mouseState.bufferedDrops);
	m_mouseState.bufferedDrops.clear();

	m_mouseState.scrollOffset = m_mouseState.bufferedScroll;
	m_mouseState.bufferedScroll = 0.0f;

	m_mouseState.lastPosition = m_mouseState.position;
	m_mouseState.position = m_mouseState.bufferedPosition;
	m_mouseState.moveDelta = m_mouseState.position - m_mouseState.lastPosition;
	m_mouseState.moved = glm::length(m_mouseState.moveDelta) > 0.001f;

	m_mouseState.relativePosition = m_mouseState.position - m_window->position();
	//LogNotice("mousePos: {}", m_mouseState.position);

	// update mouse state
	for (e2::UIMouseButtonState& buttonState : m_mouseState.buttons)
	{
		// states
		buttonState.lastState = buttonState.state;
		buttonState.state = buttonState.bufferedState;

		// derivatives
		buttonState.pressed = focused && buttonState.state && !buttonState.lastState && !buttonState.ignorePress;
		buttonState.released = focused && !buttonState.state && buttonState.lastState;
		buttonState.held = focused && buttonState.state && buttonState.lastState && !buttonState.ignorePress;

		if (buttonState.ignorePress && buttonState.released)
		{
			buttonState.ignorePress = false;
			buttonState.released = false;
		}

		buttonState.clicked = focused && buttonState.released;

		double timeLastPressed = buttonState.timeLastPressed.durationSince().seconds();

		buttonState.doubleClicked = focused && buttonState.pressed && timeLastPressed < dblClickDuration;

		if (buttonState.doubleClicked)
		{
			buttonState.timeLastPressed = e2::Moment::epoch();
			buttonState.pressed = false;
			//buttonState.ignorePress = true;
		}

		if (!buttonState.doubleClicked)
		{
			if (buttonState.pressed)
			{
				buttonState.pressedPosition = m_mouseState.position;
				buttonState.maxDragDistance = 0.0f;

				buttonState.timeLastPressed = e2::timeNow();
			}
		}

		if (buttonState.held)
		{

			buttonState.dragOffset = m_mouseState.position - buttonState.pressedPosition;
			buttonState.dragDistance = glm::length(buttonState.dragOffset);
			if (buttonState.dragDistance > buttonState.maxDragDistance)
			{
				buttonState.maxDragDistance = buttonState.dragDistance;
			}

			if (buttonState.maxDragDistance > 1.0f)
			{
				buttonState.timeLastPressed = e2::Moment::epoch();
			}
		}
		
	}
	for (e2::UIKeyState& keyState : m_keyboardState.keys)
	{
		// states
		keyState.lastState = keyState.state;
		keyState.state = keyState.bufferedState;

		// derivatives
		keyState.pressed = focused && keyState.state && !keyState.lastState;
		keyState.released = focused && !keyState.state && keyState.lastState;
	}
	


	m_currentZ = 1.0f;
	m_hasRecordedData = false;
	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	
	buff->beginRecord(true, m_pipelineSettings);
	buff->beginRender(m_renderTarget);

	m_inFrame = true;

}

void e2::UIContext::submitFrame()
{

	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];

	if (m_inFrame)
	{
		buff->endRender();
		buff->endRecord();
	}
	m_inFrame = false;

	// No need to queue if we didnt record
	if(m_hasRecordedData)
		renderManager()->queue(buff, nullptr, nullptr);

	// end of frame submission: purge old data
	m_clearList.clear();
	for (std::pair<size_t const, e2::UIWidgetState*>& pair : m_dataMap)
	{
		size_t widgetHash = pair.first;
		e2::UIWidgetState* widgetData = pair.second;
		if (++widgetData->framesSinceActive >= e2::uiRetainFrameCount)
		{
			m_dataArena.destroy(widgetData);
			m_clearList.push_back(widgetHash);
		}
	}

	for (size_t id : m_clearList)
	{
		m_dataMap.erase(id);
	}
}


void e2::UIContext::setClientArea(glm::vec2 const& offset, glm::vec2 const& size)
{
	m_renderStates[0].offset = offset;
	m_renderStates[0].size = size;
	m_renderStates[0].cursor = offset;
}


void e2::UIContext::gameLabel(std::string const& text, uint8_t fontSize /*= 12*/, e2::UITextAlign horizAlign /*= UITextAlign::Begin*/)
{
	e2::UIStyle const& style = uiManager()->workingStyle();
	float textWidth = calculateTextWidth(FontFace::Serif, uint8_t(fontSize * style.scale), text);

	static const e2::Name n = "";

	glm::vec2 minSize = glm::vec2(0.0f, 20.0f * style.scale);
	e2::UIWidgetState* widgetState = reserve(n, minSize);

	if (minSize.x > widgetState->size.x || minSize.y > widgetState->size.y)
		return;

	float xOffset = 0.0f;
	if (horizAlign == UITextAlign::Middle)
	{
		xOffset -= textWidth / 2.0f;
		xOffset += widgetState->size.x / 2.0f;
	}
	else if (horizAlign == UITextAlign::End)
	{
		xOffset -= textWidth;
		xOffset += widgetState->size.x;
	}

	// Y offset is always centered
	//drawQuad(widgetState->position, widgetState->size, 0xFF00000F);
	//drawRasterTextShadow(FontFace::Serif, uint8_t(fontSize * style.scale), widgetState->position + glm::vec2(xOffset, widgetState->size.y / 2.0), text);
	drawRasterText(FontFace::Serif, uint8_t(fontSize * style.scale), 0xFFFFFFFF, widgetState->position + glm::vec2(xOffset, widgetState->size.y / 2.0), text);

}

void e2::UIContext::log(e2::Name id, float& scrollOffset, bool autoscroll)
{
	e2::UIStyle const& style = uiManager()->workingStyle();
	const float lineHeight = 20.0f * style.scale;
	glm::vec2 minSize = glm::vec2(0.0f, 0.0f);
	e2::UIWidgetState* widgetState = reserve(id, minSize);

	if(widgetState->hovered && glm::abs(m_mouseState.scrollOffset) > 0.0f)
		scrollOffset += -m_mouseState.scrollOffset * lineHeight;

	if (scrollOffset < 0.0f)
		scrollOffset = 0.0f;

	float numberOfLines = widgetState->size.y / lineHeight;

	std::scoped_lock lock(e2::Log::getMutex());
	uint32_t numEntries = e2::Log::numEntries();


	float maxScroll =glm::max(0.0f, ((float(numEntries) - numberOfLines) + 2.0f) * lineHeight);
	if (scrollOffset > maxScroll || autoscroll)
		scrollOffset = maxScroll;

	float lineStart = scrollOffset / lineHeight;
	uint32_t lineStartI = int32_t(lineStart);
	uint32_t numberOfLinesI = int32_t(numberOfLines);



	drawQuad(widgetState->position, widgetState->size, style.dark1);

	glm::vec2 cursor = widgetState->position;
	for (uint32_t i = lineStartI; i < lineStartI + numberOfLinesI; i++)
	{
		
		if (i < numEntries)
		{
			e2::LogEntry const&entry =e2::Log::getEntry(i);

			std::string svr;
			if (entry.severity == Severity::Notice)
				svr = "^7";
			else if (entry.severity == Severity::Warning)
				svr = "^2";
			else if (entry.severity == Severity::Error)
				svr = "^4";

			std::string msg = std::format("**{}{}:{}:^0** {}", svr, entry.function, entry.line, entry.message);

			drawRasterText(e2::FontFace::Monospace, uint8_t(12.f * style.scale), style.bright1, cursor + glm::vec2(4.0f * style.scale, lineHeight * 0.5f), msg);
		}
		cursor.y += lineHeight;
		
	}

}

void e2::UIContext::label(e2::Name id, std::string const& text, uint8_t fontSize, e2::UITextAlign horizAlign)
{
	e2::UIStyle const& style = uiManager()->workingStyle();

	float textWidth = calculateTextWidth(FontFace::Serif, uint8_t(fontSize * style.scale), text);

	glm::vec2 minSize = glm::vec2(0.0f, 20.0f * style.scale);
	e2::UIWidgetState* widgetState = reserve(id, minSize);

	if (minSize.x > widgetState->size.x || minSize.y > widgetState->size.y)
		return;

	float xOffset = 0.0f;
	if (horizAlign == UITextAlign::Middle)
	{
		xOffset -= textWidth / 2.0f;
		xOffset += widgetState->size.x / 2.0f;
	}
	else if (horizAlign == UITextAlign::End)
	{
		xOffset -= textWidth;
		xOffset += widgetState->size.x;
	}

	// Y offset is always centered
	//drawQuad(widgetState->position, widgetState->size, 0xFF00000F);
	//drawRasterTextShadow(FontFace::Serif, uint8_t(fontSize * style.scale), widgetState->position + glm::vec2(xOffset, widgetState->size.y / 2.0), text);
	drawRasterText(FontFace::Serif, uint8_t(fontSize * style.scale), style.windowFgColor, widgetState->position + glm::vec2(xOffset, widgetState->size.y / 2.0), text);
	
}

bool e2::UIContext::button(e2::Name id, std::string const& title)
{
	e2::UIStyle const& style = uiManager()->workingStyle();

	float textWidth = calculateTextWidth(FontFace::Sans, uint8_t(11.f * style.scale), title);
	float xOffset = -(textWidth / 2.0f);

	glm::vec2 minSize = glm::vec2 (textWidth + 8.0f * style.scale, 20.0f * style.scale);
	e2::UIWidgetState* widgetState = reserve(id, minSize);

	if (minSize.x > widgetState->size.x || minSize.y > widgetState->size.y)
		return false;

	bool clicked = widgetState->active && widgetState->hovered && m_mouseState.buttons[0].clicked;
	if (m_mouseState.buttons[0].held && widgetState->active && widgetState->hovered)
	{
		drawQuadFancy(widgetState->position , widgetState->size, style.accents[UIAccent_Blue], 4.f * style.scale, 0.0f);
		drawRasterText(FontFace::Sans, uint8_t(11 * style.scale), style.bright1, widgetState->position + glm::vec2(widgetState->size.x / 2.0f + xOffset, widgetState->size.y / 2.0), title);
	}
	else
	{
		drawQuadFancy(widgetState->position, widgetState->size, style.buttonColor, 4.f * style.scale, 1.0f);
		drawRasterText(FontFace::Sans, uint8_t(11 * style.scale), style.buttonTextColor, widgetState->position + glm::vec2(widgetState->size.x / 2.0f + xOffset, widgetState->size.y / 2.0), title);
	}
	

	return clicked;
}

bool e2::UIContext::checkbox(e2::Name id, bool& value, std::string const& text)
{
	e2::UIStyle const& style = uiManager()->workingStyle();

	float textWidth = calculateTextWidth(FontFace::Sans, uint8_t(12.f * style.scale), text);
	float padding = 2.0f * style.scale;
	float innerOffsetX = 2.0f * style.scale;
	glm::vec2 sizeOuter = glm::vec2(28.0f, 16.0f) * style.scale;
	glm::vec2 sizeInner = glm::vec2(12.0f, 12.0f) * style.scale;

	const glm::vec2 paddingVec2 = glm::vec2(padding);
	const glm::vec2 paddingDblVec2 = glm::vec2(padding*2.0f);
	const glm::vec2 innerOffset = glm::vec2(innerOffsetX, (sizeOuter.y - sizeInner.y) / 2.0f);
	const glm::vec2 innerOffset2 = glm::vec2(sizeOuter.x - sizeInner.x - innerOffsetX, (sizeOuter.y - sizeInner.y) / 2.0f);
	const float innerRadius = sizeInner.y / 2.0f;
	const float outerRadius = (sizeOuter.y - 1.0f) / 2.0f;
	
	glm::vec2 minSize(padding + sizeOuter.x + padding + padding + textWidth, padding + sizeOuter.y + padding);


	e2::UIWidgetState* widgetState = reserve(id, minSize);

	if (minSize.x > widgetState->size.x || minSize.y > widgetState->size.y)
		return false;

	if (value)
	{
		drawQuadFancy(widgetState->position + paddingVec2, sizeOuter, style.accents[UIAccent_Blue], outerRadius, 1.0f);
		drawQuadFancy(widgetState->position + paddingVec2 + innerOffset2, sizeInner, style.buttonColor, innerRadius, 0.1f);
	}
	else
	{
		drawQuadFancy(widgetState->position + paddingVec2, sizeOuter, style.windowBgColorInactive, outerRadius, 1.0f);
		drawQuadFancy(widgetState->position + paddingVec2 + innerOffset, sizeInner, style.windowBgColor, innerRadius, 0.1f);
	}



	drawRasterText(FontFace::Sans, uint8_t(12 * style.scale), style.windowFgColor, widgetState->position + glm::vec2(padding + sizeOuter.x + padding + padding, widgetState->size.y / 2.0), text);


	bool clicked = widgetState->hovered && widgetState->active && m_mouseState.buttons[0].clicked;
	if (clicked)
	{
		value = !value;
		return true;
	}

	return false;
}


bool e2::UIContext::radio(e2::Name id, uint32_t currentIndex, uint32_t& groupValue, std::string const& text)
{

	e2::UIStyle const& style = uiManager()->workingStyle();


	float textWidth = calculateTextWidth(FontFace::Sans, uint8_t(12.f * style.scale), text);
	float padding = 4.0f * style.scale;

	glm::vec2 sizeOuter = glm::vec2(12.0f, 12.0f) * style.scale;
	glm::vec2 sizeInner = glm::vec2(6.0f, 6.0f) * style.scale;

	glm::vec2 innerOffset = (sizeOuter / 2.0f - sizeInner / 2.0f);

	const glm::vec2 paddingVec2 = glm::vec2(padding);
	const glm::vec2 paddingDblVec2 = glm::vec2(padding * 2.0f);
	const float innerRadius = sizeInner.y / 2.0f;
	const float outerRadius = sizeOuter.y / 2.0f;

	glm::vec2 minSize(padding + sizeOuter.x + padding + padding + textWidth, padding + sizeOuter.y + padding);

	e2::UIWidgetState* widgetState = reserve(id, minSize);

	if (minSize.x > widgetState->size.x || minSize.y > widgetState->size.y)
		return false;

	bool thisActive = currentIndex == groupValue;

	if (thisActive)
	{
		drawQuadFancy(widgetState->position + paddingVec2, sizeOuter, style.accents[UIAccent_Blue], outerRadius, 1.0f);
		drawQuadFancy(widgetState->position + paddingVec2 + innerOffset, sizeInner, style.buttonColor, innerRadius, 0.1f);
	}
	else
	{
		drawQuadFancy(widgetState->position + paddingVec2, sizeOuter, style.windowBgColorInactive, outerRadius, 1.0f);
		drawQuadFancy(widgetState->position + paddingVec2 + innerOffset, sizeInner, style.windowBgColor, innerRadius, 0.1f);
	}


	drawRasterText(FontFace::Sans, uint8_t(12 * style.scale), style.windowFgColor, widgetState->position + glm::vec2(padding + sizeOuter.x + padding + padding, widgetState->size.y / 2.0), text);

	bool clicked = widgetState->active && widgetState->hovered && m_mouseState.buttons[0].clicked;
	if (clicked)
	{
		groupValue = currentIndex;
		return true;
	}

	return false;
}

bool e2::UIContext::inputText(e2::Name id, std::string& buffer)
{
	constexpr glm::vec2 minSize(100.0f, 20.0f);
	e2::UIWidgetState* widgetState = reserve(id, minSize);
	return false;
}

bool e2::UIContext::sliderInt(e2::Name id, int32_t& value, int32_t min, int32_t max, char const* format /*= "%d"*/)
{
	constexpr glm::vec2 minSize(100.0f, 20.0f);
	e2::UIWidgetState* widgetState = reserve(id, minSize);
	return false;
}

bool e2::UIContext::sliderFloat(e2::Name id, float& value, float min, float max, char const* format /*= "%.3f"*/)
{
	e2::UIStyle const& style = uiManager()->workingStyle();

	constexpr glm::vec2 minSize(200.0f, 20.0f);
	e2::UIWidgetState* widgetState = reserve(id, minSize);


	bool mouseDown = widgetState->active&& widgetState->hovered&& m_mouseState.buttons[0].held;

	drawQuad(widgetState->position, widgetState->size, style.windowFgColor);

	float mouseX = (m_mouseState.relativePosition - widgetState->position).x;
	float mouseNormalized = mouseX / widgetState->size.x;

	if (mouseDown)
	{
		value = glm::clamp(mouseNormalized * (max - min), min, max);
	}

	float normalized = (value - min) / (max - min);
	drawQuad(widgetState->position + glm::vec2(1.0f, 1.0f), glm::vec2( (widgetState->size.x * normalized) - 2.0f, widgetState->size.y - 2.0f ), style.accents[0]);

	drawRasterText(e2::FontFace::Serif, 9, style.windowBgColorInactive, widgetState->position + glm::vec2(0.0f, widgetState->size.y / 2.0f), id.string());

	std::string valueStr = std::format("{:.2f}", value);
	float textWidth = calculateTextWidth(e2::FontFace::Monospace, 9, valueStr);
	drawRasterText(e2::FontFace::Monospace, 9, style.windowBgColor, widgetState->position + glm::vec2(widgetState->size.x, widgetState->size.y / 2.0f) - glm::vec2(textWidth, 0.0f), valueStr);
	return false;
}



void e2::UIContext::clearScissor()
{
	setScissor({}, glm::vec2(m_renderTargetSize));
}

void e2::UIContext::setScissor(glm::vec2 position, glm::vec2 size)
{
	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	buff->setScissor(position, size);
}

void e2::UIContext::drawQuad(glm::vec2 position, glm::vec2 size, e2::UIColor color)
{
	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	e2::UIManager* ui = uiManager();

	constexpr float epsilon = 0.0001f;
	m_currentZ -= epsilon;

	e2::UIQuadPushConstants pushConstants{};
	pushConstants.quadColor = color.toVec4();
	pushConstants.quadPosition = position;
	pushConstants.quadSize = size;
	pushConstants.quadZ = m_currentZ;
	pushConstants.surfaceSize = glm::vec2(m_renderTargetSize);


	// @todo optimize this by queue'ing quads and then on submit render them. 
	// profile first so we know its nececssary
	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->quadPipeline.pipeline);
	buff->pushConstants(ui->quadPipeline.layout, 0, sizeof(e2::UIQuadPushConstants), reinterpret_cast<uint8_t*>(&pushConstants));

	buff->draw(6, 1);

	m_hasRecordedData = true;

	
	
}

void e2::UIContext::drawTexturedQuad(glm::vec2 position, glm::vec2 size, e2::UIColor color, e2::ITexture* texture, glm::vec2 uvOffset /*= { 0.0f, 0.0f }*/, glm::vec2 uvScale /*= {1.0f, 1.0f}*/, e2::UITexturedQuadType type)
{
	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	e2::UIManager* ui = uiManager();

	constexpr float epsilon = 0.0001f;
	m_currentZ -= epsilon;

	position = glm::ivec2(position);

	e2::UITexturedQuadPushConstants pushConstants{};
	pushConstants.quadColor = color.toVec4();
	pushConstants.quadPosition = position;
	pushConstants.quadSize = size;
	pushConstants.quadZ = m_currentZ;
	pushConstants.surfaceSize = glm::vec2(m_renderTargetSize);
	pushConstants.textureIndex = ui->idFromTexture(texture);
	pushConstants.uvOffset = uvOffset;
	pushConstants.uvSize = uvScale;
	pushConstants.type = uint32_t(type);


	// @todo optimize this by queue'ing quads and then on submit render them. 
	// profile first so we know its nececssary
	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->m_texturedQuadPipeline.pipeline);
	buff->bindDescriptorSet(ui->m_texturedQuadPipeline.layout, 0, ui->m_texturedQuadSets[frameIndex]);
	buff->pushConstants(ui->m_texturedQuadPipeline.layout, 0, sizeof(e2::UITexturedQuadPushConstants), reinterpret_cast<uint8_t*>(&pushConstants));

	buff->draw(6, 1);

	m_hasRecordedData = true;
}

void e2::UIContext::drawSprite(glm::vec2 position, e2::Sprite sprite)
{

}

void e2::UIContext::drawQuadFancy(glm::vec2 position, glm::vec2 size, e2::UIColor color, float cornerRadius, float bevelStrength, bool windowBorder)
{

	position.x = glm::floor(position.x);
	position.y = glm::floor(position.y);

	size.x = glm::ceil(size.x);
	size.y = glm::ceil(size.y);

	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	e2::UIManager* ui = uiManager();

	constexpr float epsilon = 0.0001f;
	m_currentZ -= epsilon;

	e2::UIFancyQuadPushConstants pushConstants{};
	pushConstants.quadColor = color.toVec4();
	pushConstants.quadPosition = position;
	pushConstants.quadSize = size;
	pushConstants.quadZ = m_currentZ;
	pushConstants.surfaceSize = glm::vec2(m_renderTargetSize);
	pushConstants.cornerRadius = cornerRadius;
	pushConstants.bevelStrength = bevelStrength;
	pushConstants.type = windowBorder ? 1 : 0;
	pushConstants.pixelScale = ui->workingStyle().scale;

	// @todo optimize this by queue'ing quads and then on submit render them. 
	// profile first so we know its nececssary
	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->m_fancyQuadPipeline.pipeline);
	buff->pushConstants(ui->m_fancyQuadPipeline.layout, 0, sizeof(e2::UIFancyQuadPushConstants), reinterpret_cast<uint8_t*>(&pushConstants));

	buff->draw(6, 1);

	m_hasRecordedData = true;
}

void e2::UIContext::drawQuadShadow(glm::vec2 position, glm::vec2 size, float cornerRadius, float shadowStrength, float shadowSize)
{
	position.x = glm::floor(position.x);
	position.y = glm::floor(position.y);

	size.x = glm::ceil(size.x);
	size.y = glm::ceil(size.y);

	uint8_t frameIndex = renderManager()->frameIndex();
	e2::ICommandBuffer* buff = m_commandBuffers[frameIndex];
	e2::UIManager* ui = uiManager();

	constexpr float epsilon = 0.0001f;
	m_currentZ -= epsilon;

	e2::UIQuadShadowPushConstants pushConstants{};
	pushConstants.quadColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
	pushConstants.quadPosition = position;
	pushConstants.quadSize = size;
	pushConstants.quadZ = m_currentZ;
	pushConstants.surfaceSize = glm::vec2(m_renderTargetSize);
	pushConstants.cornerRadius = cornerRadius;
	pushConstants.shadowStrength = shadowStrength;
	pushConstants.shadowSize = shadowSize;

	// @todo optimize this by queue'ing quads and then on submit render them. 
	// profile first so we know its nececssary
	buff->bindVertexLayout(ui->quadVertexLayout);
	buff->bindIndexBuffer(ui->quadIndexBuffer);
	buff->bindVertexBuffer(0, ui->quadVertexBuffer);
	buff->bindPipeline(ui->m_quadShadowPipeline.pipeline);
	buff->pushConstants(ui->m_quadShadowPipeline.layout, 0, sizeof(e2::UIQuadShadowPushConstants), reinterpret_cast<uint8_t*>(&pushConstants));

	buff->draw(6, 1);

	m_hasRecordedData = true;
}

void e2::UIContext::drawRasterText(e2::FontFace fontFace, uint8_t fontSize, e2::UIColor color, glm::vec2 position, std::string const& markdownUtf8, bool enableColorChange, bool soft )
{
	position.x = glm::floor(position.x);
	position.y = glm::floor(position.y);

	e2::UIStyle style = uiManager()->workingStyle();
	e2::FontPtr font = renderManager()->defaultFont(fontFace);
	// this is only for editor use, so it's fine that this contains an inline markdown parser executed every frame, and a shit ton of separate quad drawcalls etc.
	// you guys have workstations for a reason right? What else are you going to with that insanely overspec'd CPU of yours?
	// i really don't see what you're getting yourself so worked up about
	std::u32string src = e2::utf8to32(markdownUtf8);

	float midOffset = font->getMidlineOffset(e2::FontStyle::Regular, fontSize);

	UIColor defaultColor = color;
	const e2::UIColor accents[10] = {
		style.bright1,	// 0 bright
		style.dark1,	// 1 dark
		style.accents[0], // 2 Yellow 
		style.accents[1], // 3 Orange
		style.accents[2], // 4 Red
		style.accents[3], // 5 Magenta
		style.accents[4], // 6 Violet
		style.accents[5], // 7 Blue
		style.accents[6], // 8 Cyan
		style.accents[7], // 9 Green
	};

	glm::vec2 cursor = position + glm::vec2(0.f, midOffset);

	uint32_t nextCodepoint = 0;
	uint32_t prevCodepoint = 0;
	uint32_t i = 0;
	bool escape = false;
	e2::FontStyle currentStyle = FontStyle::Regular;
	for (uint32_t i = 0; i < src.size(); i++)
	{
		uint32_t codepoint = src[i];

		bool peekable = i < src.size() - 1;
		if (peekable)
		{
			nextCodepoint = src[i + 1];
		}

		// special characters
		if (codepoint <= 32)
		{
			// we ignore 10 (\n)
			// space, just OK?
			// literally dont know how long a space is supposed to be.
			// should probably be scaled to the fontsize or smth @todo
			if (codepoint == 32)
			{
				cursor.x += font->getSpaceAdvance(currentStyle, fontSize);
			}

			continue;
		}

		if (!escape)
		{
			// \ escape
			if (codepoint == 92)
			{
				escape = true;
				continue;
			}
			// * handle bold and italics 
			else if (codepoint == 42)
			{
				// ** double means flip bold 
				if (peekable && nextCodepoint == 42)
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Bold));
					// add more to i, since we already parse the next codepoint here
					i++;
				}
				// * single means we flip italic 
				else
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Italic));
				}

				continue;
			}
			// ^ handle fontface changes 
			else if (codepoint == 94 && peekable)
			{
				// s = sans 
				if (nextCodepoint == 115)
				{
					font = renderManager()->defaultFont(e2::FontFace::Sans);
				}
				// f = serif 
				else if (nextCodepoint == 102)
				{
					font = renderManager()->defaultFont(e2::FontFace::Serif);
				}
				// m = monospace 
				else if (nextCodepoint == 109)
				{
					font = renderManager()->defaultFont(e2::FontFace::Monospace);
				}
				// 0-9 = accent colors (see start of function for a table)
				else if (nextCodepoint >= 48 && nextCodepoint <= 57 && enableColorChange)
				{
					uint32_t colorIndex = nextCodepoint - 48;
					color = accents[colorIndex];
				}
				// - = disable accent 
				else if (nextCodepoint == 45)
				{
					color = defaultColor;
				}
				i++;
				continue;
			}
		}

		// normal characters start here
		if (i != 0)
			cursor.x += font->getRasterKerningDistance(prevCodepoint, codepoint, currentStyle, fontSize);

		e2::FontGlyph const&glyph = font->getRasterGlyph(codepoint, currentStyle, fontSize);
		
		drawTexturedQuad(cursor + glyph.offset, glyph.size, color, font->glyphTexture(glyph.textureIndex), glyph.uvOffset, glyph.uvSize, e2::UITexturedQuadType::FontRaster);

		cursor.x += glyph.advanceX;

		prevCodepoint = codepoint;
		escape = false;
	}
}




void e2::UIContext::drawSDFText(e2::FontFace fontFace, float fontSize, e2::UIColor color, glm::vec2 position, std::string const& markdownUtf8, bool enableColorChange /*= true*/, bool soft /*= false*/)
{
	position.x = glm::floor(position.x);
	position.y = glm::floor(position.y);

	e2::UIStyle style = uiManager()->workingStyle();
	e2::FontPtr font = renderManager()->defaultFont(fontFace);
	// this is only for editor use, so it's fine that this contains an inline markdown parser executed every frame, and a shit ton of separate quad drawcalls etc.
	// you guys have workstations for a reason right? What else are you going to with that insanely overspec'd CPU of yours?
	// i really don't see what you're getting yourself so worked up about
	std::u32string src = e2::utf8to32(markdownUtf8);

	float midOffset = font->getMidlineOffset(e2::FontStyle::Regular, fontSize);

	UIColor defaultColor = color;
	const e2::UIColor accents[10] = {
		style.bright1,	// 0 bright
		style.dark1,	// 1 dark
		style.accents[0], // 2 Yellow 
		style.accents[1], // 3 Orange
		style.accents[2], // 4 Red
		style.accents[3], // 5 Magenta
		style.accents[4], // 6 Violet
		style.accents[5], // 7 Blue
		style.accents[6], // 8 Cyan
		style.accents[7], // 9 Green
	};

	glm::vec2 cursor = position + glm::vec2(0.f, midOffset);

	uint32_t nextCodepoint = 0;
	uint32_t prevCodepoint = 0;
	uint32_t i = 0;
	bool escape = false;
	e2::FontStyle currentStyle = FontStyle::Regular;
	for (uint32_t i = 0; i < src.size(); i++)
	{
		uint32_t codepoint = src[i];

		bool peekable = i < src.size() - 1;
		if (peekable)
		{
			nextCodepoint = src[i + 1];
		}

		// special characters
		if (codepoint <= 32)
		{
			// we ignore 10 (\n)
			// space, just OK?
			// literally dont know how long a space is supposed to be.
			// should probably be scaled to the fontsize or smth @todo
			if (codepoint == 32)
			{
				cursor.x += font->getSDFSpaceAdvance(currentStyle, fontSize);
			}

			continue;
		}

		if (!escape)
		{
			// \ escape
			if (codepoint == 92)
			{
				escape = true;
				continue;
			}
			// * handle bold and italics 
			else if (codepoint == 42)
			{
				// ** double means flip bold 
				if (peekable && nextCodepoint == 42)
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Bold));
					// add more to i, since we already parse the next codepoint here
					i++;
				}
				// * single means we flip italic 
				else
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Italic));
				}

				continue;
			}
			// ^ handle fontface changes 
			else if (codepoint == 94 && peekable)
			{
				// s = sans 
				if (nextCodepoint == 115)
				{
					font = renderManager()->defaultFont(e2::FontFace::Sans);
				}
				// f = serif 
				else if (nextCodepoint == 102)
				{
					font = renderManager()->defaultFont(e2::FontFace::Serif);
				}
				// m = monospace 
				else if (nextCodepoint == 109)
				{
					font = renderManager()->defaultFont(e2::FontFace::Monospace);
				}
				// 0-9 = accent colors (see start of function for a table)
				else if (nextCodepoint >= 48 && nextCodepoint <= 57 && enableColorChange)
				{
					uint32_t colorIndex = nextCodepoint - 48;
					color = accents[colorIndex];
				}
				// - = disable accent 
				else if (nextCodepoint == 45)
				{
					color = defaultColor;
				}
				i++;
				continue;
			}
		}

		// normal characters start here
		if (i != 0)
			cursor.x += font->getSDFKerningDistance(prevCodepoint, codepoint, currentStyle, fontSize);

		e2::FontGlyph const& glyph = font->getSDFGlyph(codepoint, currentStyle);

		glm::vec2 glyphSize = (glyph.size / 32.0f) * fontSize;
		float advanceX = (glyph.advanceX / 32.0f) * fontSize;
		glm::vec2 glyphOffset = (glyph.offset / 32.0f)* fontSize;

		drawTexturedQuad(cursor + glyphOffset, glyphSize, color, font->glyphTexture(glyph.textureIndex), glyph.uvOffset, glyph.uvSize, e2::UITexturedQuadType::FontSDFShadow);

		cursor.x += advanceX;

		prevCodepoint = codepoint;
		escape = false;
	}
}

void e2::UIContext::drawRasterTextShadow(e2::FontFace fontFace, uint8_t fontSize, glm::vec2 position, std::string const& markdownUtf8)
{
	drawRasterText(fontFace, fontSize, 0x000000FF, position + glm::vec2(1.0f, 1.0f), markdownUtf8, false, true);
}

float e2::UIContext::calculateSDFTextWidth(e2::FontFace fontFace, float fontSize, std::string const& markdownUtf8)
{
	e2::UIStyle style = uiManager()->workingStyle();
	e2::FontPtr font = renderManager()->defaultFont(fontFace);
	// this is only for editor use, so it's fine that this contains an inline markdown parser executed every frame, and a shit ton of separate quad drawcalls etc.
	// you guys have workstations for a reason right? What else are you going to with that insanely overspec'd CPU of yours?
	// i really don't see what you're getting yourself so worked up about
	std::u32string src = e2::utf8to32(markdownUtf8);

	float midOffset = font->getMidlineOffset(e2::FontStyle::Regular, fontSize);

	glm::vec2 cursor = glm::vec2(0.f, midOffset);

	uint32_t nextCodepoint = 0;
	uint32_t prevCodepoint = 0;
	uint32_t i = 0;
	bool escape = false;
	e2::FontStyle currentStyle = FontStyle::Regular;
	for (uint32_t i = 0; i < src.size(); i++)
	{
		uint32_t codepoint = src[i];

		bool peekable = i < src.size() - 1;
		if (peekable)
		{
			nextCodepoint = src[i + 1];
		}

		// special characters
		if (codepoint <= 32)
		{
			// we ignore 10 (\n)
			// space, just OK?
			// literally dont know how long a space is supposed to be.
			// should probably be scaled to the fontsize or smth @todo
			if (codepoint == 32)
			{
				cursor.x += font->getSDFSpaceAdvance(currentStyle, fontSize);
			}

			continue;
		}

		if (!escape)
		{
			// \ escape
			if (codepoint == 92)
			{
				escape = true;
				continue;
			}
			// * handle bold and italics 
			else if (codepoint == 42)
			{
				// ** double means flip bold 
				if (peekable && nextCodepoint == 42)
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Bold));
					// add more to i, since we already parse the next codepoint here
					i++;
				}
				// * single means we flip italic 
				else
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Italic));
				}

				continue;
			}
			// ^ handle fontface changes 
			else if (codepoint == 94 && peekable)
			{
				// s = sans 
				if (nextCodepoint == 115)
				{
					font = renderManager()->defaultFont(e2::FontFace::Sans);
				}
				// f = serif 
				else if (nextCodepoint == 102)
				{
					font = renderManager()->defaultFont(e2::FontFace::Serif);
				}
				// m = monospace 
				else if (nextCodepoint == 109)
				{
					font = renderManager()->defaultFont(e2::FontFace::Monospace);
				}
				// 0-9 = accent colors (see start of function for a table)
				else if (nextCodepoint >= 48 && nextCodepoint <= 57)
				{
					//uint32_t colorIndex = nextCodepoint - 48;
					//color = accents[colorIndex];
				}
				// - = disable accent 
				else if (nextCodepoint == 45)
				{
					//color = defaultColor;
				}
				i++;
				continue;
			}
		}

		// normal characters start here
		if (i != 0)
			cursor.x += font->getSDFKerningDistance(prevCodepoint, codepoint, currentStyle, fontSize);

		e2::FontGlyph const& glyph = font->getSDFGlyph(codepoint, currentStyle);

		cursor.x += (glyph.advanceX / 32.0f) * fontSize;

		prevCodepoint = codepoint;
		escape = false;
	}

	return cursor.x;
}

float e2::UIContext::calculateTextWidth(e2::FontFace fontFace, uint8_t fontSize, std::string const& markdownUtf8)
{
	e2::UIStyle style = uiManager()->workingStyle();
	e2::FontPtr font = renderManager()->defaultFont(fontFace);
	// this is only for editor use, so it's fine that this contains an inline markdown parser executed every frame, and a shit ton of separate quad drawcalls etc.
	// you guys have workstations for a reason right? What else are you going to with that insanely overspec'd CPU of yours?
	// i really don't see what you're getting yourself so worked up about
	std::u32string src = e2::utf8to32(markdownUtf8);

	float midOffset = font->getMidlineOffset(e2::FontStyle::Regular, fontSize);

	glm::vec2 cursor = glm::vec2(0.f, midOffset);

	uint32_t nextCodepoint = 0;
	uint32_t prevCodepoint = 0;
	uint32_t i = 0;
	bool escape = false;
	e2::FontStyle currentStyle = FontStyle::Regular;
	for (uint32_t i = 0; i < src.size(); i++)
	{
		uint32_t codepoint = src[i];

		bool peekable = i < src.size() - 1;
		if (peekable)
		{
			nextCodepoint = src[i + 1];
		}

		// special characters
		if (codepoint <= 32)
		{
			// we ignore 10 (\n)
			// space, just OK?
			// literally dont know how long a space is supposed to be.
			// should probably be scaled to the fontsize or smth @todo
			if (codepoint == 32)
			{
				cursor.x += font->getSpaceAdvance(currentStyle, fontSize);
			}

			continue;
		}

		if (!escape)
		{
			// \ escape
			if (codepoint == 92)
			{
				escape = true;
				continue;
			}
			// * handle bold and italics 
			else if (codepoint == 42)
			{
				// ** double means flip bold 
				if (peekable && nextCodepoint == 42)
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Bold));
					// add more to i, since we already parse the next codepoint here
					i++;
				}
				// * single means we flip italic 
				else
				{
					currentStyle = e2::FontStyle(uint8_t(currentStyle) ^ uint8_t(e2::FontStyle::Italic));
				}

				continue;
			}
			// ^ handle fontface changes 
			else if (codepoint == 94 && peekable)
			{
				// s = sans 
				if (nextCodepoint == 115)
				{
					font = renderManager()->defaultFont(e2::FontFace::Sans);
				}
				// f = serif 
				else if (nextCodepoint == 102)
				{
					font = renderManager()->defaultFont(e2::FontFace::Serif);
				}
				// m = monospace 
				else if (nextCodepoint == 109)
				{
					font = renderManager()->defaultFont(e2::FontFace::Monospace);
				}
				// 0-9 = accent colors (see start of function for a table)
				else if (nextCodepoint >= 48 && nextCodepoint <= 57)
				{
					//uint32_t colorIndex = nextCodepoint - 48;
					//color = accents[colorIndex];
				}
				// - = disable accent 
				else if (nextCodepoint == 45)
				{
					//color = defaultColor;
				}
				i++;
				continue;
			}
		}

		// normal characters start here
		if (i != 0)
			cursor.x += font->getRasterKerningDistance(prevCodepoint, codepoint, currentStyle, fontSize);

		e2::FontGlyph const& glyph = font->getRasterGlyph(codepoint, currentStyle, fontSize);

		//drawTexturedQuad(cursor + glyph.offset, glyph.size, color, font->glyphTexture(glyph.textureIndex), glyph.uvOffset, glyph.uvSize, e2::UITexturedQuadType::FontRaster);

		cursor.x += glyph.advanceX;

		prevCodepoint = codepoint;
		escape = false;
	}

	return cursor.x;
}

e2::UIRenderState& e2::UIContext::pushRenderState(e2::Name id, glm::vec2 const& size, glm::vec2 offset)
{
	UIRenderState &prevState = renderState();

	pushId(id);
	e2::UIRenderState newState;
	newState.offset = offset;
	prevState.getSizeForNext(prevState, size, newState.offset, newState.size);
	newState.cursor = newState.offset;

	newState.maxParallelSize = 0.f;
	newState.indentLevels = 0;

	m_renderStates.push(newState);
	return renderState();
}

void e2::UIContext::popRenderState()
{
	clearScissor();
	popId();
	// cant pop base state
	if (m_renderStates.size() <= 1)
	{
		return;
	}

	m_renderStates.pop();

}



namespace
{
	void Custom_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;
		outSize = state.size;
	}

	void FlexV_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;

		outSize.x = state.size.x;
		outSize.y = state.flexSizes[state.flexIterator];
		state.flexIterator++;
		state.cursor.y += outSize.y;
	}

	void FlexH_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;

		outSize.y = state.size.y;
		outSize.x = state.flexSizes[state.flexIterator];
		state.flexIterator++;
		state.cursor.x += outSize.x;
	}

	void StackV_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;

		if (minSize.x == 0.0f)
			outSize.x = state.size.x;
		else
			outSize.x = glm::min(state.size.x, minSize.x);

		float remainingSpace = state.size.y - (state.cursor.y - state.offset.y);
		outSize.y = glm::min(remainingSpace, minSize.y);
		state.cursor.y += outSize.y;
	}

	void StackH_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;

		if (minSize.y == 0.0f)
			outSize.y = state.size.y;
		else
			outSize.y = glm::min(state.size.y, minSize.y);

		float remainingSpace = state.size.x - (state.cursor.x - state.offset.x);
		outSize.x = glm::min(remainingSpace, minSize.x);
		state.cursor.x += outSize.x;
	}

	void Wrap_getSizeForNext(e2::UIRenderState& state, glm::vec2 const& minSize, glm::vec2& outOffset, glm::vec2& outSize)
	{
		outOffset = state.cursor;
		outSize = minSize;

		state.cursor.x += outSize.x;
		if (state.cursor.x > state.offset.x + state.size.x)
		{
			state.cursor.y += state.maxParallelSize;
			state.cursor.x = state.offset.x;
			state.maxParallelSize = 0.0f;
			outOffset = state.cursor;
		}

		if (outSize.y > state.maxParallelSize)
			state.maxParallelSize = outSize.y;
	}
}

void e2::UIContext::pushFixedPanel(e2::Name id, glm::vec2 const& position, glm::vec2 const& size)
{
	e2::UIRenderState& prevState = renderState();

	//e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, 0.f));


	pushId(id);
	e2::UIRenderState newState;
	newState.offset = position;
	newState.size = size;
	newState.cursor = newState.offset;
	newState.maxParallelSize = 0.f;
	newState.indentLevels = 0;
	newState.getSizeForNext = &::Custom_getSizeForNext;
	m_renderStates.push(newState);
	e2::UIRenderState& state = renderState();

}

void e2::UIContext::popFixedPanel()
{
	popRenderState();
}

void e2::UIContext::beginFlexV(e2::Name id, float const* rowSizes, uint32_t rowSizeCount)
{
	e2::UIRenderState& prevState = renderState();

	e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, 0.f));
	newState.getSizeForNext = &::FlexV_getSizeForNext;

	// Calculate the flex sizes
	float accum = 0.0f;
	uint32_t numEmpty = 0;
	for (uint32_t i = 0; i < rowSizeCount; i++)
	{
		if (rowSizes[i] > 0.0f)
		{
			accum += rowSizes[i];
		}
		else
		{
			numEmpty++;
		}
	}

	float emptySize = glm::max(0.0f, (newState.size.y - accum) / float(numEmpty));

	// Fill the calculated flex sizes
	float fullRowSize = 0.0f;
	for (uint32_t i = 0; i < rowSizeCount; i++)
	{
		float rem = glm::max(0.0f, newState.size.y - fullRowSize);
		float adder = rowSizes[i] > 0.0f ? rowSizes[i] : emptySize;

		if (adder > rem)
			adder = rem;

		fullRowSize += adder;

		newState.flexSizes.push(adder);
	}
	
}

void e2::UIContext::endFlexV()
{
	popRenderState();
}

void e2::UIContext::flexVSlider(e2::Name id, float& value, float scale)
{
	e2::UIStyle const& style = uiManager()->workingStyle();


	glm::vec2 minSize = glm::vec2(0.0f, 0.0f) * style.scale;
	e2::UIWidgetState* widgetState = reserve(id, minSize);


	if (widgetState->active && m_mouseState.buttons[0].pressed)
	{
		widgetState->dragOrigin = value;
	}

	if (widgetState->active && m_mouseState.buttons[0].held)
	{
		value = glm::max(1.0f, widgetState->dragOrigin + (m_mouseState.buttons[0].dragOffset.y * scale));
	}

	if (widgetState->hovered)
	{
		drawQuad(widgetState->position, widgetState->size, style.accents[UIAccent_Blue]);
		m_window->cursor(CursorShape::ResizeNS);
	}
	else
	{
		drawQuad(widgetState->position, widgetState->size, style.windowBgColorInactive);
	}
}

void e2::UIContext::beginFlexH(e2::Name id, float const* rowSizes, uint32_t rowSizeCount)
{
	e2::UIRenderState& prevState = renderState();

	e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, 0.f));
	newState.getSizeForNext = &::FlexH_getSizeForNext;

	// Calculate the flex sizes
	float accum = 0.0f;
	uint32_t numEmpty = 0;
	for (uint32_t i = 0; i < rowSizeCount; i++)
	{
		if (rowSizes[i] > 0.0f)
		{
			accum += rowSizes[i];
		}
		else
		{
			numEmpty++;
		}
	}

	float emptySize = glm::max(0.0f, (newState.size.x - accum) / float(numEmpty));

	// Fill the calculated flex sizes
	float fullRowSize = 0.0f;
	for (uint32_t i = 0; i < rowSizeCount; i++)
	{
		float rem = glm::max(0.0f, newState.size.x - fullRowSize);
		float adder = rowSizes[i] > 0.0f ? rowSizes[i] : emptySize;

		if (adder > rem)
			adder = rem;

		fullRowSize += adder;

		newState.flexSizes.push(adder);
	}

}

void e2::UIContext::endFlexH()
{
	popRenderState();
}

void e2::UIContext::flexHSlider(e2::Name id, float& value, float scale /*= 1.0f*/)
{
	e2::UIStyle const& style = uiManager()->workingStyle();


	glm::vec2 minSize = glm::vec2(0.0f, 0.0f) * style.scale;
	e2::UIWidgetState* widgetState = reserve(id, minSize);


	if (widgetState->active && m_mouseState.buttons[0].pressed)
	{
		widgetState->dragOrigin = value;
	}

	if (widgetState->active && m_mouseState.buttons[0].held)
	{
		value = glm::max(1.0f, widgetState->dragOrigin + (m_mouseState.buttons[0].dragOffset.x * scale));
	}

	if (widgetState->hovered)
	{
		drawQuad(widgetState->position, widgetState->size, style.accents[UIAccent_Blue]);
		m_window->cursor(CursorShape::ResizeWE);
	}
	else
	{
		drawQuad(widgetState->position, widgetState->size, style.windowBgColorInactive);
	}
}

void e2::UIContext::beginStackV(e2::Name id, glm::vec2 offset)
{
	e2::UIRenderState& prevState = renderState();

	e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, 0.f), offset);
	newState.getSizeForNext = &::StackV_getSizeForNext;

}

void e2::UIContext::endStackV()
{
	popRenderState();
}

void e2::UIContext::beginStackH(e2::Name id, float minHeight)
{
	e2::UIRenderState& prevState = renderState();

	e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, minHeight));
	newState.getSizeForNext = &::StackH_getSizeForNext;
}

void e2::UIContext::endStackH()
{
	popRenderState();
}

void e2::UIContext::beginWrap(e2::Name id)
{
	e2::UIRenderState& prevState = renderState();

	e2::UIRenderState& newState = pushRenderState(id, glm::vec2(0.f, 0.0f));
	newState.getSizeForNext = &::Wrap_getSizeForNext;
}

void e2::UIContext::endWrap()
{
	popRenderState();
}

e2::UIRenderState& e2::UIContext::renderState()
{
	return m_renderStates[m_renderStates.size() - 1];
}

e2::UIRenderState& e2::UIContext::baseState()
{
	return m_renderStates[0];
}

glm::vec2 const& e2::UIContext::renderedSize()
{
	return m_renderedSize;
}

glm::vec2 e2::UIContext::renderedPadding()
{
	return glm::vec2(m_renderTargetSize) - m_renderStates[0].size;
}

void e2::UIContext::unsetActive()
{
	m_activeId = 0;
}

void e2::UIContext::pushId(e2::Name name)
{
	m_idStack.push(name);
}

void e2::UIContext::popId()
{
	if (!m_idStack.pop())
	{
		LogError("Tried to pop id stack but id stack was empty. Check your pop/push pairs.");
	}
}

size_t e2::UIContext::getCombinedHash(e2::Name id)
{
	size_t seed = 0;
	for (e2::Name n : m_idStack)
	{
		e2::hash_combine(seed, n);
	}

	e2::hash_combine(seed, id);

	return seed;
}

e2::UIWidgetState* e2::UIContext::reserve(e2::Name id, glm::vec2 const& minSize)
{
	e2::UIWidgetState* data = nullptr;

	// resolve the data we need to use 
	size_t combinedHash = getCombinedHash(id);
	auto finder = m_dataMap.find(combinedHash);
	if (finder != m_dataMap.end())
	{
		data = finder->second;
		data->framesSinceActive = 0;
	}
	else
	{
		data = m_dataArena.create();
		m_dataMap[combinedHash] = data;

		data->id = combinedHash;
	}

	// set the widget size and position 
	e2::UIRenderState& rs = renderState();
	//data->position = rs.cursor;
	rs.getSizeForNext(rs, minSize, data->position, data->size);
	data->hovered = e2::intersect(m_mouseState.relativePosition, data->position, data->size);

	if (data->hovered && m_mouseState.buttons[0].pressed)
	{
		m_activeId = combinedHash;
		data->active = true;
	}

	if (!data->hovered && m_mouseState.buttons[0].pressed)
	{
		data->active = false;
	}

	glm::vec2 farPosition = data->position - m_renderStates[0].offset + data->size;
	if (m_renderedSize.x < farPosition.x)
		m_renderedSize.x = farPosition.x;
	if (m_renderedSize.y < farPosition.y)
		m_renderedSize.y = farPosition.y;


	setScissor(data->position, data->size);

	/* @todo move to stack
	// update max row height if needed, to know how much we need to advance cursor vertically once its time 
	if (minSize.y > m_renderState.maxRowHeight)
		m_renderState.maxRowHeight = minSize.y;

	// advance the cursor horizontally
	m_renderState.cursor.x += minSize.x + 4.0f; // 4px margin

	// push down cursor if it makes sense 
	if (m_renderState.cursor.x >= m_renderState.clientAreaMax.x)
	{
		newLine();
	}*/

	return data;
}

glm::uvec2 const& e2::UIContext::size()
{
	return m_renderTargetSize;
}

void e2::UIContext::resize(glm::uvec2 newSize)
{
	if (m_renderTargetSize != newSize && newSize.x != 0 && newSize.y != 0)
	{
		//LogNotice("Resizing UI context {}", newSize);
		renderContext()->waitIdle();

		if(m_renderTarget)
			e2::destroy(m_renderTarget);
		
		if(m_colorTexture)
			e2::destroy(m_colorTexture);

		if(m_depthTexture)
			e2::destroy(m_depthTexture);

		// Setup target textures 
		e2::TextureCreateInfo textureCreateInfo{};
		textureCreateInfo.resolution = glm::uvec3(newSize, 1);

		// create color attachment 
		textureCreateInfo.format = TextureFormat::RGBA8;
		textureCreateInfo.initialLayout = TextureLayout::ColorAttachment;
		m_colorTexture = renderContext()->createTexture(textureCreateInfo);

		// create depth attachment
		textureCreateInfo.format = TextureFormat::D16;
		textureCreateInfo.initialLayout = TextureLayout::DepthAttachment;
		m_depthTexture = renderContext()->createTexture(textureCreateInfo);


		// Setup render target 
		e2::RenderTargetCreateInfo renderTargetInfo{};
		renderTargetInfo.areaExtent = newSize;

		e2::RenderAttachment colorAttachment{};
		colorAttachment.target = m_colorTexture;
		colorAttachment.clearMethod = ClearMethod::ColorUint;
		colorAttachment.clearValue.clearColorf32 = { 0.0f, 0.0f, 0.0f, 0.0f };
		colorAttachment.loadOperation = LoadOperation::Clear;
		colorAttachment.storeOperation = StoreOperation::Store;
		renderTargetInfo.colorAttachments.push(colorAttachment);

		renderTargetInfo.depthAttachment.target = m_depthTexture;
		renderTargetInfo.depthAttachment.clearMethod = ClearMethod::DepthStencil;
		renderTargetInfo.depthAttachment.clearValue.depth = 1.0f;
		renderTargetInfo.depthAttachment.loadOperation = LoadOperation::Clear;
		renderTargetInfo.depthAttachment.storeOperation = StoreOperation::Store;

		m_renderTarget = renderContext()->createRenderTarget(renderTargetInfo);
		m_renderTargetSize = newSize;
	}
}

e2::ITexture* e2::UIContext::colorTexture()
{
	return m_colorTexture;
}

e2::UIMouseState & e2::UIContext::mouseState()
{
	return m_mouseState;
}

e2::UIKeyboardState& e2::UIContext::keyboardState()
{
	return m_keyboardState;
}

bool e2::UIKeyboardState::state(e2::Key k)
{
	return keys[uint16_t(k)].state;
}

bool e2::UIKeyboardState::pressed(e2::Key k)
{
	return keys[uint16_t(k)].pressed;
}

bool e2::UIKeyboardState::released(e2::Key k)
{
	return keys[uint16_t(k)].released;
}

bool e2::UIMouseState::buttonState(e2::MouseButton btn)
{
	return buttons[uint8_t(btn)].state;
}

bool e2::UIMouseState::buttonPressed(e2::MouseButton btn)
{
	return buttons[uint8_t(btn)].pressed;
}

bool e2::UIMouseState::buttonReleased(e2::MouseButton btn)
{
	return buttons[uint8_t(btn)].released;
}

bool e2::UIMouseState::buttonDoubleClick(e2::MouseButton btn)
{
	return buttons[uint8_t(btn)].doubleClicked;
}
