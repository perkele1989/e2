
#pragma once 

#include <e2/buildcfg.hpp>
#include <e2/export.hpp>
#include <e2/utils.hpp>
#include <glm/glm.hpp>

namespace e2
{
	/** */
	class AnimusPose : public e2::Object
	{
		ObjectDeclaration()
	public:
		AnimusPose();
		virtual ~AnimusPose();

		e2::StackVector<glm::mat4, e2::maxNumSkeletonBones> bones;

	};

}

#include "animuspose.generated.hpp"