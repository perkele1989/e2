#pragma once 

#include <cstdint>
#include <e2/utils.hpp>

namespace e2 
{
    constexpr uint64_t maxNumEmpires = 256;
    using EmpireId = uint8_t;

	enum class PassableFlags : uint8_t
	{
		Land = 0b0000'0001,
		WaterShallow = 0b0000'0010,
		WaterDeep = 0b0000'0100,
		Mountain = 0b0000'1000,
	};

	EnumFlagsDeclaration(PassableFlags);


}