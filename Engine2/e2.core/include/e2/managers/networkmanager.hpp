
#pragma once 

#include <e2/export.hpp>
#include <e2/manager.hpp>

#include <e2/utils.hpp>
#include <unordered_set>

#if defined(_WIN64)
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif
#include <e2/timer.hpp>

namespace e2
{
	class Engine;
	class Session;

	constexpr uint64_t maxBufferedPackets = 1024;
	constexpr uint64_t netFrameSize = 508;
	constexpr uint64_t netFragmentHeaderSize = 10;
	constexpr uint64_t maxFragmentPayloadSize = netFrameSize - netFragmentHeaderSize;
	constexpr uint64_t maxNumFragments = 65536;
	// 32.768 megabyte max packet sizes (500*65536


	enum class NetFrameType : uint8_t
	{
		TryConnect = 10, // c -> s 
		ConnectEstablished = 11, // s -> c

		Disconnect = 20, // bi


		DataFragment = 30, // bi

	};

	EnumFlagsDeclaration(NetFrameType);

	enum class NetFrameFlags : uint8_t
	{
		Default = 0
	};

	EnumFlagsDeclaration(NetFrameFlags);

	struct NetFragment
	{
		NetFrameType frameType;
		NetFrameFlags frameFlags;

		uint16_t packetId; // sequential packet id (sender)
		uint16_t fragmentIndex; // fragment index (in packet, 0-n)
		uint16_t numFragments;
		uint16_t fragmentSize;
		uint8_t fragmentPayload[e2::maxFragmentPayloadSize];
	};



	struct NetPeerAddress
	{
		uint32_t address; // address in network byte order
		uint16_t port; // port in network byte order 

		bool operator<(const NetPeerAddress& other) const
		{
			return std::tie(address, port) < std::tie(other.address, other.port);
		}

		bool operator==(const NetPeerAddress& other) const
		{
			return address == other.address && port == other.port;
		}
	};

	struct NetPeerAddressHash
	{
		std::size_t operator()(const NetPeerAddress& key) const
		{
			return std::hash<uint32_t>()(key.address) ^ (std::hash<uint16_t>()(key.port) << 1);
		}
	};

	struct NetSendFragment
	{
		NetPeerAddress address;
		NetFragment fragment;
	};

	struct NetPacket
	{
		NetPacket() = default;
		NetPacket(e2::NetPeerAddress const& address, uint8_t const* packetData, uint64_t packetSize);
		e2::NetPeerAddress peerAddress;
		e2::HeapStream data;
	};

	struct NetReceiveState
	{
		uint16_t numFragments{};
		bool* fragmentsReceived{};
		uint32_t packetSize{};
		uint8_t* packetData{};
	};

	struct NetPeerState
	{
		uint16_t outgoingPacketIndex{};
		NetPeerAddress peerAddress;
		std::unordered_map<uint16_t, NetReceiveState> receivingPackets;

	};

	enum class NetworkState
	{
		Offline = 0,
		Server = 1,
		Client_Connecting = 2,
		Client_Connected = 3
	};

	using PacketHandlerCallback = void (*)(e2::Engine* engine, e2::NetPacket &packet);




	class E2_API NetCallbacks
	{
	public:
		/** Called when server started on this machine */
		virtual void onServerStarted() {}

		/** Called when server started on this machine */
		virtual void onServerStopped() {}

		/** Called when we connected to a server */
		virtual void onConnected(NetPeerAddress const& peerAddress) {}

		/** Called when we disconnected from a server */
		virtual void onDisconnected(NetPeerAddress const& peerAddress) {}
		 
		/** Called when a client connected to this server */
		virtual void onPeerConnected(e2::NetPeerAddress const& address) {}

		/** Called when a client disconnected from this server */
		virtual void onPeerDisconnected(e2::NetPeerAddress const& address) {}
	};



	class E2_API NetworkManager : public Manager
	{
	public:
		NetworkManager(Engine* owner);
		virtual ~NetworkManager();

		void disable();
		void startServer(uint16_t port);
		void tryConnect(e2::NetPeerAddress const& address);
		
		void notifyServerDisconnected();

		void flagConnected();

		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void preUpdate(double deltaTime) override;
		virtual void update(double deltaTime) override;

		inline bool threadRunning()
		{
			return !m_killFlag;
		}

		NetPeerState *getOrCreatePeerState(NetPeerAddress const& peerAddress);
		void notifyClientDisconnected(NetPeerAddress const& address);
		void notifyClientConnected(NetPeerAddress const& address);

		void fetchOutgoing(e2::StackVector<e2::NetSendFragment, e2::maxBufferedPackets>& outSwap);

		inline NetPeerAddress serverAddress() const
		{
			return m_serverAddress;
		}

#if defined(_WIN64)
		inline SOCKET& platformSocket()
		{
			return m_socket;
		}

		inline sockaddr_in& platformAddress()
		{
			return m_recvAddr;
		}
#endif

		void receivePacket(e2::NetPacket& newPacket);

		void handleReceivedPacket(e2::NetPacket& packet);

		inline NetworkState currentState() const
		{
			return m_state;
		}

		void sendPacket(e2::NetPacket& outPacket);
		void broadcastPacket(e2::HeapStream& packet);

		void registerHandler(uint8_t handlerId, PacketHandlerCallback handler);

		bool isServer()
		{
			return m_state == e2::NetworkState::Server;
		}

		bool isClient()
		{
			return m_state == e2::NetworkState::Client_Connected;
		}

		bool isConnected()
		{
			return isServer() || isClient();
		}

		void registerNetCallbacks(e2::NetCallbacks* cb);
		void unregisterNetCallbacks(e2::NetCallbacks* cb);

	protected:
		uint16_t m_hostPort{ 1337};
		NetPeerAddress m_serverAddress; // only valid for client
		NetworkState m_state{e2::NetworkState::Offline};
		bool m_disconnectFlag{};
		std::thread m_thread;
		bool m_killFlag{};
		e2::StackVector<e2::NetPacket, e2::maxBufferedPackets> m_incomingBuffer;
		e2::StackVector<e2::NetSendFragment, e2::maxBufferedPackets> m_outgoingBuffer;
		std::recursive_mutex m_syncMutex;
		e2::Moment m_timeSinceTryConnect;
		uint32_t m_timesTriedConnect{0};
		std::unordered_map<NetPeerAddress, NetPeerState*, NetPeerAddressHash> m_peerStates;


		std::array<PacketHandlerCallback, 256> m_handlers;

#if defined(_WIN64)
		WSADATA m_wsaHandle;

		SOCKET m_socket{};
		sockaddr_in m_recvAddr; // only for server

#endif

		std::unordered_set<NetCallbacks*> m_netCallbacks;
	};


}
