#ifndef DRAMPOWER_UTIL_EXTENSION_MANAGER
#define DRAMPOWER_UTIL_EXTENSION_MANAGER

#include <cstddef>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <vector>
#include <bit>

#include <DRAMPower/util/extension_base.h>

#include <DRAMUtils/util/types.h>

namespace DRAMPower::util::extension_manager {

// Helper function to count trailing zeros in an integral type
template <typename T>
constexpr int countZeros(T value) {
    static_assert(std::is_integral_v<T>, "countZeros only works with integral types");
    if (value == 0) return std::numeric_limits<T>::digits;

    int count = 0;
    constexpr T one = 1;
    while ((value & one) == 0) {
        count++;
        value >>= 1;
    }
    return count;
}

// dynamic extension manager
// BaseExtension class is used for type erasure
template <typename BaseExtension>
class ExtensionManager {
private:
// Type definitions
    using Extension_storage_t = std::unordered_map<std::type_index, std::shared_ptr<BaseExtension>>;

// Member variables
    Extension_storage_t m_extensions;

protected:
// Protected member functions
    template <typename T, typename... Args>
    decltype(auto) registerExtension_impl(Args&&... args) {
        static_assert(std::is_base_of_v<BaseExtension, T>,
            "T must derive from BaseExtension");

        auto ext = std::make_shared<T>(std::forward<Args>(args)...);
        auto typeIndex = std::type_index(typeid(T));
        m_extensions[typeIndex] = ext;
        return ext;
    }

public:
// Constructor
    ExtensionManager() = default;

// Public member functions
    // Register an extension, forwarding any additional arguments.
    template <typename T, typename... Args>
    void registerExtension(Args&&... args) {
        registerExtension_impl<T>(std::forward<Args>(args)...);
    }

    // Retrieve an extension (returns an empty weak_ptr if not found)
    template <typename T>
    std::weak_ptr<T> getExtension() {
        static_assert(std::is_base_of_v<BaseExtension, T>,
            "T must derive from BaseExtension");
        if (m_extensions.empty()) {
            return std::weak_ptr<T>{};
        }
        auto it = m_extensions.find(std::type_index(typeid(T)));
        if (it != m_extensions.end()) {
            return std::static_pointer_cast<T>(it->second);
        }
        return std::weak_ptr<T>{};
    }

    // Check if an extension is registered
    template <typename T>
    bool hasExtension() const {
        static_assert(std::is_base_of_v<BaseExtension, T>,
            "T must derive from BaseExtension");
        if (m_extensions.empty()) {
            return false;
        }
        return m_extensions.find(std::type_index(typeid(T))) != m_extensions.end();
    }

    // Visitor pattern for extensions
    template <typename T, typename Func>
    void withExtension(Func&& func) {
        static_assert(std::is_base_of_v<BaseExtension, T>,
            "T must derive from BaseExtension");
        if (!m_extensions.empty()) {
            auto it = m_extensions.find(std::type_index(typeid(T)));
            if (it != m_extensions.end()) {
                std::forward<Func>(func)(*std::static_pointer_cast<T>(it->second));
            }
        }
    }
};

template <typename BaseExtension, typename Hooks, typename enable = void>
class ExtensionManagerWithHooks : ExtensionManager<BaseExtension> {
    static_assert(DRAMUtils::util::always_false<BaseExtension>::value,
        "BaseExtension must inherit from ExtensionWithHooks<Hooks>"
    );
};
template <typename BaseExtension, typename Hooks>
class ExtensionManagerWithHooks<
    BaseExtension,
    Hooks,
    std::enable_if_t<std::is_base_of_v<ExtensionWithHooks<Hooks>, BaseExtension>>
>
: public ExtensionManager<BaseExtension> {
private:
// Type definitions
    using Hooks_t = Hooks;
    using HookValue_t = std::underlying_type_t<Hooks_t>;
    using HookCache_t = std::vector<std::vector<std::shared_ptr<BaseExtension>>>;

// Member variables
    constexpr static std::size_t m_numHooks = std::numeric_limits<HookValue_t>::digits;
    HookCache_t m_hookCache;

// Private member functions
    static constexpr std::size_t getHookPosition(Hooks_t hook) {
        return countZeros(static_cast<HookValue_t>(hook));
    }

    void initializeHooks() {
        if (m_hookCache.empty()) {
            m_hookCache.resize(m_numHooks);
        }
    }

    void updateHookCache(std::shared_ptr<BaseExtension>&& extension) {
        // dynamic cast used for additional type safety
        auto hookExt = std::dynamic_pointer_cast<ExtensionWithHooks<Hooks>>(extension);
        if (!hookExt) return; // dynamic_cast checked in std::enable_if_t

        const HookValue_t supportedHooks = static_cast<HookValue_t>(hookExt->getSupportedHooks());
        for (std::size_t i = 0; i < m_hookCache.size(); ++i) {
            if ((supportedHooks & (1 << i)) != 0) {
                m_hookCache[i].push_back(extension);
            }
        }
    }

public:
// Constructor
    ExtensionManagerWithHooks() {
        initializeHooks();
    }

// Public member functions
    // Register an extension, forwarding any additional arguments.
    template <typename T, typename... Args>
    void registerExtension(Args&&... args) {
        updateHookCache(this->template registerExtension_impl<T>(std::forward<Args>(args)...));
    }

    // Call the hook function for all registered extensions supporting the hook
    template<typename Func>
    void callHook(Hooks_t hook, Func&& func) {
        const std::size_t bitPosition = getHookPosition(hook);
        if (bitPosition < 0 || bitPosition >= m_hookCache.size()) {
            return; // Invalid hook
        }

        for (const auto& extension_ptr : m_hookCache[bitPosition]) {
            std::forward<Func>(func)(*extension_ptr);
        }
    }

};

} // namespace DRAMPower::util::extension_manager

#endif /* DRAMPOWER_UTIL_EXTENSION_MANAGER */
