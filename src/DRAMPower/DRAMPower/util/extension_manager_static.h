#ifndef DRAMPOWER_UTIL_EXTENSION_MANAGER_STATIC
#define DRAMPOWER_UTIL_EXTENSION_MANAGER_STATIC

#include <cstddef>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <functional>
#include <vector>
#include <stdint.h>

#include <DRAMUtils/util/types.h>

namespace DRAMPower::util::extension_manager_static {

/** static extensionmanager
 * Template arguments:
 * - Parent: Parent type of the type which holds the StaticExtensionManager
 * - Seq: DRAMUtils::util::type_sequence of the StaticExtensions
 * - Hook: enum for retrieving the extensions supportting the given hook in callHook
 */ 
template <typename Seq, typename Hook>
class StaticExtensionManager{
    static_assert(DRAMUtils::util::always_false<Seq>::value, "Cannot construct StaticExtensionManager.");
};
template <typename Hook, typename... StaticExtensions>
class StaticExtensionManager<DRAMUtils::util::type_sequence<StaticExtensions...>, Hook> {
private:
// Type definitions
    using Extension_type_sequence_t = DRAMUtils::util::type_sequence<StaticExtensions...>;
    using Extension_tuple_t = std::tuple<StaticExtensions...>;
    using Extension_index_sequence_t = std::index_sequence_for<StaticExtensions...>;
    using Hook_t = Hook;
    using Hook_value_t = std::underlying_type_t<Hook>;
// Constexpr helpers
    constexpr static std::size_t m_numExtensions = sizeof...(StaticExtensions);
// Members
    Extension_tuple_t m_extensions;

private:
// Private member functions
    template <Hook_t hook, typename Func, std::size_t... Is>
    constexpr void callHookImpl(Func&& func, std::index_sequence<Is...>) {
        // use index_sequence to loop over callHookIfSupported
        // example: callHookIfSupported<0>(...), callHookIfSupported<1>(...), ...
        if constexpr (sizeof...(Is) > 0) {
            (callHookIfSupported<hook, Is>(std::forward<Func>(func)), ...);
        }/* else {
            // Prevent "unused parameter" warning
            (void) hook;
            (void) func;
        }*/
    }

    template <Hook_t hook, std::size_t I, typename Func>
    constexpr void callHookIfSupported(Func&& func) {
        // Use index from callHookImpl to get the tuple elementtype to query for the supported hooks
        using ExtensionType = std::tuple_element_t<I, Extension_tuple_t>;
        constexpr Hook_t supportedHooks = ExtensionType::getSupportedHooks();
        if constexpr ((static_cast<Hook_value_t>(supportedHooks) & static_cast<Hook_value_t>(hook)) != 0) {
            // retrieve the extension from the tuple and call the function
            std::forward<Func>(func)(std::get<I>(m_extensions));
        }
    }

public:
// Constructor
    StaticExtensionManager()
    : m_extensions(StaticExtensions{}...)
    {}
// Public member functions
    template <Hook_t hook, typename Func>
    constexpr void callHook(Func&& func) {
        // Call Hook implementation with integer sequence of length sizeof...(StaticExtensions)
        callHookImpl<hook>(std::forward<Func>(func), Extension_index_sequence_t{});
    }

    // has extension
    template <typename T>
    constexpr static bool hasExtension() {
        return DRAMUtils::util::is_one_of<T, Extension_type_sequence_t>::value;
    }

    // get extension reference
    template <typename T>
    constexpr T& getExtension() {
        static_assert(hasExtension<T>(), "Extension not found");
        return std::get<T>(m_extensions);
    }
    // get extension reference
    template <typename T>
    constexpr const T& getExtension() const {
        static_assert(hasExtension<T>(), "Extension not found");
        return std::get<T>(m_extensions);
    }

    // visitor
    template <typename Extension, typename Func>
    constexpr decltype(auto) withExtension(Func&& func) {
        static_assert(hasExtension<Extension>(), "Extension not found");
        return std::forward<Func>(func)(getExtension<Extension>());
    }
};

} // namespace DRAMPower::util::extension_manager_static

#endif /* DRAMPOWER_UTIL_EXTENSION_MANAGER_STATIC */
