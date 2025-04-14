#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

#include <DRAMUtils/util/types.h>

#include <DRAMPower/util/extension_manager_static.h>

using namespace DRAMPower;

// static extension example
class StaticExtensionExample {};

// Hook example
enum class StaticExtensionHookExample : uint64_t {
    Hook_0 = 1 << 0,
    Hook_1 = 1 << 1,
    Hook_2 = 1 << 2,
    Hook_3 = 1 << 3,
    Hook_4 = 1 << 4,
    // ...
    ALL = Hook_0 | Hook_1 | Hook_2 | Hook_3 | Hook_4
};
constexpr StaticExtensionHookExample operator|(StaticExtensionHookExample lhs, StaticExtensionHookExample rhs) {
    return static_cast<StaticExtensionHookExample>(static_cast<std::underlying_type<StaticExtensionHookExample>::type>(lhs) |
        static_cast<std::underlying_type<StaticExtensionHookExample>::type>(rhs));
}
constexpr StaticExtensionHookExample operator&(StaticExtensionHookExample lhs, StaticExtensionHookExample rhs) {
    return static_cast<StaticExtensionHookExample>(static_cast<std::underlying_type<StaticExtensionHookExample>::type>(lhs) &
        static_cast<std::underlying_type<StaticExtensionHookExample>::type>(rhs));
}
constexpr bool operator!=(StaticExtensionHookExample lhs, size_t rhs) {
    return static_cast<std::underlying_type<StaticExtensionHookExample>::type>(lhs) != rhs;
}

// static extension with hook example
class StaticExtensionWithHookExample {
public:

    static constexpr StaticExtensionHookExample getSupportedHooks() {
        return StaticExtensionHookExample::Hook_0
            | StaticExtensionHookExample::Hook_3
            | StaticExtensionHookExample::Hook_4;
    }

// Hooks
    void Hook_0(int& i) {i = 0;}
    void Hook_3(int& i) {i = 3;}
    void Hook_4(int& i) {i = 4;}

};


template <typename... Extensions>
class StaticExtensionExampleType {
public:
// Type definitions
    using Extension_manager_t = util::extension_manager_static::StaticExtensionManager<
        DRAMUtils::util::type_sequence<Extensions...>,
        StaticExtensionHookExample
    >;
// Member variables
    Extension_manager_t extensionManager;
// Hook helper
    template <StaticExtensionHookExample hook, typename... Args>
    void invokeHook(Args&&... args) {
        extensionManager.template callHook<hook>(std::forward<Args>(args)...);
    }

// Member functions
    void testhook0(int &i) {
        invokeHook<StaticExtensionHookExample::Hook_0>([this, &i](auto& ext) {
            ext.Hook_0(i);
        });
    }
    void testhook1(int &i) {
        invokeHook<StaticExtensionHookExample::Hook_1>([this, &i](auto& ext) {
            ext.Hook_1(i);
        });
    }
    void testhook2(int &i) {
        invokeHook<StaticExtensionHookExample::Hook_2>([this, &i](auto& ext) {
            ext.Hook_2(i);
        });
    }
    void testhook3(int &i) {
        invokeHook<StaticExtensionHookExample::Hook_3>([this, &i](auto& ext) {
            ext.Hook_3(i);
        });
    }
    void testhook4(int &i) {
        invokeHook<StaticExtensionHookExample::Hook_4>([this, &i](auto& ext) {
            ext.Hook_4(i);
        });
    }
};

class MiscTestStaticExtension : public ::testing::Test {
protected:
    // Test variables
    using testclass_t = StaticExtensionExampleType<StaticExtensionWithHookExample>;

    std::unique_ptr<testclass_t> dut;

    virtual void SetUp()
    {
        dut = std::make_unique<testclass_t>();
    }

    virtual void TearDown()
    {
    }
};

#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_EQ(lhs, util::dynamic_bitset<N>(N, rhs))

TEST_F(MiscTestStaticExtension, StaticExtensionManager0)
{
    // Test assertions
    int i = -1;
    dut->testhook0(i);
    ASSERT_EQ(i, 0);
    i = -1;
    dut->testhook1(i);
    ASSERT_EQ(i, -1);
    i = -1;
    dut->testhook2(i);
    ASSERT_EQ(i, -1);
    i = -1;
    dut->testhook3(i);
    ASSERT_EQ(i, 3);
    i = -1;
    dut->testhook4(i);
    ASSERT_EQ(i, 4);
    i = -1;
}
