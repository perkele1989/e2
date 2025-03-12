
#include "platformer/platformer.hpp"

#include "e2/e2.hpp"
#include "e2/managers/rendermanager.hpp"
#include "e2/managers/asyncmanager.hpp"
#include "e2/managers/typemanager.hpp"
#include "e2/managers/assetmanager.hpp"
#include "e2/managers/networkmanager.hpp"
#include "e2/managers/audiomanager.hpp"
#include "e2/managers/uimanager.hpp"
#include "e2/game/gamesession.hpp"
#include "e2/renderer/renderer.hpp"
#include "e2/transform.hpp"
#include "e2/buffer.hpp"

#include "e2/renderer/shadermodels/lightweight.hpp"

#include <glm/gtx/intersect.hpp>
#include <glm/gtx/vector_angle.hpp>

#include <glm/gtc/noise.hpp>
#include <glm/gtx/spline.hpp>
#include <glm/gtx/easing.hpp>

#include <fmod_errors.h>

#pragma warning(disable : 4996)

#include <filesystem>
#include <Shlobj.h>
#include <ctime>
#include <Ws2tcpip.h>

#include <random>
#include <algorithm>


namespace
{
	// client -> server
	void _handleClientFetchGameInfo(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isServer())
		{
			return;
		}

		e2::NetPacket response;
		response.peerAddress = packet.peerAddress;
		response.data << e2::packetId_ServerGameInfo;
		response.data << (uint8_t)game->serverManager.players.size();
		for (auto& [id, player]: game->serverManager.players)
		{
			response.data << player.name;
		}
		net->sendPacket(response);
	}

	// client -> server
	void _handleClientInfo(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isServer())
		{
			return;
		}

		e2::Name name;
		packet.data >> name;

		LogNotice("Player \"{}\" joined", name);

		game->serverManager.players[name].name = name;
		game->serverManager.players[name].address = packet.peerAddress;
		game->serverManager.players[name].position = { 300.0f, 500.0f };
	}

	// client -> server
	void _handleClientInput(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isServer())
		{
			return;
		}

		e2::Name name;
		packet.data >> name;

		uint64_t clientFrameIndex{};
		packet.data >> clientFrameIndex;

		if (!game->serverManager.players.contains(name))
			return;

		e2::ServerPlayerState &playerState = game->serverManager.players.at(name);
		if (playerState.lastClientFrameIndex >= clientFrameIndex)
			return;

		packet.data >> playerState.buttonMoveLeft;
		packet.data >> playerState.buttonMoveRight;
		packet.data >> playerState.buttonJump;

		playerState.lastClientFrameIndex = clientFrameIndex;
	}

	// server -> client
	void _handleServerGameInfo(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isClient())
		{
			return;
		}

		uint8_t numPlayers{};
		packet.data >> numPlayers;

		LogNotice("Server: {} players", numPlayers);

		for (uint8_t i = 0; i < numPlayers; i++)
		{
			e2::Name playerName;
			packet.data >> playerName;

			LogNotice("Server: Player {}: {}", i, playerName);
		}
		// @todo actually do somethign with the data
	}

	// server -> client
	void _handleServerPlayerLeft(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isClient())
		{
			return;
		}

		e2::Name whoLeft;
		packet.data >> whoLeft;

		LogNotice("Server: player {} left", whoLeft);

		game->clientManager.players.erase(whoLeft);
	}

	// server -> client
	void _handleServerGameEnd(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isClient())
		{
			return;
		}

		LogNotice("Server ended game");
	}

	// server -> client
	void _handleServerFrame(e2::Engine* engine, e2::NetPacket& packet)
	{
		e2::NetworkManager* net = engine->networkManager();
		e2::Platformer* game = static_cast<e2::Platformer*>(engine->application());

		if (!net->isClient())
		{
			return;
		}

		uint64_t serverFrameIndex{};
		packet.data >> serverFrameIndex;

		uint8_t numPlayers{};
		packet.data >> numPlayers;

		for (uint8_t i = 0; i < numPlayers; i++)
		{
			e2::Name name;
			glm::vec2 pos, vel;
			packet.data >> name >> pos >> vel;
			bool newPlayer = false;
			if (!game->clientManager.players.contains(name))
			{
				game->clientManager.players[name].name = name;
				newPlayer = true;
			}

			e2::ClientPlayerState& state = game->clientManager.players.at(name);
			if (!newPlayer && state.lastServerFrameIndex >= serverFrameIndex)
				continue;

			state.position = pos;
			state.velocity = vel;
			state.lastServerFrameIndex = serverFrameIndex;
		}

	}
}

e2::Platformer::Platformer(e2::Context* ctx)
	: e2::Application(ctx)
{
	
}

e2::Platformer::~Platformer()
{
	
}

void e2::Platformer::onServerStarted()
{

}

void e2::Platformer::onServerStopped()
{
	serverManager = ServerManager();
}

void e2::Platformer::onConnected(e2::NetPeerAddress const& address)
{
	LogNotice("Sending client info to {}.{}.{}.{}:{}", uint8_t(address.address), uint8_t(address.address >> 8), uint8_t(address.address >> 16), uint8_t(address.address>> 24), address.port);
	e2::NetPacket clientInfoPacket;
	clientInfoPacket.peerAddress = address;
	clientInfoPacket.data << e2::packetId_ClientInfo;
	clientInfoPacket.data << clientManager.localPlayerName;
	networkManager()->sendPacket(clientInfoPacket);
}

void e2::Platformer::onDisconnected(e2::NetPeerAddress const& address)
{
	clientManager = ClientManager();
}

void e2::Platformer::onPeerConnected(e2::NetPeerAddress const& address)
{

}

void e2::Platformer::onPeerDisconnected(e2::NetPeerAddress const& address)
{
	for (auto& [name, state] : serverManager.players)
	{
		e2::Name nameCopy = name;
		if (state.address == address)
		{
			serverManager.players.erase(nameCopy);

			e2::HeapStream notify;
			notify << e2::packetId_ServerPlayerLeft;
			notify << nameCopy;
			networkManager()->broadcastPacket(notify);
			return;
		}
	}

}

void e2::Platformer::initialize()
{
	e2::WindowCreateInfo winCreateInfo{};
	winCreateInfo.title = "NetworkPlatformTest";
	winCreateInfo.resizable = true;
	//winCreateInfo.mode = WindowMode::Fullscreen;
	m_window = renderManager()->mainThreadContext()->createWindow(winCreateInfo);
	//m_window->setFullscreen(true);

	//m_window->source(m_renderer->colorTarget());


	m_uiContext = e2::create<e2::UIContext>(this, m_window);
	m_window->registerInputHandler(m_uiContext);
	m_window->source(m_uiContext->colorTexture());

	renderManager()->registerCallbacks(this);

	networkManager()->registerHandler(e2::packetId_ClientFetchGameInfo, _handleClientFetchGameInfo);
	networkManager()->registerHandler(e2::packetId_ClientInput, _handleClientInput);
	networkManager()->registerHandler(e2::packetId_ClientInfo, _handleClientInfo);

	networkManager()->registerHandler(e2::packetId_ServerGameInfo, _handleServerGameInfo);
	networkManager()->registerHandler(e2::packetId_ServerFrame, _handleServerFrame);
	networkManager()->registerHandler(e2::packetId_ServerGameEnd, _handleServerGameEnd);
	networkManager()->registerHandler(e2::packetId_ServerPlayerLeft, _handleServerPlayerLeft);

	std::string randName;
	for (uint8_t i = 0; i < 8; i++)
	{
		randName.push_back((char)e2::randomInt(65, 90));
	}
	clientManager.localPlayerName = randName;
	LogNotice("Local name: {}", clientManager.localPlayerName);

	networkManager()->registerNetCallbacks(this);
}

void e2::Platformer::shutdown()
{
	networkManager()->unregisterNetCallbacks(this);
	renderManager()->unregisterCallbacks(this);

	m_window->unregisterInputHandler(m_uiContext);
	e2::destroy(m_uiContext);

	e2::destroy(m_window);
}

void e2::Platformer::update(double seconds)
{

	if (m_window->wantsClose())
	{
		engine()->shutdown();
	}

	uiContext()->setClientArea({}, m_window->size());
	auto ui = m_uiContext;
	auto& kb = ui->keyboardState();

	auto net = engine()->networkManager();
	e2::NetworkState netState = net->currentState();
	e2::NetPeerAddress clientServerAddress = net->serverAddress();
	uint16_t serverPort = ntohs(clientServerAddress.port);
	uint32_t serverAddress = clientServerAddress.address;

	auto nativeAddress = net->platformAddress();
	e2::NetPeerAddress platformAddress;
	platformAddress.address = nativeAddress.sin_addr.s_addr;
	platformAddress.port = nativeAddress.sin_port;

	uint16_t hostPort = ntohs(platformAddress.port);
	uint32_t hostAddress = platformAddress.address;


	clientManager.buttonMoveLeft = kb.state(e2::Key::Left);
	clientManager.buttonMoveRight = kb.state(e2::Key::Right);
	clientManager.buttonJump = kb.state(e2::Key::Space);
	
	if (netState == e2::NetworkState::Client_Connected)
	{
		if (clientManager.lastSentInput.durationSince().seconds() >= 1.0 / clientManager.inputTickRate)
		{
			e2::NetPacket inputFrame;
			inputFrame.peerAddress = clientServerAddress;
			inputFrame.data << e2::packetId_ClientInput;

			inputFrame.data << clientManager.localPlayerName;
			inputFrame.data << clientManager.frameIndex;

			inputFrame.data << clientManager.buttonMoveLeft;
			inputFrame.data << clientManager.buttonMoveRight;
			inputFrame.data << clientManager.buttonJump;

			net->sendPacket(inputFrame);

			clientManager.lastSentInput = e2::timeNow();
			clientManager.frameIndex++;
		}


		for (auto& [name, state] : clientManager.players)
		{
			// render client players
			bool localPlayer = (clientManager.localPlayerName == name);

			ui->drawQuad(state.position - glm::vec2{ 8.0f, 8.0f }, { 16.0f, 16.0f }, 0xFF0000FF);
		}
	}
	else if (netState == e2::NetworkState::Server)
	{
		if (serverManager.lastSentFrame.durationSince().seconds() >= 1.0 / serverManager.frameTickRate)
		{
			e2::HeapStream frame;

			frame << e2::packetId_ServerFrame;
			frame << serverManager.frameIndex;
			frame << uint8_t(serverManager.players.size());

			for (auto& [id, state] : serverManager.players)
			{
				frame << state.name << state.position << state.velocity;
			}

			net->broadcastPacket(frame);

			serverManager.lastSentFrame = e2::timeNow();
			serverManager.frameIndex++;
		}

		for (auto& [name, state] : serverManager.players)
		{
			// update and render server players
			bool localPlayer = (clientManager.localPlayerName == name);

			if (localPlayer)
			{
				state.buttonMoveLeft = clientManager.buttonMoveLeft;
				state.buttonMoveRight = clientManager.buttonMoveRight;
			}

			if (state.buttonMoveLeft)
				state.velocity.x = -100.0f;
			else if (state.buttonMoveRight)
				state.velocity.x = 100.0f;
			else
				state.velocity.x = 0.0f;

			state.position += state.velocity * float(seconds);

			ui->drawQuad(state.position - glm::vec2{ 8.0f, 8.0f }, { 16.0f, 16.0f }, 0xFF0000FF);
		}

	}







	std::string netStatus;
	if (netState == e2::NetworkState::Offline)
	{
		netStatus = "Offline";
	}
	else if (netState == e2::NetworkState::Server)
	{
		netStatus = std::format("[Server] Hosted at {}.{}.{}.{} on port {}", uint8_t(hostAddress), uint8_t(hostAddress >> 8), uint8_t(hostAddress >> 16), uint8_t(hostAddress >> 24) , hostPort);
	}
	else if (netState == e2::NetworkState::Client_Connecting)
	{
		netStatus = std::format("[Client] Establishing connection to {}.{}.{}.{} on port {}...", uint8_t(serverAddress), uint8_t(serverAddress >> 8), uint8_t(serverAddress >> 16) , uint8_t(serverAddress >> 24) , serverPort);
	}
	else if (netState == e2::NetworkState::Client_Connected)
	{
		netStatus = std::format("[Client] Connected to {}.{}.{}.{} on port {}", uint8_t(serverAddress), uint8_t(serverAddress >> 8), uint8_t(serverAddress >> 16) , uint8_t(serverAddress >> 24) , serverPort);
	}
	e2::UIStyle& style = uiManager()->workingStyle();
	ui->drawQuadFancy({ 4.0f, 4.0f }, { 500.f, 200.f }, style.windowBgColor, 8.0f, 1.0f, true);
	ui->pushFixedPanel("panpan", { 4.0f, 4.0f }, { 500.f, 200.f });
	ui->beginStackV("mainstack");
	ui->label("status", netStatus);
	if (netState == e2::NetworkState::Offline)
	{
		if (ui->button("hostBtn", "Host"))
		{
			net->startServer(1337);
			clientManager.joinedGame = true;
			serverManager.players[clientManager.localPlayerName].name = clientManager.localPlayerName;
			serverManager.players[clientManager.localPlayerName].position = {200.0f, 500.0f};
		}
		if (ui->button("joinBtn", "Join"))
		{
			e2::NetPeerAddress joinAddress;


			addrinfo hints{}, *res;
			hints.ai_family = AF_INET; // IPv4
			if (getaddrinfo("game.prklinteractive.com", nullptr, &hints, &res) != 0)
				LogError("failed to resolve host");

			sockaddr_in sockaddr = *reinterpret_cast<sockaddr_in*>(res->ai_addr);


			joinAddress.address = sockaddr.sin_addr.s_addr;
			joinAddress.port = htons(1337);
			net->tryConnect(joinAddress);
		}
	}
	else if (netState == e2::NetworkState::Server)
	{
		if (ui->button("discBtn", "Disconnect"))
		{
			net->disable();
			clientManager.joinedGame = false;
		}

	}
	else if (netState == e2::NetworkState::Client_Connecting)
	{
		if (ui->button("discBtn", "Cancel"))
		{
			net->disable();
		}
	}
	else if (netState == e2::NetworkState::Client_Connected)
	{
		if (ui->button("discBtn", "Disconnect"))
		{
			net->disable();

		}
	}
	ui->endStackV();
	ui->popFixedPanel();


}

e2::ApplicationType e2::Platformer::type()
{
	return e2::ApplicationType::Game;
}

void e2::Platformer::onNewFrame(uint8_t frameIndex)
{
}

void e2::Platformer::preDispatch(uint8_t frameIndex)
{
}

void e2::Platformer::postDispatch(uint8_t frameIndex)
{
	m_window->present();
}
