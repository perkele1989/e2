
#include "e2/ui/uitypes.hpp"

e2::UIColor::UIColor(uint32_t inHex)
{
	hex = std::byteswap(inHex);
}

