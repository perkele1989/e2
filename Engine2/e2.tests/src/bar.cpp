
#include "bar.hpp"


Bar::Bar()
{
	LogNotice("A {}", uint64_t(this));
}

Bar::~Bar()
{
	LogNotice("Z {}", uint64_t(this));
} 

