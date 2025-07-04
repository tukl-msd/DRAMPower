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

// Helpers
namespace detail {
    template <typename Func, typename Ext>
    constexpr auto tryCallFunctor(Func&& func, Ext& ext) -> decltype(std::forward<Func>(func)(ext)) {
        if constexpr (std::is_invocable_v<Func, Ext&>) {
            return std::forward<Func>(func)(ext);
        } else {
            // If the function is not invocable, do nothing
            return;
        }
    }

    template <typename Ext, typename FuncTuple, std::size_t... Is>
    constexpr void callHookIfSupportedImpl(Ext& extension, FuncTuple&& funcTuple, std::index_sequence<Is...>) {
        (detail::tryCallFunctor(std::get<Is>(std::forward<FuncTuple>(funcTuple)), extension), ...);
    }
} // namespace detail

/** static extensionmanager
 * Template arguments:
 * - Parent: Parent type of the type which holds the StaticExtensionManager
 * - Seq: DRAMUtils::util::type_sequence of the StaticExtensions
 * - Hook: enum for retrieving the extensions supporting the given hook in callHook
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
    template <Hook_t hook, typename FuncTuple, std::size_t... Is>
    constexpr void callHookImpl(FuncTuple&& funcTuple, std::index_sequence<Is...>) {
        // use index_sequence to loop over callHookIfSupported
        // example: callHookIfSupported<0>(...), callHookIfSupported<1>(...), ...
        if constexpr (sizeof...(Is) > 0) {
            (callHookIfSupported<hook, Is>(std::forward<FuncTuple>(funcTuple)), ...);
        }/* else {
            // Prevent "unused parameter" warning
            (void) hook;
            (void) func;
        }*/
    }

    template <Hook_t hook, std::size_t I, typename FuncTuple>
    constexpr void callHookIfSupported(FuncTuple&& funcTuple) {
        // Use index from callHookImpl to get the tuple elementtype to query for the supported hooks
        using ExtensionType = std::tuple_element_t<I, Extension_tuple_t>;
        constexpr Hook_t supportedHooks = ExtensionType::getSupportedHooks();
        if constexpr ((static_cast<Hook_value_t>(supportedHooks) & static_cast<Hook_value_t>(hook)) != 0) {
            // retrieve the extension from the tuple and call the functor if the extension is supported
            auto& extension = std::get<I>(m_extensions);
            detail::callHookIfSupportedImpl(extension, std::forward<FuncTuple>(funcTuple),
                std::make_index_sequence<std::tuple_size_v<std::decay_t<FuncTuple>>>{});
        }
    }

    

public:
// Constructor
    StaticExtensionManager()
    : m_extensions(StaticExtensions{}...)
    {}
// Public member functions
    template <Hook_t hook, typename... Func>
    constexpr void callHook(Func&&... funcs) {
        using FuncTuple = std::tuple<Func...>;
        // Explicit functor copy to ensure memory safety
        FuncTuple funcTuple{std::forward<Func>(funcs)...};
        if constexpr (sizeof...(Func) > 0) {
            // Call Hook implementation with integer sequence of length sizeof...(StaticExtensions)
            callHookImpl<hook>(std::move(funcTuple), Extension_index_sequence_t{});
        }
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
