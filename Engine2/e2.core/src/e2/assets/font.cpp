
#include "e2/assets/font.hpp"
#include "e2/utils.hpp"

#include "e2/rhi/rendercontext.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

namespace
{

	struct FaceBuildState
	{
		~FaceBuildState()
		{
			// this is allocated in e2::Font::read, but we might just delete it here
			if (fontData)
				delete[] fontData;
		}

		stbtt_fontinfo info;
		uint32_t fontDataSize{};
		uint8_t* fontData{};
	};

	struct FontBuildState
	{
		uint32_t currentTexture{};
		e2::StackVector<FaceBuildState, size_t(e2::FontStyle::Count)> faces;
		stbrp_context packContext; // make sure to reinit every time we push a texture 
		e2::StackVector<stbrp_node, e2::glyphMapResolution> nodes;
	};

	e2::Arena<::FontBuildState> buildStateArena = e2::Arena<::FontBuildState>(e2::maxNumFontAssets);

}


e2::Font::~Font()
{

	//stbtt_InitFont();
	//stbtt_GetGlyphSDF();
	//stbtt_FreeSDF();

	for (e2::ITexture* tex : m_glyphTextures)
	{
		e2::destroy(tex);
	}

	if (m_buildId != UINT64_MAX)
	{
		::buildStateArena.destroy2(m_buildId);
	}

}


void e2::Font::write(Buffer& destination) const
{

}

bool e2::Font::read(Buffer& source)
{
	if (m_buildId != UINT64_MAX)
	{
		::buildStateArena.destroy2(m_buildId);
	}

	m_buildId = ::buildStateArena.create2();
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	buildState->faces.resize(size_t(e2::FontStyle::Count));

	pushTexture();
	
	for (uint32_t i = 0; i < size_t(e2::FontStyle::Count); i++)
	{
		bool readCompatStyles{};
		source >> readCompatStyles;
		m_compatibleStyles[i] = readCompatStyles;
	}


	// Read and init fonts 
	for (uint8_t style = uint8_t(e2::FontStyle::Start); style < uint8_t(e2::FontStyle::Count); style++)
	{
		if (!m_compatibleStyles[style])
			continue;

		uint64_t fontDataSize{};
		source >> fontDataSize;
		buildState->faces[style].fontDataSize = (uint32_t)fontDataSize;

		// @todo optimize out this allocation if deemed possible at some point. Runs on worker thread when loading a font asset.
		buildState->faces[style].fontData = new uint8_t[fontDataSize];
		memcpy(buildState->faces[style].fontData, source.read(fontDataSize), fontDataSize);
		stbtt_InitFont(&buildState->faces[style].info, buildState->faces[style].fontData, 0);
	}

	return true;
}

float e2::Font::getMidlineOffset(e2::FontStyle preferredStyle, uint8_t size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	::FaceBuildState& faceState = buildState->faces[size_t(actualStyle)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, float(size));


	int asc, desc, gap;
	stbtt_GetFontVMetrics (&faceState.info, &asc, &desc, &gap);

	return scale * ( (float(asc) / 2.0f) + (float(desc) / 2.0f)) ;
}

float e2::Font::getSpaceAdvance(e2::FontStyle preferredStyle, uint8_t size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	::FaceBuildState& faceState = buildState->faces[size_t(actualStyle)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, float(size));

	int advance{};
	stbtt_GetCodepointHMetrics(&faceState.info, 32, &advance, nullptr);

	// @todo cache all these metrics somehwere!!
	return scale * float(advance);
}

float e2::Font::getRasterKerningDistance(uint32_t left, uint32_t right, e2::FontStyle preferredStyle, uint8_t size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);

	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	::FaceBuildState& faceState = buildState->faces[size_t(actualStyle)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, float(size));

	return stbtt_GetCodepointKernAdvance(&faceState.info, left, right) * scale;
}

float e2::Font::getSDFSpaceAdvance(e2::FontStyle preferredStyle, float size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	::FaceBuildState& faceState = buildState->faces[size_t(actualStyle)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, size);

	int advance{};
	stbtt_GetCodepointHMetrics(&faceState.info, 32, &advance, nullptr);

	// @todo cache all these metrics somehwere!!
	return scale * float(advance);
}

float e2::Font::getSDFKerningDistance(uint32_t left, uint32_t right, e2::FontStyle preferredStyle, float size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);

	::FontBuildState* buildState = buildStateArena.getById(m_buildId);
	::FaceBuildState& faceState = buildState->faces[size_t(actualStyle)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, size);

	return stbtt_GetCodepointKernAdvance(&faceState.info, left, right) * scale;
}

e2::FontStyle e2::Font::resolveStyle(e2::FontStyle preferredStyle)
{
	e2::FontStyle returner = preferredStyle;
	while (!m_compatibleStyles[size_t(returner)])
		returner = e2::FontStyle(uint8_t(returner) - 1);

	return returner;
}

e2::FontGlyph const& e2::Font::getSDFGlyph(uint32_t codepoint, e2::FontStyle preferredStyle)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);

	e2::FontSDFGlyphKey key;
	key.codepoint = codepoint;
	key.style = actualStyle;

	auto it = m_glyphMapSDF.find(key);
	if (it == m_glyphMapSDF.end())
	{
		return addSDFGlyph(key);
	}

	return it->second;
}



e2::FontGlyph const& e2::Font::getRasterGlyph(uint32_t codepoint, e2::FontStyle preferredStyle, uint8_t size)
{
	e2::FontStyle actualStyle = resolveStyle(preferredStyle);

	e2::FontRasterGlyphKey key;
	key.codepoint = codepoint;
	key.style = actualStyle;
	key.size = size;

	auto it = m_glyphMapRaster.find(key);
	if (it == m_glyphMapRaster.end())
	{
		return addRasterGlyph(key);
	}

	return it->second;
}

e2::ITexture* e2::Font::glyphTexture(uint32_t id)
{
	return m_glyphTextures[id];
}


e2::FontGlyph const& e2::Font::addSDFGlyph(e2::FontSDFGlyphKey const& key)
{
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);

	stbrp_context& packCtx = buildState->packContext;

	::FaceBuildState& faceState = buildState->faces[size_t(key.style)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, 32.f);
	int32_t padding = 8;
	uint8_t edgeValue = 180;
	//      onedge_value = 180
//      pixel_dist_scale = 180/5.0 = 36.0

	int32_t w, h, x, y;
	uint8_t* glyphData = stbtt_GetCodepointSDF(&faceState.info, scale, key.codepoint, padding, edgeValue, float(edgeValue)/ padding, &w, &h, &x, &y);

	if (glyphData == nullptr)
	{
		static e2::FontGlyph g{};
		LogError("Invalid codepoint, we should handle this better in future");
		return g;
	}
	// @todo calculate metrics for font (advance, kerning etc)

	stbrp_rect rect;
	rect.w = w;
	rect.h = h;

	// Pack the rect, if it fails, push a new texture and try again;
	if (!stbrp_pack_rects(&buildState->packContext, &rect, 1))
	{
		pushTexture();
		stbrp_pack_rects(&buildState->packContext, &rect, 1);
	}

	e2::ITexture* currGlyphTexture = m_glyphTextures[buildState->currentTexture];
	currGlyphTexture->upload(0, { rect.x, rect.y, 0 }, { rect.w, rect.h, 1 }, glyphData, w * h);
	stbtt_FreeSDF(glyphData, nullptr);

	//currGlyphTexture->generateMips();


	int advance, leftBearing;
	stbtt_GetCodepointHMetrics(&faceState.info, key.codepoint, &advance, &leftBearing);

	e2::FontGlyph newGlyph;
	newGlyph.offset.x = (float)x;
	newGlyph.offset.y = (float)y;
	newGlyph.size.x = (float)rect.w;
	newGlyph.size.y = (float)rect.h;
	newGlyph.advanceX = float(advance) * scale;
	newGlyph.textureIndex = buildState->currentTexture;
	newGlyph.uvOffset.x = float(rect.x) / float(e2::glyphMapResolution);
	newGlyph.uvOffset.y = float(rect.y) / float(e2::glyphMapResolution);
	newGlyph.uvSize.x =  (float(rect.w) / float(e2::glyphMapResolution));
	newGlyph.uvSize.y =  (float(rect.h) / float(e2::glyphMapResolution));
	m_glyphMapSDF[key] = newGlyph;
	return m_glyphMapSDF[key];
}

e2::FontGlyph const& e2::Font::addRasterGlyph(e2::FontRasterGlyphKey const& key)
{
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);

	stbrp_context& packCtx = buildState->packContext;

	::FaceBuildState& faceState = buildState->faces[size_t(key.style)];

	float scale = stbtt_ScaleForMappingEmToPixels(&faceState.info, float(key.size));

	int32_t w, h, x, y;
	uint8_t* glyphData = stbtt_GetCodepointBitmap(&faceState.info, scale, scale, key.codepoint, &w, &h, &x, &y);

	if (glyphData == nullptr)
	{
		static e2::FontGlyph g{};
		LogError("Invalid codepoint, we should handle this better in future");
		return g;
	}
	// @todo calculate metrics for font (advance, kerning etc)

	stbrp_rect rect;
	rect.w = w;
	rect.h = h;

	// Pack the rect, if it fails, push a new texture and try again;
	if (!stbrp_pack_rects(&buildState->packContext, &rect, 1))
	{
		pushTexture();
		stbrp_pack_rects(&buildState->packContext, &rect, 1);
	}

	e2::ITexture* currGlyphTexture = m_glyphTextures[buildState->currentTexture];
	currGlyphTexture->upload(0, { rect.x, rect.y, 0 }, { rect.w, rect.h, 1 }, glyphData, w * h);
	stbtt_FreeBitmap(glyphData, nullptr);

	//currGlyphTexture->generateMips();


	int advance, leftBearing;
	stbtt_GetCodepointHMetrics(&faceState.info, key.codepoint, &advance, &leftBearing);

	e2::FontGlyph newGlyph;
	newGlyph.offset.x = (float)x;
	newGlyph.offset.y = (float)y;
	newGlyph.size.x = (float)rect.w;
	newGlyph.size.y = (float)rect.h;
	newGlyph.advanceX = float(advance) * scale;
	newGlyph.textureIndex = buildState->currentTexture;
	newGlyph.uvOffset.x = float(rect.x) / float(e2::glyphMapResolution);
	newGlyph.uvOffset.y = float(rect.y) / float(e2::glyphMapResolution);
	newGlyph.uvSize.x = (float(rect.w) / float(e2::glyphMapResolution));
	newGlyph.uvSize.y = (float(rect.h) / float(e2::glyphMapResolution));
	m_glyphMapRaster[key] = newGlyph;
	return m_glyphMapRaster[key];
}

void e2::Font::pushTexture()
{
	::FontBuildState* buildState = buildStateArena.getById(m_buildId);

	stbrp_init_target(&buildState->packContext, e2::glyphMapResolution, e2::glyphMapResolution, &buildState->nodes[0], e2::glyphMapResolution);

	e2::TextureCreateInfo createInfo{};
	createInfo.format = e2::TextureFormat::R8;
	// cap mips to 5, since they turn to garbage at low mips 
	//createInfo.mips = glm::min(uint32_t(5), e2::calculateMipLevels({ e2::glyphMapResolution, e2::glyphMapResolution }));
	createInfo.mips = 1;
	createInfo.resolution = glm::uvec3(e2::glyphMapResolution, e2::glyphMapResolution, 1);

	e2::ITexture* newTexture = renderContext()->createTexture(createInfo);
	m_glyphTextures.push(newTexture);

	buildState->currentTexture = (uint32_t)m_glyphTextures.size() - 1;
	
}
