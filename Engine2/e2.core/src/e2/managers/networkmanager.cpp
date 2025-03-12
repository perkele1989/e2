#include <e2/managers/networkmanager.hpp>








namespace
{
	static void s_networkThread(e2::NetworkManager* manager)
	{
		e2::StackVector<uint8_t, 1024> recvBuff;


		while (manager->threadRunning())
		{
#if defined(_WIN64)

			/*
				uint16_t packetId; // sequential packet id (sender)
				uint16_t fragmentIndex; // fragment index (in packet, 0-n)
				uint16_t numFragments;
				uint16_t fragmentSize;
			*/

			// receive a packet
			//sockaddr_in platformAddr = manager->platformAddress();
			sockaddr_in recvAddr;
			int32_t recvAddrSize = sizeof(recvAddr);
			int32_t bytesRead = recvfrom(manager->platformSocket(), reinterpret_cast<char*>(recvBuff.data()), recvBuff.capacity(), 0, (SOCKADDR*)&recvAddr, &recvAddrSize);

			if (bytesRead == SOCKET_ERROR)
			{
				int32_t wsaError = WSAGetLastError();
				if (wsaError != WSAEWOULDBLOCK)
				{
					LogError("recvfrom failed: {}", WSAGetLastError());
				}
			}
			else
			{

				e2::NetPeerAddress peerAddress;
				peerAddress.address = recvAddr.sin_addr.s_addr;
				peerAddress.port = recvAddr.sin_port;

				//LogNotice("Received UDP frame from {}.{}.{}.{}:{}, at {} bytes",
				//	uint8_t(peerAddress.address),
				//	uint8_t(peerAddress.address >> 8),
				//	uint8_t(peerAddress.address >> 16),
				//	uint8_t(peerAddress.address >> 24),
				//	ntohs(peerAddress.port),
				//	bytesRead);

				e2::NetworkState currentState = manager->currentState();

				bool isServer = (currentState == e2::NetworkState::Server);
				bool isClient = !isServer && (currentState != e2::NetworkState::Offline);
				bool isClientConnecting = currentState == e2::NetworkState::Client_Connecting;
				bool isClientConnected = isClient && !isClientConnecting;

				if (isClientConnecting)
				{
					manager->flagConnected();
				}

				e2::NetPeerState* peerState = manager->getOrCreatePeerState(peerAddress); // @todo mutex?

				e2::RawMemoryStream rawStream(recvBuff.data(), bytesRead, true);
				uint8_t frameTypeInt{};
				uint8_t frameFlagsInt{};
				rawStream >> frameTypeInt >> frameFlagsInt;

				e2::NetFrameType frameType = e2::NetFrameType(frameTypeInt);
				e2::NetFrameFlags frameFlags = e2::NetFrameFlags(frameFlagsInt);
				if (frameType == e2::NetFrameType::DataFragment)
				{
					e2::NetFragment newFragment;

					rawStream >> newFragment.packetId;
					rawStream >> newFragment.fragmentIndex;
					rawStream >> newFragment.numFragments;
					rawStream >> newFragment.fragmentSize;

					if (rawStream.remaining() != newFragment.fragmentSize)
					{
						LogError("Expected {} bytes, but got {}, invalid fragment", newFragment.fragmentSize, rawStream.remaining());
						continue;
					}

					if (newFragment.numFragments > 1)
					{
						auto packetStateIt = peerState->receivingPackets.find(newFragment.packetId);
						if (packetStateIt == peerState->receivingPackets.end())
						{
							e2::NetReceiveState newState;
							newState.numFragments = newFragment.numFragments;
							newState.fragmentsReceived = new bool[newState.numFragments];
							newState.packetData = new uint8_t[newState.numFragments * e2::maxFragmentPayloadSize];


							if (newFragment.fragmentIndex == newFragment.numFragments - 1)
							{
								newState.packetSize = (e2::maxFragmentPayloadSize * (newState.numFragments - 1)) + newFragment.fragmentSize;
							}
							else
							{
								newState.packetSize = 0; // this is set when last fragment is resolved
							}

							newState.fragmentsReceived[newFragment.fragmentIndex] = true;
							uint8_t* writeFragmentData = newState.packetData + (newFragment.fragmentIndex * e2::maxFragmentPayloadSize);
							std::memcpy(writeFragmentData, rawStream.data() + rawStream.cursor(), newFragment.fragmentSize);

							peerState->receivingPackets[newFragment.packetId] = newState;
						}
						else
						{
							e2::NetReceiveState& packetState = packetStateIt->second;

							if (newFragment.fragmentIndex == newFragment.numFragments - 1)
							{
								packetState.packetSize = (e2::maxFragmentPayloadSize * (packetState.numFragments - 1)) + newFragment.fragmentSize;
							}
							packetState.fragmentsReceived[newFragment.fragmentIndex] = true;
							uint8_t* writeFragmentData = packetState.packetData + (newFragment.fragmentIndex * e2::maxFragmentPayloadSize);
							std::memcpy(writeFragmentData, rawStream.data() + rawStream.cursor(), newFragment.fragmentSize);

							bool allWritten = true;
							for (uint16_t i = 0; i < packetState.numFragments; i++)
							{
								if (!packetState.fragmentsReceived[i])
								{
									allWritten = false;
									break;
								}
							}

							if (allWritten)
							{
								e2::NetPacket newPacket(peerAddress, packetState.packetData, packetState.packetSize);
								manager->receivePacket(newPacket);

								delete[] packetState.packetData;
								delete[] packetState.fragmentsReceived;
								peerState->receivingPackets.erase(newFragment.packetId);

							}
						}
					}
					else
					{
						e2::NetPacket newPacket(peerAddress, rawStream.data() + rawStream.cursor(), newFragment.fragmentSize);
						manager->receivePacket(newPacket);
					}
				}
				else if (frameType == e2::NetFrameType::Disconnect)
				{
					if (isClient)
					{
						LogNotice("Server sent disconnect, so we are disconnecting");
						manager->notifyServerDisconnected();
					}
					else if (isServer)
					{
						LogNotice("Client sent disconnect, so we are dropping them");
						manager->notifyClientDisconnected(peerAddress);
					}
				}
				else if (frameType == e2::NetFrameType::TryConnect)
				{
					if (isServer)
					{
						// @todo send connect established or failed 

						uint8_t resp[] = { (uint8_t)e2::NetFrameType::ConnectEstablished, (uint8_t)e2::NetFrameFlags::Default };
						int32_t bytesSent = sendto(manager->platformSocket(), reinterpret_cast<char*>(resp), 2, 0, (SOCKADDR*)&recvAddr, sizeof(recvAddr));
						if (bytesSent == SOCKET_ERROR)
						{
							LogError("sendto failed: {}", WSAGetLastError());
						}
						else
						{
							LogNotice("Sent {} bytes", bytesSent);
						}

						manager->notifyClientConnected(peerAddress);

					}
				}
			}

			// send fragments 

			thread_local e2::StackVector<e2::NetSendFragment, e2::maxBufferedPackets> swapBuffer;
			swapBuffer.clear();

			manager->fetchOutgoing(swapBuffer);

			thread_local uint8_t fragBuff[e2::netFrameSize];
			thread_local uint64_t fragBuffSize = 0;
			for (e2::NetSendFragment& frag : swapBuffer)
			{
				sockaddr_in addr;
				addr.sin_family = AF_INET;
				addr.sin_addr.s_addr = frag.address.address;
				addr.sin_port = frag.address.port;

				uint64_t fullSize = 2;
				if (frag.fragment.frameType == e2::NetFrameType::DataFragment)
				{

					fullSize = e2::netFragmentHeaderSize + frag.fragment.fragmentSize;

					e2::RawMemoryStream rawStream(fragBuff, fullSize, true);

					rawStream << uint8_t(frag.fragment.frameType);
					rawStream << uint8_t(frag.fragment.frameFlags);

					rawStream << frag.fragment.packetId;
					rawStream << frag.fragment.fragmentIndex;
					rawStream << frag.fragment.numFragments;
					rawStream << frag.fragment.fragmentSize;
					rawStream.write(frag.fragment.fragmentPayload, frag.fragment.fragmentSize);
				}
				else
				{
					e2::RawMemoryStream rawStream(fragBuff, fullSize, true);

					rawStream << uint8_t(frag.fragment.frameType);
					rawStream << uint8_t(frag.fragment.frameFlags);
				}

				sendto(manager->platformSocket(), reinterpret_cast<char*>(fragBuff), fullSize, 0, (SOCKADDR*)&addr, sizeof(addr));
			}
#endif
		}
	}
}










e2::NetworkManager::NetworkManager(Engine* owner)
	: e2::Manager(owner)
{
}

e2::NetworkManager::~NetworkManager()
{
}

void e2::NetworkManager::disable()
{
	// @todo send disconnect to server if client, or all clients if server



	if (m_thread.joinable())
	{
		m_killFlag = true;
		m_thread.join();
	}

#if defined(_WIN64)
	if (m_socket != INVALID_SOCKET)
	{

		for (auto &[address, otherState] : m_peerStates)
		{
			uint8_t discFrame[] = { uint8_t(e2::NetFrameType::Disconnect), uint8_t(e2::NetFrameFlags::Default)};
			sockaddr_in otherAddr;
			otherAddr.sin_family = AF_INET;
			otherAddr.sin_addr.s_addr = address.address;
			otherAddr.sin_port = address.port;
			sendto(m_socket, reinterpret_cast<char*>(discFrame), 2, 0, (SOCKADDR*) & otherAddr, sizeof(otherAddr));
		}

		closesocket(m_socket);
		m_socket = INVALID_SOCKET;
	}	
#endif


	for (auto& [address, state] : m_peerStates)
	{
		for (auto& [packetId, recvState] : state->receivingPackets)
		{
			if (recvState.packetData)
				delete[] recvState.packetData;
			if (recvState.fragmentsReceived)
				delete[] recvState.fragmentsReceived;
		}
		e2::destroy(state);
	}

	m_peerStates.clear();
	m_incomingBuffer.clear();
	m_outgoingBuffer.clear();

	m_state = e2::NetworkState::Offline;
}

void e2::NetworkManager::startServer(uint16_t port)
{
	disable();

	m_hostPort = port;

#if defined(_WIN64)
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		LogError("socket failed: {}", WSAGetLastError());
		m_state = e2::NetworkState::Offline;
		return;
	}

	u_long mode = 1; // 1 = non-blocking, 0 = blocking
	ioctlsocket(m_socket, FIONBIO, &mode);

	m_recvAddr.sin_family = AF_INET;
	m_recvAddr.sin_port = htons(m_hostPort);
	m_recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int32_t result = bind(m_socket, (SOCKADDR*)&m_recvAddr, sizeof(m_recvAddr));
	if (result != 0)
	{
		LogError("bind failed: {}", WSAGetLastError());
		closesocket(m_socket);
		m_state = e2::NetworkState::Offline;
		return;
	}
#endif

	m_state = e2::NetworkState::Server;

	m_killFlag = false;
	m_thread = std::thread(::s_networkThread, this);
}

void e2::NetworkManager::tryConnect(e2::NetPeerAddress const& address)
{
	disable();
	m_serverAddress = address;

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = address.port;
	addr.sin_addr.s_addr = address.address;


	uint32_t serverIp = addr.sin_addr.s_addr;
	uint16_t serverPort = ntohs(addr.sin_port);

	LogNotice("Server address: {}.{}.{}.{}:{}", uint8_t(serverIp), uint8_t(serverIp >> 8), uint8_t(serverIp >> 16), uint8_t(serverIp >> 24), serverPort);


	m_state = e2::NetworkState::Client_Connecting;

#if defined(_WIN64)
	m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (m_socket == INVALID_SOCKET)
	{
		LogError("socket failed: {}", WSAGetLastError());
		m_state = e2::NetworkState::Offline;
		return;
	}

	u_long mode = 1; // 1 = non-blocking, 0 = blocking
	ioctlsocket(m_socket, FIONBIO, &mode);

	uint8_t sendTo[] = { (uint8_t)e2::NetFrameType::TryConnect, (uint8_t)e2::NetFrameFlags::Default };
	int32_t bytesSent = sendto(m_socket, reinterpret_cast<char*>(sendTo), 2, 0, (SOCKADDR*)&addr, sizeof(addr));
	if (bytesSent == SOCKET_ERROR)
	{
		LogError("sendto failed: {}", WSAGetLastError());
		disable();
		return;
	}
	else
	{
		LogNotice("Sent {} bytes", bytesSent);
	}


	struct sockaddr_in localAddr;
	int localAddrLen = sizeof(localAddr);

	if (getsockname(m_socket, (struct sockaddr*)&localAddr, &localAddrLen) == SOCKET_ERROR)
	{
		LogError("getsockname failed: {}", WSAGetLastError());
	}
	else
	{
		uint32_t localIp = localAddr.sin_addr.s_addr;
		uint16_t localPort = ntohs(localAddr.sin_port);

		LogNotice("Local address: {}.{}.{}.{}:{}", uint8_t(localIp), uint8_t(localIp >> 8), uint8_t(localIp >> 16), uint8_t(localIp >> 24), localPort);
	}


#endif




	m_killFlag = false;
	m_thread = std::thread(::s_networkThread, this);

	// @todo send tryconnect 
	m_timesTriedConnect = 0;
	m_timeSinceTryConnect = e2::timeNow();
}

void e2::NetworkManager::notifyServerDisconnected()
{
	m_disconnectFlag = true;
}

void e2::NetworkManager::flagConnected()
{
	if (m_state == e2::NetworkState::Client_Connecting)
	{
		m_state = e2::NetworkState::Client_Connected;

		for (auto cb : m_netCallbacks)
			cb->onConnected(m_serverAddress);
	}
}


void e2::NetworkManager::initialize()
{
#if defined(_WIN64)
	WSAStartup(MAKEWORD(2, 2), &m_wsaHandle);
#endif

}

void e2::NetworkManager::shutdown()
{
	disable();
#if defined(_WIN64)
	WSACleanup();
#endif
}

void e2::NetworkManager::preUpdate(double deltaTime)
{
}

void e2::NetworkManager::update(double deltaTime)
{
	if (m_state != e2::NetworkState::Offline)
	{
		if (m_state == e2::NetworkState::Client_Connecting)
		{
			if (m_timeSinceTryConnect.durationSince().seconds() > 1.0)
			{
				m_timeSinceTryConnect = e2::timeNow();
				if (++m_timesTriedConnect > 4)
				{
					disable();
					return;
				}

				e2::NetSendFragment tryConnectFragment;
				tryConnectFragment.address = m_serverAddress;
				tryConnectFragment.fragment.frameType = e2::NetFrameType::TryConnect;
				tryConnectFragment.fragment.frameFlags = e2::NetFrameFlags::Default;
				tryConnectFragment.fragment.fragmentIndex = 0;
				tryConnectFragment.fragment.fragmentSize = 0;
				tryConnectFragment.fragment.numFragments = 0;
				tryConnectFragment.fragment.packetId = 0;
				std::scoped_lock lock(m_syncMutex);
				m_outgoingBuffer.push(tryConnectFragment);
			}
		}

		thread_local e2::StackVector<e2::NetPacket, e2::maxBufferedPackets> swapBuffer;
		swapBuffer.clear();
		{
			std::scoped_lock lock(m_syncMutex);
			for (e2::NetPacket& packet : m_incomingBuffer)
			{
				swapBuffer.push(packet);
			}
			m_incomingBuffer.clear();
		}

		for (e2::NetPacket& packet : swapBuffer)
		{
			handleReceivedPacket(packet);
		}
	}
	
	if (m_disconnectFlag)
	{
		for (auto cb : m_netCallbacks)
			cb->onDisconnected(m_serverAddress);

		disable();
		m_disconnectFlag = false;
	}
}

e2::NetPeerState *e2::NetworkManager::getOrCreatePeerState(NetPeerAddress const& peerAddress)
{
	std::scoped_lock lock(m_syncMutex);
	auto stateIt = m_peerStates.find(peerAddress);
	if (stateIt == m_peerStates.end())
	{
		e2::NetPeerState *newState = e2::create<e2::NetPeerState>();
		newState->peerAddress = peerAddress;
		m_peerStates[peerAddress] = newState;

	}

	return m_peerStates[peerAddress];
}

void e2::NetworkManager::notifyClientDisconnected(NetPeerAddress const& address)
{
	std::scoped_lock lock(m_syncMutex);
	auto stateIt = m_peerStates.find(address);
	if (stateIt != m_peerStates.end())
	{
		e2::NetPeerState* state = stateIt->second;

		for (auto& [packetId, recvState] : state->receivingPackets)
		{
			if (recvState.packetData)
				delete[] recvState.packetData;
			if (recvState.fragmentsReceived)
				delete[] recvState.fragmentsReceived;
		}
		e2::destroy(state);
		m_peerStates.erase(address);
	}

	if (m_state == e2::NetworkState::Server)
	{
		for (auto cb : m_netCallbacks)
			cb->onPeerDisconnected(address);
	}
}

void e2::NetworkManager::notifyClientConnected(NetPeerAddress const& address)
{
	if (m_state == e2::NetworkState::Server)
	{
		for (auto cb : m_netCallbacks)
			cb->onPeerConnected(address);
	}
}

void e2::NetworkManager::fetchOutgoing(e2::StackVector<e2::NetSendFragment, e2::maxBufferedPackets>& outSwap)
{
	std::scoped_lock lock(m_syncMutex);
	for (e2::NetSendFragment& frag : m_outgoingBuffer)
	{
		outSwap.push(frag);
	}
	m_outgoingBuffer.clear();
	
}

void e2::NetworkManager::receivePacket(e2::NetPacket& newPacket)
{
	std::scoped_lock lock(m_syncMutex);
	if (m_incomingBuffer.full())
	{
		LogError("incoming packet buffer full, dropping packet");
		return;
	}
	m_incomingBuffer.push(newPacket);
}

void e2::NetworkManager::handleReceivedPacket(e2::NetPacket& packet)
{
	uint8_t const* id = packet.data.read(1);
	if (m_handlers[*id])
	{
		m_handlers[*id](engine(), packet);
	}
}


void e2::NetworkManager::broadcastPacket(e2::HeapStream& packet)
{
	if (!isServer())
	{
		LogError("tried to broadcast from client, refusing");
		return;
	}

	packet.seek(0);
	uint64_t packetSize = packet.remaining();
	uint8_t const* packetData = packet.read(packetSize);
	packet.seek(0);

	for (auto& [address, state] : m_peerStates)
	{
		e2::NetPacket netPacket(address, packetData, packetSize); 
		sendPacket(netPacket);
	}
}

void e2::NetworkManager::sendPacket(e2::NetPacket& outPacket)
{
	outPacket.data.seek(0);
	uint64_t size = outPacket.data.remaining();
	if (size > e2::maxFragmentPayloadSize)
	{
		LogError("We do not support sending multi frag packets yet @todo");
		return;
	}

	e2::NetSendFragment sendFragment;

	sendFragment.address = outPacket.peerAddress;
	sendFragment.fragment.frameType = e2::NetFrameType::DataFragment;
	sendFragment.fragment.frameFlags = e2::NetFrameFlags::Default;
	sendFragment.fragment.fragmentIndex = 0;
	sendFragment.fragment.numFragments = 1;
	sendFragment.fragment.fragmentSize = size;
	memcpy(sendFragment.fragment.fragmentPayload, outPacket.data.read(size), size);

	std::scoped_lock lock(m_syncMutex);
	if (m_outgoingBuffer.full())
	{
		LogError("outgoing packet buffer full, dropping packet");
		return;
	}
	e2::NetPeerState* peerState = getOrCreatePeerState(outPacket.peerAddress);
	sendFragment.fragment.packetId = peerState->outgoingPacketIndex;
	m_outgoingBuffer.push(sendFragment);
}

void e2::NetworkManager::registerHandler(uint8_t handlerId, PacketHandlerCallback handler)
{
	m_handlers[handlerId] = handler;
}

void e2::NetworkManager::registerNetCallbacks(e2::NetCallbacks* cb)
{
	m_netCallbacks.insert(cb);
}

void e2::NetworkManager::unregisterNetCallbacks(e2::NetCallbacks* cb)
{
	m_netCallbacks.erase(cb);
}

e2::NetPacket::NetPacket(e2::NetPeerAddress const& address, uint8_t const* packetData, uint64_t packetSize)
	: peerAddress(address)
{
	data.write(packetData, packetSize);
	data.seek(0);
}
