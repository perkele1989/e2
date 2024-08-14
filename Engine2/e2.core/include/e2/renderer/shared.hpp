
#pragma once 

#include <e2/export.hpp>
#include <e2/buffer.hpp>
#include <e2/utils.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace e2
{



	struct E2_API TonemapConstants
	{
		alignas(16) glm::vec4 parameters; // exposure, whitepoint, etc etc
	};

	enum class RenderLayer : uint32_t
	{
		Sky = 512,

		Default = 1024,

		Water = 2048,

		Fog = 4096
	};

}