
#pragma once 

#include <e2/utils.hpp>

/** @tags(arena,arenaSize=2) */
class Bar : public e2::ManagedObject
{
	ObjectDeclaration()
public:
	Bar();
	virtual ~Bar(); 

	float gr = 0.7174f;
};

#include "bar.generated.hpp"