
#pragma once 

#include <e2/export.hpp>

#include <e2/utils.hpp>
//#include <e2/input/resource.hpp>

#include <glm/glm.hpp>

#include <vector>

namespace e2
{

	enum class ButtonState : uint8_t
	{
		Pressed,
		Released
	};

	class IInputDevice;
	class IInputContext;
	class Player;

	/** @tags(arena, arenaSize=4096)*/
	class E2_API InputButton : public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		InputButton(e2::Name name, IInputDevice* device);

		/*
		void invoke(ButtonState state)
		{
			e2::Player* player = device->boundPlayer();
			if (!player)
				return;

			e2::Bind* bind = player->bind(m_name);
			if (!bind)
				return;

			bind->trigger(state);
		}*/

	protected:
		e2::Name m_name;
		e2::IInputDevice* m_device{};
		

	};

	class E2_API IInputDevice : public e2::ManagedObject
	{
		ObjectDeclaration()
	public:
		IInputDevice(e2::IInputContext* inputContext);
		virtual ~IInputDevice();

		/*inline e2::IInputContext* context() const
		{
			return m_context;
		}*/

		e2::Player* boundPlayer();

		void bindPlayer(e2::Player* player);

	protected:
		e2::IInputContext* m_context{};

		e2::Player* m_boundPlayer{};
	};
}

#include "inputdevice.generated.hpp"