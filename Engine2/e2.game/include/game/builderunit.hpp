
#pragma once

#include "game/gameunit.hpp"
#include <e2/renderer/meshproxy.hpp>


namespace e2
{



	/** @tags(arena, arenaSize=4096)*/
	class Engineer : public e2::GameUnit
	{
		ObjectDeclaration();
	public:
		Engineer(e2::GameContext* ctx, glm::ivec2 const& tile, uint8_t empire);
		virtual ~Engineer();
		virtual void updateAnimation(double seconds) override;
		virtual void drawUI(e2::UIContext* ctx) override;

	protected:
		e2::SkinProxy* m_skinProxy{};
		e2::Ptr<e2::Animation> m_testAnim;
		float m_testAnimTime{};
		e2::Pose* m_testPose{};
	};
}

#include "builderunit.generated.hpp"