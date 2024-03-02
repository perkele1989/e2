
#include "e2/input/inputdevice.hpp"

e2::InputButton::InputButton(e2::Name name, IInputDevice* device)
	: m_name(name)
	, m_device(device)

{

}

e2::IInputDevice::IInputDevice(e2::IInputContext* inputContext)
	: m_context(inputContext)
{

}

e2::IInputDevice::~IInputDevice()
{

}

e2::Player* e2::IInputDevice::boundPlayer()
{
	return m_boundPlayer;
}

void e2::IInputDevice::bindPlayer(e2::Player* player)
{

}
