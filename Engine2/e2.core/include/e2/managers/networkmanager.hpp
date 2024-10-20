
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <e2/utils.hpp>


namespace e2
{
	class Engine;
	class Session;

	constexpr uint64_t maxBufferedPackets = 1024;
	constexpr uint64_t packetDataSize = 480;

	struct NetworkPacket
	{
		uint16_t sequenceId;
		uint16_t packetType;
		uint16_t packetSize;
		uint8_t packetData[e2::packetDataSize];
	};

	class E2_API NetworkManager : public Manager
	{
	public:
		NetworkManager(Engine* owner);
		virtual ~NetworkManager();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

	protected:

	};


}
