
#pragma once 



#include <e2/application.hpp>
#include <e2/managers/rendermanager.hpp>
#include <e2/ui/uicontext.hpp>
#include <e2/rhi/window.hpp>
#include <e2/managers/networkmanager.hpp>

namespace e2
{
	constexpr uint64_t maxNetPlayers = 8;
	constexpr uint8_t packetId_ClientFetchGameInfo = 0;
	constexpr uint8_t packetId_ClientInput = 1;
	constexpr uint8_t packetId_ClientInfo = 2;
	constexpr uint8_t packetId_ServerGameInfo = 128;
	constexpr uint8_t packetId_ServerFrame = 129;
	constexpr uint8_t packetId_ServerGameEnd = 130;
	constexpr uint8_t packetId_ServerPlayerLeft = 131;


	struct ServerPlayerState
	{
		e2::Name name;
		e2::NetPeerAddress address;

		uint64_t lastClientFrameIndex{};

		// input states
		bool buttonMoveLeft{};
		bool buttonMoveRight{};
		bool buttonJump{};

		// world states 
		glm::vec2 position{};
		glm::vec2 velocity{};
	};


	struct ClientPlayerState
	{
		e2::Name name;
		uint64_t lastServerFrameIndex{};

		// world states 
		glm::vec2 position{};
		glm::vec2 velocity{};
	};

	struct ServerManager
	{
		uint64_t frameIndex;
		double frameTickRate = 120.0;
		e2::Moment lastSentFrame;

		std::unordered_map<e2::Name, ServerPlayerState> players;
	};

	struct ClientManager
	{
		uint64_t frameIndex;

		bool joinedGame{};
		e2::Name localPlayerName;
		bool buttonMoveLeft{};
		bool buttonMoveRight{};
		bool buttonJump{};

		double inputTickRate = 120.0;
		e2::Moment lastSentInput;

		std::unordered_map<e2::Name, ClientPlayerState> players;
	};


	class Platformer : public e2::Application, public e2::RenderCallbacks, public e2::NetCallbacks
	{
	public:
		Platformer(e2::Context* ctx);
		virtual ~Platformer();



		virtual void onServerStarted() override;
		virtual void onServerStopped() override;
		virtual void onConnected(e2::NetPeerAddress const& address) override;
		virtual void onDisconnected(e2::NetPeerAddress const& address) override;
		virtual void onPeerConnected(e2::NetPeerAddress const& address) override;
		virtual void onPeerDisconnected(e2::NetPeerAddress const& address) override;


		virtual void initialize() override;
		virtual void shutdown() override;
		virtual void update(double seconds) override;
		virtual ApplicationType type() override;

		inline e2::IWindow* window()
		{
			return m_window;
		}

		inline e2::UIContext* uiContext()
		{
			return m_uiContext;
		}


		virtual void onNewFrame(uint8_t frameIndex) override;
		virtual void preDispatch(uint8_t frameIndex) override;
		virtual void postDispatch(uint8_t frameIndex) override;


		ServerManager serverManager;
		ClientManager clientManager;

	protected:
		e2::IWindow* m_window{};
		e2::UIContext* m_uiContext{};
	};

}

#include "platformer.generated.hpp"