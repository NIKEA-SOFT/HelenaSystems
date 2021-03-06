#ifndef HELENA_SYSTEMS_ECSMANAGER_HPP
#define HELENA_SYSTEMS_ECSMANAGER_HPP

#include <Helena/Debug/Assert.hpp>

#ifdef ENTT_ASSERT
    #undef ENTT_ASSERT
#endif

#define ENTT_ASSERT     HELENA_ASSERT
#define ENTT_ID_TYPE    std::uint32_t

#include <entt/entt.hpp>

namespace Helena::Systems
{
    class ECSManager final
    {
        // EnTT type_seq need overload for using our shared memory indexer
        template <typename, typename>
        friend struct ENTT_API entt::type_seq;

        // Support EnTT type_seq across boundary
        std::unordered_map<std::uint64_t, entt::id_type> m_TypeSequence;
        entt::id_type GetSequenceIndex(std::uint64_t index) {
            const auto [it, result] = m_TypeSequence.try_emplace(index, m_TypeSequence.size());
            return static_cast<entt::id_type>(it->second);
        }

    public:
        static constexpr auto Null = entt::null;

        using Entity = entt::entity;

        template <entt::id_type Value>
        using Tag = entt::tag<Value>;

        template <typename... Type>
        using ExcludeType = entt::exclude_t<Type...>;

        template <typename... Type>
        using GetType = entt::get_t<Type...>;

        template<typename... Type>
        static constexpr ExcludeType<Type...> Exclude{};

        template<typename... Type>
        static constexpr GetType<Type...> Get{};

    public:
        ECSManager() = default;
        ~ECSManager();
        ECSManager(const ECSManager&) = delete;
        ECSManager(ECSManager&&) noexcept = delete;
        ECSManager& operator=(const ECSManager&) = delete;
        ECSManager& operator=(ECSManager&&) noexcept = delete;

        [[nodiscard]] auto CreateEntity() -> Entity;

        template <typename Type, typename = std::enable_if_t<std::is_integral_v<Type>>>
        [[nodiscard]] auto CreateEntity(const Type id) -> Entity;

        template <typename It>
        auto CreateEntity(It first, It last) -> void;

        [[nodiscard]] auto HasEntity(const Entity id) const -> bool;

        [[nodiscard]] auto SizeEntity() const noexcept -> std::size_t;

        [[nodiscard]] auto AliveEntity() const -> std::size_t;

        auto ReserveEntity(const std::size_t size) -> void;

        auto RemoveEntity(const Entity id) -> void;

        template <typename It>
        auto RemoveEntity(It first, It last) -> void;

        template <typename Type, typename = std::enable_if_t<std::is_integral_v<Type>>>
        [[nodiscard]] static auto Cast(const Type id) noexcept;

        [[nodiscard]] static auto Cast(const Entity id) noexcept;

        template <typename Func>
        auto Each(Func&& callback) const -> void;

        template <typename Func>
        auto EachOrphans(Func&& callback) const -> void;

        template <typename Component, typename... Args>
        auto AddComponent(const Entity id, Args&&... args) -> decltype(auto);

        template <typename... Components>
        [[nodiscard]] auto GetComponent(const Entity id) -> decltype(auto);

        template <typename... Components>
        [[nodiscard]] auto GetComponent(const Entity id) const -> decltype(auto);

        template <typename... Components>
        [[nodiscard]] auto GetComponentPtr(const Entity id);

        template <typename... Components>
        [[nodiscard]] auto GetComponentPtr(const Entity id) const;

        template <typename... Components>
        [[nodiscard]] auto HasComponent(const Entity id) const -> bool;

        [[nodiscard]] auto HasComponent(const Entity id) const -> bool;

        template <typename... Components>
        [[nodiscard]] auto AnyComponent(const Entity id) const -> bool;

        template <typename Func>
        auto VisitComponent(const Entity id, Func&& callback) const -> void;

        template <typename Func>
        auto VisitComponent(Func&& callback) const -> void;

        template <typename... Components, typename... ExcludeFilter>
        [[nodiscard]] auto ViewComponent(ExcludeType<ExcludeFilter...> = {}) -> decltype(auto);

        template <typename... Components, typename... ExcludeFilter>
        [[nodiscard]] auto ViewComponent(ExcludeType<ExcludeFilter...> = {}) const -> decltype(auto);

        template <typename... Owned, typename... ExcludeFilter>
        [[nodiscard]] auto GroupComponent(ExcludeType<ExcludeFilter...> = {}) -> decltype(auto);

        template <typename... Owned, typename... ExcludeFilter>
        [[nodiscard]] auto GroupComponent(ExcludeType<ExcludeFilter...> = {}) const -> decltype(auto);

        template <typename... Owned, typename... GetFilter, typename... ExcludeFilter>
        [[nodiscard]] auto GroupComponent(GetType<GetFilter...>, ExcludeType<ExcludeFilter...> = {}) -> decltype(auto);

        template <typename... Owned, typename... GetFilter, typename... ExcludeFilter>
        [[nodiscard]] auto GroupComponent(GetType<GetFilter...>, ExcludeType<ExcludeFilter...> = {}) const -> decltype(auto);

        template <typename... Components>
        auto RemoveComponent(const Entity id) -> void;

        template <typename... Components, typename It>
        auto RemoveComponent(It first, It last) -> void;

        template <typename... Components>
        auto ClearComponent() -> void;

        auto Clear() -> void;

        template <typename Component>
        [[nodiscard]] auto SizeComponent() const -> std::size_t;

        template <typename... Components>
        auto ReserveComponent(const std::size_t size) -> void;

    private:

        entt::registry m_Registry;
    };
}

namespace Helena::Events::ECSManager
{
    struct CreateEntity {
        Systems::ECSManager::Entity Entity {Helena::Systems::ECSManager::Null};
    };

    struct RemoveEntity {
        Systems::ECSManager::Entity Entity {Helena::Systems::ECSManager::Null};
    };

    template <typename Component>
    struct AddComponent {
        Systems::ECSManager::Entity Entity {Helena::Systems::ECSManager::Null};
    };

    template <typename Component>
    struct RemoveComponent {
        Systems::ECSManager::Entity Entity {Helena::Systems::ECSManager::Null};
    };
}

#include "ECSManager.ipp"

#endif // HELENA_SYSTEMS_ECSMANAGER_HPP
