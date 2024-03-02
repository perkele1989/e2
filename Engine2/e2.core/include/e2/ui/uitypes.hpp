
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>

#include <bit>

namespace e2
{

	struct UIVertex
	{
		glm::vec4 position;
	};

	struct E2_API UIColor
	{
		UIColor(glm::vec4 const& source)
		{
			glm::uvec4 c = glm::floor(source * 255.f);
			r = c.x;
			g = c.y;
			b = c.z;
			a = c.w;
		}

		UIColor(uint32_t inHex);

		UIColor(uint8_t inR, uint8_t inG, uint8_t inB, uint8_t inA)
		{
			r = inR;
			g = inG;
			b = inB;
			a = inA;
		}

		glm::vec4 toVec4()
		{
			glm::vec4 returner;
			returner.x = glm::pow(float(r) / 255.f, 2.2f);
			returner.y = glm::pow(float(g) / 255.f, 2.2f);
			returner.z = glm::pow(float(b) / 255.f, 2.2f);
			returner.w = glm::pow(float(a) / 255.f, 2.2f);

			return returner;
		}

		union
		{
			struct
			{
				uint8_t r;
				uint8_t g;
				uint8_t b;
				uint8_t a;
			};

			uint32_t hex{ 0xFF000000 };
		};
	};

	enum UIAccent : uint8_t
	{
		UIAccent_Yellow = 0,
		UIAccent_Orange,
		UIAccent_Red,
		UIAccent_Magenta,
		UIAccent_Violet,
		UIAccent_Blue,
		UIAccent_Cyan,
		UIAccent_Green,
	};

	struct E2_API UIStyle
	{
		// Scales UI elements for DPI reasons
		float scale = 1.00f;

		// These heavily influence the style of the application, and official styles should keep these as default
		// --
		float windowCornerRadius = 20.0f;

		// These are lighter configurable values, that do not influence the overall UX too much 
		// -- 
		e2::UIColor bright1 {0xDFE1E3FF};
		e2::UIColor bright2 {0xCBCED1FF};
		e2::UIColor dark2 { 0x7F7F7FFF };
		e2::UIColor dark1 { 0x2B2B2BFF };

		// Default window colors, background and text (foreground)
		//e2::UIColor windowBgColor{ 0xF0F0F0FF };
		e2::UIColor windowBgColor{ 0xDFE1E3FF };
		e2::UIColor windowBgColorInactive { 0xCBCED1FF };
		e2::UIColor windowFgColor{ 0x2B2B2BFF };
		e2::UIColor windowFgColorInactive{ 0x7F7F7FFF };

		// Window bevel colors
		e2::UIColor windowBevelColorInner { 0xFFFFFF80 };
		e2::UIColor windowBevelColorOuter { 0x0000001A };

		// Widget colors
		e2::UIColor buttonColor { 0xF0F0F0FF };
		e2::UIColor buttonTextColor { 0x0A0A0AFF };

		// Accents 
		e2::UIColor accents[8] = {
			0xB58900FF, // 0 Yellow #B58900 (these colors taken from solarized @todo make sure to give credit)
			0xCB4B16FF, // 1 Orange #CB4B16
			0xDC322FFF, // 2 Red #DC322F
			0xD33682FF, // 3 Magenta #D33682
			0x6C71C4FF, // 4 Violet #6C71C4
			0x268BD2FF, // 5 Blue #268BD2
			0x2AA198FF, // 6 Cyan #2AA198
			0x859900FF, // 7 Green #859900
		};
	};

	/** @todo needed? */
	enum class UIWindowType
	{
		// Default, normal window
		Default,

		// Dialog box, for messages or smaller windows. Needs a parent window and will lock it until closed
		Dialog,

		// transient popup window, cant have focus, used for things like hover messages etc. 
		Popup
	};


	enum class UITexturedQuadType : uint32_t
	{
		Default = 0,
		FontRaster,
		FontSDFShadow
	};

	enum class UITextAlign : uint8_t
	{
		Begin = 0,
		Middle,
		End
	};

}