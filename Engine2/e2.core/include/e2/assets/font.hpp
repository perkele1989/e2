
#pragma once

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/rhi/texture.hpp>
#include <e2/assets/asset.hpp>

#include <unordered_map>

namespace e2
{

	enum class FontFace : uint8_t
	{
		/** Use for titles, headers, etc. */
		Sans = 0,

		/** Use for bread text, descriptions, etc. */
		Serif,

		/** Use for code, data, values etc. */
		Monospace,
		Count
	};


	struct E2_API FontGlyph
	{
		uint32_t textureIndex{};

		// UV offsets
		glm::vec2 uvOffset;
		glm::vec2 uvSize;

		// Vertex offsets (scaled)
		glm::vec2 offset;
		glm::vec2 size;

		// The X advance for this glyph
		float advanceX{};
	};

	/** Note: This enum can be used both as bitflags, but also as consecutive indices to arrays. If you change it, make sure these attributes stay true.  */
	enum class FontStyle : uint8_t
	{
		Regular = 0, // 0
		Bold, // 1
		Italic, // 2
		BoldItalic, // 4
		Count,
		Start = Regular,
	};

	struct FontRasterGlyphKey
	{
		uint32_t codepoint{};
		e2::FontStyle style{};
		uint8_t size{11};
	};



	inline bool operator== (FontRasterGlyphKey const& lhs, FontRasterGlyphKey const& rhs) noexcept
	{
		return lhs.codepoint == rhs.codepoint && lhs.style == rhs.style && lhs.size == rhs.size;
	}

	inline bool operator!= (FontRasterGlyphKey const& lhs, FontRasterGlyphKey const& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	inline bool operator< (FontRasterGlyphKey const& lhs, FontRasterGlyphKey const& rhs) noexcept
	{
		return std::tie(lhs.codepoint, lhs.style, lhs.size) < std::tie(rhs.codepoint, rhs.style, rhs.size);
	}

	struct FontSDFGlyphKey
	{
		uint32_t codepoint{};
		e2::FontStyle style{};
	};

	inline bool operator== (FontSDFGlyphKey const& lhs, FontSDFGlyphKey const& rhs) noexcept
	{
		return lhs.codepoint == rhs.codepoint && lhs.style == rhs.style;
	}

	inline bool operator!= (FontSDFGlyphKey const& lhs, FontSDFGlyphKey const& rhs) noexcept
	{
		return !(lhs == rhs);
	}

	inline bool operator< (FontSDFGlyphKey const& lhs, FontSDFGlyphKey const& rhs) noexcept
	{
		return std::tie(lhs.codepoint, lhs.style) < std::tie(rhs.codepoint, rhs.style);
	}
}

/** Make FontGlyphKey hashable */
template <>
struct std::hash<e2::FontSDFGlyphKey>
{
	std::size_t operator()(const e2::FontSDFGlyphKey& key) const
	{
		size_t hash{};
		e2::hash_combine(hash, key.codepoint);
		e2::hash_combine(hash, uint8_t(key.style));
		return hash;
	}
};

template <>
struct std::hash<e2::FontRasterGlyphKey>
{
	std::size_t operator()(const e2::FontRasterGlyphKey& key) const
	{
		size_t hash{};
		e2::hash_combine(hash, key.codepoint);
		e2::hash_combine(hash, uint8_t(key.style));
		e2::hash_combine(hash, uint8_t(key.size));
		return hash;
	}
};

namespace e2 
{


	//struct E2_API TextGeometry
	//{
	//	// 
	//	e2::IDataBuffer* indexBuffer{};
	//	// x, y, u, v (pos and uv)
	//	e2::IDataBuffer* vertexBuffer{};
	//	// int textureIndex (for glyphtextures)
	//	e2::IDataBuffer* textureBuffer{};
	//};

	/** @tags(dynamic, arena, arenaSize=e2::maxNumFontAssets) */
	class E2_API Font : public e2::Asset
	{
		ObjectDeclaration()
	public:
		Font() = default;
		virtual ~Font();

		virtual void write(e2::IStream& destination) const override;
		virtual bool read(e2::IStream& source) override;

		float getMidlineOffset(e2::FontStyle preferredStyle, uint8_t size);

		float getSpaceAdvance(e2::FontStyle preferredStyle, uint8_t size);

		float getRasterKerningDistance(uint32_t left, uint32_t right, e2::FontStyle preferredStyle, uint8_t size);

		float getSDFMidlineOffset(e2::FontStyle preferredStyle, float size);


		float getSDFSpaceAdvance(e2::FontStyle preferredStyle, float size);

		float getSDFKerningDistance(uint32_t left, uint32_t right, e2::FontStyle preferredStyle, float size);


		/** Given a preferred style, return what we can offer */
		e2::FontStyle resolveStyle(e2::FontStyle preferredStyle);
		
		e2::FontGlyph const& getSDFGlyph(uint32_t codepoint, e2::FontStyle preferredStyle);
		e2::FontGlyph const& getRasterGlyph(uint32_t codepoint, e2::FontStyle preferredStyle, uint8_t size);

		e2::ITexture* glyphTexture(uint32_t id);

	protected:

		e2::FontGlyph const& addSDFGlyph(e2::FontSDFGlyphKey const& key);
		e2::FontGlyph const& addRasterGlyph(e2::FontRasterGlyphKey const& key);

		void pushTexture();

		/** 
		 * An index to the internal build state for this specific font 
		 * We obscure it to avoid making private dependencies public
		 */
		uint64_t m_buildId{UINT64_MAX};

		bool m_compatibleStyles[size_t(e2::FontStyle::Count)];
		e2::StackVector<e2::ITexture*, e2::maxNumFontGlyphMaps> m_glyphTextures;
		std::unordered_map<e2::FontSDFGlyphKey, e2::FontGlyph> m_glyphMapSDF;
		std::unordered_map<e2::FontRasterGlyphKey, e2::FontGlyph> m_glyphMapRaster;


	};

}

EnumFlagsDeclaration(e2::FontStyle);

#include "font.generated.hpp"