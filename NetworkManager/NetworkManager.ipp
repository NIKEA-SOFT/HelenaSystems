#ifndef HELENA_SYSTEMS_NETWORKMANAGER_IPP
#define HELENA_SYSTEMS_NETWORKMANAGER_IPP

#include "NetworkManager.hpp"
#include <Helena/Engine/Engine.hpp>

#include <chrono>

namespace Helena::Systems
{
    /* ------------ [NetworkManager::Connection] ------------ */
    [[nodiscard]] inline std::uint8_t NetworkManager::Connection::GetSequenceID() noexcept {
        return static_cast<Session*>(m_Peer->data)->m_Sequence;
    }

    [[nodiscard]] inline bool NetworkManager::Connection::Validate() const noexcept {
        return m_Peer && m_Peer->data && m_SequenceID == static_cast<const Session*>(m_Peer->data)->m_Sequence;
    }

    inline void NetworkManager::Connection::Send(EMessage type, std::uint8_t channel, const std::uint8_t* data, std::uint32_t size) const
    {
        if(Valid())
        {
            const auto session = static_cast<Session*>(m_Peer->data);
            if(session->m_State != EStateConnection::Connected) {
                HELENA_MSG_WARNING("Packet cannot be sent now for connection!");
                return;
            }

            if(void* memory = enet_malloc(sizeof(ENetPacket) + size))
            {
                const auto packet = static_cast<ENetPacket*>(memory);

                switch(type)
                {
                    case EMessage::None:        packet->flags = 0; break;
                    case EMessage::Reliable:    packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE; break;
                    case EMessage::Fragmented:  packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_UNRELIABLE_FRAGMENTED; break;
                    case EMessage::Unsequenced: packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_UNSEQUENCED; break;
                }

                packet->data = static_cast<std::uint8_t*>(memory) + sizeof(ENetPacket);
                packet->referenceCount = 0;
                packet->dataLength = size;
                packet->freeCallback = nullptr;
                packet->userData = nullptr;

                std::memcpy(packet->data, data, size);
                enet_peer_send(m_Peer, channel, packet);
            }
        }
    }

    inline void NetworkManager::Connection::Disconnect(EResetConnection flag, std::uint32_t data)
    {
        if(!Valid()) {
            return;
        }

        const auto session = static_cast<Session*>(m_Peer->data);
        if(session->m_State != EStateConnection::Disconnecting && session->m_State != EStateConnection::Disconnected)
        {
            switch(flag)
            {
                case EResetConnection::Default: {
                    session->m_State = EStateConnection::Disconnecting;
                    enet_peer_disconnect_later(m_Peer, data);
                } break;
                case EResetConnection::Update: {
                    session->m_State = EStateConnection::Disconnecting;
                    enet_peer_disconnect(m_Peer, data);
                } break;
                case EResetConnection::Force: {
                    session->m_State = EStateConnection::Disconnected;
                    enet_peer_reset(m_Peer);
                } break;
                case EResetConnection::Now: {
                    session->m_State = EStateConnection::Disconnecting;
                    enet_peer_disconnect_now(m_Peer, data);
                } break;
            }
        }
    }

    [[nodiscard]] inline NetworkManager::Network& NetworkManager::Connection::GetNetwork() noexcept {
        return *m_Net;
    }

    [[nodiscard]] inline const NetworkManager::Network& NetworkManager::Connection::GetNetwork() const noexcept {
        return *m_Net;
    }

    inline void NetworkManager::Connection::SetUserData(std::unique_ptr<NetworkManager::UserData> data) {
        const auto session = static_cast<NetworkManager::Session*>(m_Peer->data);
        session->m_UserData = std::move(data);
    }

    template <typename T>
    requires std::is_base_of_v<NetworkManager::UserData, T>
    [[nodiscard]] T* NetworkManager::Connection::GetUserData() noexcept
    {
        if(Valid()) {
            const auto session = static_cast<NetworkManager::Session*>(m_Peer->data);
            return static_cast<T*>(session->m_UserData.get());   
        }

        return nullptr;
    }

    template <typename T>
    requires std::is_base_of_v<NetworkManager::UserData, T>
    [[nodiscard]] const T* NetworkManager::Connection::GetUserData() const noexcept 
    {
        if(Valid()) {
            const auto session = static_cast<const Session*>(m_Peer->data);
            return static_cast<const T*>(session->m_UserData.get());
        }

        return nullptr;
    }

    [[nodiscard]] inline std::uint32_t NetworkManager::Connection::GetID() const noexcept {
        return enet_peer_get_id(m_Peer);
    }

    [[nodiscard]] inline NetworkManager::EStateConnection NetworkManager::Connection::GetState() const noexcept {
        const auto session = static_cast<Session*>(m_Peer->data);
        return session->m_State;
    }

    [[nodiscard]] inline bool NetworkManager::Connection::Valid() const noexcept {
        return m_Net && m_Peer && m_Peer->data && m_SequenceID == static_cast<const Session*>(m_Peer->data)->m_Sequence;
    }
    

    /* -------------- [NetworkManager::Network] ------------- */
    inline NetworkManager::Network::Network(std::uint16_t id) : m_Host{}, m_HandshakeList{}, m_UserData{}, m_NetworkID{id}, m_Server{}, m_Initialized{}
    {
        if(!enet_initialize()) {
            m_Initialized = true;
        } else {
            HELENA_ASSERT(m_Initialized, "WinSock init failed");
            Helena::Engine::Shutdown("Initialize network failed");
        }
    }

    inline NetworkManager::Network::~Network() 
    {
        Shutdown();
        if(m_Initialized) {
            enet_deinitialize();
        }
    }

    inline NetworkManager::Network::Network(Network&& other) noexcept {
        m_Host = other.m_Host;
        m_UserData = std::move(other.m_UserData);
        m_NetworkID = other.m_NetworkID;
        m_Initialized = other.m_Initialized;
        m_Server = other.m_Server;

        other.m_Host = nullptr;
    }

    inline NetworkManager::Network& NetworkManager::Network::operator=(Network&& other) noexcept {
        m_Host = other.m_Host;
        m_UserData = std::move(other.m_UserData);
        m_NetworkID = other.m_NetworkID;
        m_Initialized = other.m_Initialized;
        m_Server = other.m_Server;

        other.m_Host = nullptr;
        return *this;
    }

    [[nodiscard]] inline bool NetworkManager::Network::CreateServer(const Config& config) 
    {
        if(!m_Initialized) {
            return false;
        }

        if(m_Host) {
            HELENA_MSG_ERROR("Create server with ip: {}, port: {} failed: current network already used!",
                config.GetIP(), config.GetPort());
            return false;
        }

        m_Server = true;
        m_Host = CreateHost(config, m_Server);
        return m_Host;
    }

    [[nodiscard]] inline bool NetworkManager::Network::CreateClient(const Config& config) 
    {
        if(!m_Initialized) {
            return false;
        }

        if(m_Host && m_Server) {
            HELENA_MSG_ERROR("Client connection cannot be created inside server network!");
            return false;
        }
        
        if(!m_Host) {
            m_Server = false;
            m_Host = CreateHost(config, m_Server);
        }

        if(m_Host)
        {
            ENetAddress address{};
            if(CreateAddress(address, config.GetIP(), config.GetPort()))
            {
                if(const auto peer = enet_host_connect(m_Host, &address, config.GetChannels(), config.GetData())) 
                {
                    const auto session = static_cast<Session*>(peer->data);
                    session->m_State = EStateConnection::Connecting;
                    session->m_Sequence++;
                    session->m_UserData.reset();
                    session->m_HandshakeKey = 0;
                    return true;
                } else {
                    HELENA_MSG_ERROR("Connect to server ip: {}, port: {} failed!", config.GetIP(), config.GetPort());
                }
            }
        }

        return false;
    }

    inline void NetworkManager::Network::Shutdown() 
    {
        if(Valid()) 
        {
            const auto peer = m_Host->peers;
            delete[] static_cast<Session*>(peer->data);

            enet_host_flush(m_Host);
            enet_host_destroy(m_Host);
        }
    }

    inline void NetworkManager::Network::Broadcast(EMessage type, std::uint8_t channel, const std::uint8_t* data, std::uint32_t size) const
    {
        if(Valid())
        {
            if(void* memory = enet_malloc(sizeof(ENetPacket) + size))
            {
                const auto packet = static_cast<ENetPacket*>(memory);

                switch(type)
                {
                    case EMessage::None:        packet->flags = 0; break;
                    case EMessage::Reliable:    packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE; break;
                    case EMessage::Fragmented:  packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_UNRELIABLE_FRAGMENTED; break;
                    case EMessage::Unsequenced: packet->flags = ENetPacketFlag::ENET_PACKET_FLAG_UNSEQUENCED; break;
                }

                packet->data = static_cast<std::uint8_t*>(memory) + sizeof(ENetPacket);
                packet->referenceCount = 0;
                packet->dataLength = size;
                packet->freeCallback = nullptr;
                packet->userData = nullptr;

                std::memcpy(packet->data, data, size);
                enet_host_broadcast(m_Host, channel, packet);
            }
        }
    }

    [[nodiscard]] inline std::uint16_t NetworkManager::Network::GetID() const noexcept {
        return m_NetworkID;
    }

    inline void NetworkManager::Network::SetUserData(std::unique_ptr<NetworkManager::UserData> data) {
        m_UserData = std::move(data);
    }

    template <typename T>
    requires std::is_base_of_v<NetworkManager::UserData, T>
    [[nodiscard]] T* NetworkManager::Network::GetUserData() noexcept {
        return static_cast<T*>(m_UserData.get());
    }

    template <typename T>
    requires std::is_base_of_v<NetworkManager::UserData, T>
    [[nodiscard]] const T* NetworkManager::Network::GetUserData() const noexcept {
        return static_cast<const T*>(m_UserData.get());
    }

    [[nodiscard]] inline bool NetworkManager::Network::Server() const noexcept {
        HELENA_ASSERT(Valid(), "Network invalid");
        return m_Server;
    }

    [[nodiscard]] inline bool NetworkManager::Network::Valid() const noexcept {
        return m_Initialized && m_Host;
    }

    template <typename Func>
    void NetworkManager::Network::Each(Func&& func) 
    {
        HELENA_ASSERT(Valid(), "Network invalid");
        for(auto peer = m_Host->peers; peer < &m_Host->peers[m_Host->peerCount]; ++peer) {
            func(NetworkManager::Connection{this, peer});
        }
    }

    template <typename Func>
    void NetworkManager::Network::Each(Func&& func) const 
    {
        HELENA_ASSERT(Valid(), "Network invalid");
        for(const auto* peer = m_Host->peers; peer < &m_Host->peers[m_Host->peerCount]; ++peer) {
            func(NetworkManager::Connection{this, peer});
        }
    }

    [[nodiscard]] inline bool NetworkManager::Network::CreateAddress(ENetAddress& address, const std::string_view ip, std::uint16_t port)
    {
        address.port = port;
        if(enet_address_set_ip(&address, ip.data())) {
            HELENA_MSG_ERROR("Create address with ip: {}, port: {} failed!", ip, port);
            return false;
        }

        return true;
    }

    [[nodiscard]] inline ENetHost* NetworkManager::Network::CreateHost(const Config& config, bool server)
    {
        ENetHost* host{};

        if(server) {
            ENetAddress address{};
            if(CreateAddress(address, config.GetIP(), config.GetPort())) {
                host = enet_host_create(&address, config.GetPeers(), config.GetChannels(),
                    config.GetBandwidthIn(), config.GetBandwidthOut(), config.GetBufferSize());
            }
        } else {
            host = enet_host_create(nullptr, config.GetPeers(), 1,
                config.GetBandwidthIn(), config.GetBandwidthOut(), config.GetBufferSize());
        }

        if(host)
        {
            Session* sessions = new Session[host->peerCount]{};
            for(auto currentPeer = host->peers; currentPeer < &host->peers[host->peerCount]; ++currentPeer) {
                currentPeer->data = sessions++;
            }
        } else {
            HELENA_MSG_ERROR("Create host with ip: {}, port: {}, peers: {}, channels: {} failed!",
                config.GetIP(), config.GetPort(), config.GetPeers(), config.GetChannels());
        }
        return host;
    }

    [[nodiscard]] inline std::int64_t NetworkManager::Network::Scramble(std::int64_t value) noexcept {
        std::int64_t out = value ^ 0xDEADBEEFC0DECAFE;
        //out = (out & 0xF0F0F0F0F0F0F0) >> 4 | (out & 0x0F0F0F0F0F0F0F) << 4;
        return out ^ 0xC0DEFACE12345678;
    }

    inline bool NetworkManager::Network::SendHandshake(ENetPeer* peer, std::int64_t key)
    {
        const auto crypt    = Scramble(key);
        const auto packet   = enet_packet_create(reinterpret_cast<const std::uint8_t*>(&crypt),
            sizeof(std::int64_t), ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE);
        if(!packet || enet_peer_send(peer, 0, packet)) {
            enet_peer_reset(peer);
            enet_packet_destroy(packet);
            return false;
        }

        return true;
    }

    inline void NetworkManager::Network::AddHandshake(ENetPeer* peer) {
        m_HandshakeList.emplace_back(this, peer);
    }

    inline void NetworkManager::Network::RemoveHandshake(ENetPeer* peer) 
    {
        const auto id = enet_peer_get_id(peer);
        const auto it = std::find_if(m_HandshakeList.cbegin(), m_HandshakeList.cend(), [id](const auto conn) {
            return conn.GetID() == id;
        });

        if(it != m_HandshakeList.cend()) {
            m_HandshakeList.erase(it);
        }
    }

    inline void NetworkManager::Network::Update(std::uint32_t timeout, std::uint32_t eventsLimit)
    {
        while(true)
        {
            ENetEvent event{};
            if(enet_host_check_events(m_Host, &event) <= 0)
            {
                if(enet_host_service(m_Host, &event, timeout) <= 0) {
                    break;
                }
            }

            switch(event.type)
            {
                case ENET_EVENT_TYPE_NONE: break;
                case ENET_EVENT_TYPE_CONNECT:
                {
                    const auto session = static_cast<Session*>(event.peer->data);
                    session->m_State = EStateConnection::Handshake;

                    if(m_Server)
                    {
                        constexpr auto timeoutHandshake = 2;

                        session->m_Sequence++;
                        session->m_UserData.reset();
                        session->m_HandshakeKey = std::chrono::duration_cast<std::chrono::seconds>(
                            std::chrono::steady_clock::now().time_since_epoch()).count() + timeoutHandshake;

                        if(SendHandshake(event.peer, Scramble(session->m_HandshakeKey))) {
                            AddHandshake(event.peer);
                        }
                    }

                } break;
                case ENET_EVENT_TYPE_DISCONNECT: {
                    Connection conn{this, event.peer};
                    Helena::Engine::SignalEvent<Events::NetworkManager::Event>(conn, event.data, EStateEvent::Disconnect);
                    static_cast<Session*>(event.peer->data)->m_State = EStateConnection::Disconnected;
                } break;
                case ENET_EVENT_TYPE_DISCONNECT_TIMEOUT: {
                    Connection conn{this, event.peer};
                    Helena::Engine::SignalEvent<Events::NetworkManager::Event>(conn, event.data, EStateEvent::Timeout);
                    static_cast<Session*>(event.peer->data)->m_State = EStateConnection::Disconnected;
                } break;
                case ENET_EVENT_TYPE_RECEIVE:
                {
                    Connection conn{this, event.peer};
                    const auto session = static_cast<Session*>(event.peer->data);

                    if(session->m_State == EStateConnection::Handshake)
                    {
                        if(event.packet->dataLength != sizeof(std::int64_t)) 
                        {
                            if(m_Server) {
                                RemoveHandshake(event.peer);
                            }
                            enet_peer_reset(event.peer);
                            enet_packet_destroy(event.packet);
                            break;
                        }

                        const auto decrypt = Scramble(*reinterpret_cast<std::int64_t*>(event.packet->data));
                        if(m_Server)
                        {
                            RemoveHandshake(event.peer);

                            session->m_HandshakeKey = session->m_HandshakeKey ^ (conn.GetID() + 1uLL);
                            if(session->m_HandshakeKey != decrypt || !SendHandshake(event.peer, Scramble(session->m_HandshakeKey))) {
                                enet_peer_reset(event.peer);
                                enet_packet_destroy(event.packet);
                                break;
                            }

                            session->m_State = EStateConnection::Connected;
                            Helena::Engine::SignalEvent<Events::NetworkManager::Event>(conn, event.peer->eventData, EStateEvent::Connect);
                        }
                        else
                        {
                            if(!session->m_HandshakeKey) {
                                session->m_HandshakeKey = decrypt ^ (conn.GetID() + 1uLL);
                                (void)SendHandshake(event.peer, Scramble(session->m_HandshakeKey));
                            } else if(session->m_HandshakeKey == decrypt) {
                                session->m_State = EStateConnection::Connected;
                                Helena::Engine::SignalEvent<Events::NetworkManager::Event>(conn, event.peer->eventData, EStateEvent::Connect);
                            } else {
                                enet_peer_reset(event.peer);
                            }

                        }

                        enet_packet_destroy(event.packet);
                        break;
                    } 

                    EMessage type{};
                    switch(event.packet->flags)
                    {
                        case 0: type = EMessage::None; break;
                        case ENetPacketFlag::ENET_PACKET_FLAG_RELIABLE: type = EMessage::Reliable; break;
                        case ENetPacketFlag::ENET_PACKET_FLAG_UNRELIABLE_FRAGMENTED: type = EMessage::Fragmented; break;
                        case ENetPacketFlag::ENET_PACKET_FLAG_UNSEQUENCED: type = EMessage::Unsequenced; break;
                        default: {
                            HELENA_MSG_WARNING("Recv not supported message flag: {}", event.packet->flags);
                            type = EMessage::Reliable;   // by default unknown flags replaced on Reliable
                        } break;
                    }

                    Helena::Engine::SignalEvent<Events::NetworkManager::Message>(conn, event.packet->data, event.packet->dataLength, type, event.channelID);
                    enet_packet_destroy(event.packet);
                } break;
            }

            if(event.type != ENET_EVENT_TYPE_NONE && eventsLimit) 
            {
                eventsLimit--;
                if(!eventsLimit) {
                    break;
                }
            }
        }

        if(!m_HandshakeList.empty())
        {
            const auto connection = m_HandshakeList.front();
            const auto timenow  = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            const auto session  = static_cast<const Session*>(connection.m_Peer->data);
            if(timenow >= session->m_HandshakeKey) {
                m_HandshakeList.erase(m_HandshakeList.begin());
                enet_peer_reset(connection.m_Peer);
            }
        }
    }

    /* -------------- [NetworkManager] ------------- */
    inline NetworkManager::NetworkManager() : m_Networks{}, m_NetworkSequenceID{}, m_Initialized{} {
        Engine::SubscribeEvent<Events::Engine::Tick>(&NetworkManager::Tick);
    }

    inline NetworkManager::~NetworkManager() {
        Engine::UnsubscribeEvent<Events::Engine::Tick>(&NetworkManager::Tick);
    }

    [[nodiscard]] inline NetworkManager::Network& NetworkManager::CreateNetwork() {
        return m_Networks.emplace_back(m_NetworkSequenceID++);
    }

    inline void NetworkManager::RemoveNetwork(std::uint16_t id) noexcept
    {
        const auto it = std::find_if(m_Networks.cbegin(), m_Networks.cend(), [id](const auto& net) {
            return net.GetID() == id;
        });

        if(it != m_Networks.cend()) {
            m_Networks.erase(it);
        }
    }

    [[nodiscard]] inline NetworkManager::Network* NetworkManager::GetNetwork(std::uint16_t id) noexcept {
        const auto it = std::find_if(m_Networks.begin(), m_Networks.end(), [id](const auto& net) {
            return net.GetID() == id;
        });

        return it == m_Networks.cend() ? nullptr : &(*it);
    }

    [[nodiscard]] inline const NetworkManager::Network* NetworkManager::GetNetwork(std::uint16_t id) const noexcept {
        const auto it = std::find_if(m_Networks.cbegin(), m_Networks.cend(), [id](const auto& net) {
            return net.GetID() == id;
        });

        return it == m_Networks.cend() ? nullptr : &(*it);
    }

    [[nodiscard]] inline std::size_t NetworkManager::Count() const noexcept {
        return m_Networks.size();
    }

    [[nodiscard]] inline auto NetworkManager::begin() noexcept {
        return m_Networks.begin();
    }

    [[nodiscard]] inline auto NetworkManager::begin() const noexcept {
        return m_Networks.begin();
    }

    [[nodiscard]] inline auto NetworkManager::end() noexcept {
        return m_Networks.end();
    }

    [[nodiscard]] inline auto NetworkManager::end() const noexcept {
        return m_Networks.end();
    }

    inline void NetworkManager::Tick(const Helena::Events::Engine::Tick ev)
    {
        for(auto& net : m_Networks)
        {
            if(!net.Valid()) {
                continue;
            }

            net.Update();
        }
    }
}

#endif // HELENA_SYSTEMS_NETWORKMANAGER_IPP