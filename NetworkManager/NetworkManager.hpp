#ifndef HELENA_SYSTEMS_NETWORKMANAGER_HPP
#define HELENA_SYSTEMS_NETWORKMANAGER_HPP

#define ENET_IMPLEMENTATION

#include <Helena/Engine/Events.hpp>
#include <enet/enet.h>
#include <string>
#include <list>
#include <memory>
#include <type_traits>

namespace Helena::Systems
{
    class NetworkManager 
    {

    public:
        enum class EStateConnection : std::uint8_t {
            Disconnected,
            Disconnecting,
            Connecting,
            Handshake,
            Connected
        };

        enum class EStateEvent : std::uint8_t {
            Disconnect,
            Timeout,
            Connect
        };

        enum class EResetConnection : std::uint8_t {
            Default,    // Disconnect only after all queued outgoing packets are sent
            Update,     // Disconnect only after call Update (packets will not be sent)
            Force,      // Disconnect without notify (packets and data will not be sent )
            Now         // Disconnect (all queued outgoing packets are sent now)
        };

        enum class EMessage : std::uint8_t
        {
            None,           // Not reliable and not sequenced
            Reliable,       // Reliable and sequenced packets
            Fragmented,     // Not reliable if packet size > MTU and sequenced
            Unsequenced     // Not reliable and not sequenced
        };

        class Network;
        class UserData;

    private:
        struct Event {};
        struct Message {};

        class Session
        {
        public:
            Session() : m_UserData{}, m_HandshakeKey{}, m_State{}, m_Sequence{} {}
            ~Session() = default;
            Session(const Session&) = delete;
            Session(Session&&) noexcept = default;
            Session& operator=(const Session&) = delete;
            Session& operator=(Session&&) noexcept = default;

            std::unique_ptr<UserData> m_UserData;
            std::int64_t m_HandshakeKey;
            EStateConnection m_State;
            std::uint8_t m_Sequence;
        };

    public:
        class Config {
        public:
            Config(std::string_view ip, std::uint16_t port, std::uint16_t peers, std::uint8_t channels, std::uint32_t data = 0,
                std::uint32_t bandwidthIn = 0, std::uint32_t bandwidthOut = 0, std::uint32_t bufferSize = ENET_HOST_BUFFER_SIZE_MAX)
                : m_IP{ip}, m_Port{port}, m_Peers{peers}, m_Channels{channels}, m_Data{data}
                , m_BandwidthIn{bandwidthIn}, m_BandwidthOut{bandwidthOut}, m_BufferSize{bufferSize} {}
            ~Config() = default;
            Config(const Config&) = default;
            Config(Config&&) noexcept = default;
            Config& operator=(const Config&) = default;
            Config& operator=(Config&&) noexcept = default;

            void SetIP(const std::string_view ip) {
                m_IP = ip;
            }

            void SetPort(std::uint16_t port) noexcept {
                m_Port = port;
            }

            void SetPeers(std::uint16_t peers) noexcept {
                m_Peers = peers;
            }

            void SetChannels(std::uint8_t channels) noexcept {
                m_Channels = channels;
            }

            void SetData(std::uint32_t data) noexcept {
                m_Data = data;
            }

            void SetBandwidthIn(std::uint32_t size) noexcept {
                m_BandwidthIn = size;
            }

            void SetBandwidthOut(std::uint32_t size) noexcept {
                m_BandwidthOut = size;
            }

            void SetBufferSize(std::uint32_t size) noexcept {
                m_BufferSize = size;
            }

            [[nodiscard]] const std::string_view GetIP() const noexcept {
                return m_IP;
            }

            [[nodiscard]] std::uint16_t GetPort() const noexcept {
                return m_Port;
            }

            [[nodiscard]] std::uint16_t GetPeers() const noexcept {
                return m_Peers;
            }

            [[nodiscard]] std::uint8_t GetChannels() const noexcept {
                return m_Channels;
            }

            [[nodiscard]] std::uint32_t GetData() const noexcept {
                return m_Data;
            }

            [[nodiscard]] std::uint32_t GetBandwidthIn() const noexcept {
                return m_BandwidthIn;
            }

            [[nodiscard]] std::uint32_t GetBandwidthOut() const noexcept {
                return m_BandwidthOut;
            }

            [[nodiscard]] std::uint32_t GetBufferSize() const noexcept {
                return m_BufferSize;
            }

        private:
            std::string     m_IP;
            std::uint16_t   m_Port;
            std::uint16_t   m_Peers;
            std::uint8_t    m_Channels;
            std::uint32_t   m_Data;
            std::uint32_t   m_BandwidthIn;
            std::uint32_t   m_BandwidthOut;
            std::uint32_t   m_BufferSize;
        };

        class UserData {
        public:
            UserData() = default;
            virtual ~UserData() = default;
            UserData(const UserData&) = default;
            UserData(UserData&&) noexcept = default;
            UserData& operator=(const UserData&) = default;
            UserData& operator=(UserData&&) noexcept = default;
        };

        class Connection 
        {
            friend class NetworkManager;

            [[nodiscard]] std::uint8_t GetSequenceID() noexcept;
            [[nodiscard]] bool Validate() const noexcept;

        public:
            Connection() : m_Net{}, m_Peer{}, m_SequenceID{} {}
            Connection(Network* net, ENetPeer* peer) : m_Net{net}, m_Peer{peer}, m_SequenceID{GetSequenceID()} {}
            ~Connection() = default;
            Connection(const Connection&) = default;
            Connection(Connection&&) noexcept = default;
            Connection& operator=(const Connection&) = default;
            Connection& operator=(Connection&&) noexcept = default;

            void Send(EMessage type, std::uint8_t channel, const std::uint8_t* data, std::uint32_t size) const; 

            void Disconnect(EResetConnection flag, std::uint32_t data = 0);

            [[nodiscard]] Network& GetNetwork() noexcept;
            [[nodiscard]] const Network& GetNetwork() const noexcept;

            void SetUserData(std::unique_ptr<UserData> data);

            template <typename T>
            requires std::is_base_of_v<NetworkManager::UserData, T>
            [[nodiscard]] T* GetUserData() noexcept;

            template <typename T>
            requires std::is_base_of_v<NetworkManager::UserData, T>
            [[nodiscard]] const T* GetUserData() const noexcept;

            [[nodiscard]] std::uint32_t GetID() const noexcept;

            [[nodiscard]] EStateConnection GetState() const noexcept;

            [[nodiscard]] bool Valid() const noexcept;

        private:
            Network* m_Net;
            ENetPeer* m_Peer;
            std::uint8_t m_SequenceID;
        };

        class Network
        {
            friend class NetworkManager;

        public:
            Network(std::uint16_t id);
            ~Network();
            Network(const Network&) = delete;
            Network(Network&&) noexcept;
            Network& operator=(const Network&) = delete;
            Network& operator=(Network&&) noexcept;

            [[nodiscard]] bool CreateServer(const Config& config);
            [[nodiscard]] bool CreateClient(const Config& config);

            void Shutdown();

            void Broadcast(EMessage type, std::uint8_t channel, const std::uint8_t* data, std::uint32_t size) const;

            [[nodiscard]] std::uint16_t GetID() const noexcept;

            void SetUserData(std::unique_ptr<UserData> data);

            template <typename T>
            requires std::is_base_of_v<NetworkManager::UserData, T>
            [[nodiscard]] T* GetUserData() noexcept;

            template <typename T>
            requires std::is_base_of_v<NetworkManager::UserData, T>
            [[nodiscard]] const T* GetUserData() const noexcept;

            [[nodiscard]] bool Server() const noexcept;
            [[nodiscard]] bool Valid() const noexcept;

            template <typename Func>
            void Each(Func&& func);

            template <typename Func>
            void Each(Func&& func) const;

        private:
            [[nodiscard]] static bool CreateAddress(ENetAddress& address, const std::string_view ip, std::uint16_t port);
            [[nodiscard]] static ENetHost* CreateHost(const Config& config, bool isServer);
            [[nodiscard]] static std::int64_t Scramble(std::int64_t nInput) noexcept;
            [[nodiscard]] static bool SendHandshake(ENetPeer* peer, std::int64_t key);

            void AddHandshake(ENetPeer* peer);
            void RemoveHandshake(ENetPeer* peer);

            void Update(std::uint32_t timeout = 0, std::uint32_t eventsLimit = 100);

        private:
            ENetHost* m_Host;
            std::list<Connection> m_HandshakeList;
            std::unique_ptr<UserData> m_UserData;
            std::uint16_t m_NetworkID;
            bool m_Server;
            bool m_Initialized;
        };

    public:
        NetworkManager();
        ~NetworkManager();
        NetworkManager(const NetworkManager&) = delete;
        NetworkManager(NetworkManager&&) noexcept = delete;
        NetworkManager& operator=(const NetworkManager&) = delete;
        NetworkManager& operator=(NetworkManager&&) noexcept = delete;

        [[nodiscard]] Network& CreateNetwork();
        void RemoveNetwork(std::uint16_t id) noexcept;

        [[nodiscard]] Network* GetNetwork(std::uint16_t id) noexcept;
        [[nodiscard]] const Network* GetNetwork(std::uint16_t id) const noexcept;

        [[nodiscard]] std::size_t Count() const noexcept;

        // Iterators
        [[nodiscard]] auto begin() noexcept;
        [[nodiscard]] auto begin() const noexcept;
        [[nodiscard]] auto end() noexcept;
        [[nodiscard]] auto end() const noexcept;
        
    private:
        void Tick(const Helena::Events::Engine::Tick ev);
        static void Service(Network& net, std::uint32_t timeout);

    private:
        std::list<Network> m_Networks;
        std::uint16_t m_NetworkSequenceID;
        bool m_Initialized;
    };
}

namespace Helena::Events::NetworkManager
{
    struct Event {
        Systems::NetworkManager::Connection connection;
        std::uint32_t data;
        Systems::NetworkManager::EStateEvent type;
    };

    struct Message {
        Systems::NetworkManager::Connection connection;
        std::uint8_t* data;
        std::uint32_t size;
        Systems::NetworkManager::EMessage type;
        std::uint8_t channel;
    };
}

#include "NetworkManager.ipp"

#endif // HELENA_SYSTEMS_NETWORKMANAGER_HPP
