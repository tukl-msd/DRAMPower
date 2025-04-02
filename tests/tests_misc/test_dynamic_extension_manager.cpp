#include <gtest/gtest.h>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

#include <DRAMUtils/util/types.h>

#include <DRAMPower/util/extension_manager.h>

using namespace DRAMPower;
using namespace DRAMPower::util;


// dynamic extension example without hooks
class DynamicExtensionBase {
protected:
    int m_base_variable = 47;
};
// dynamic extension example without hooks
// This class can also inherit from extension_manager::Extension
class DynamicExtensionExample : public DynamicExtensionBase {
public:
    explicit DynamicExtensionExample(bool initstate, int& captured_int)
    : m_state(initstate)
    , m_captured_int(captured_int)
    {
        captured_int = m_state ? 1 : 0;
    }

    bool getState() const {
        return m_state;
    }
    void setState(bool state) {
        m_state = state;
        // Also update a captured variable
        m_captured_int = state ? 1 : 0;
    }
    int getBaseVariable() const {
        return m_base_variable;
    }
private:
    bool m_state = false;
    int& m_captured_int;
};



// Hook example
enum class DynamicExtensionHookExample : uint64_t {
    Hook_0 = 1 << 0,
    Hook_1 = 1 << 1,
    Hook_2 = 1 << 2,
    Hook_3 = 1 << 3,
    Hook_4 = 1 << 4,
    // ...
    ALL = Hook_0 | Hook_1 | Hook_2 | Hook_3 | Hook_4
};
constexpr DynamicExtensionHookExample operator|(DynamicExtensionHookExample lhs, DynamicExtensionHookExample rhs) {
    return static_cast<DynamicExtensionHookExample>(static_cast<std::underlying_type<DynamicExtensionHookExample>::type>(lhs) |
        static_cast<std::underlying_type<DynamicExtensionHookExample>::type>(rhs));
}
constexpr DynamicExtensionHookExample operator&(DynamicExtensionHookExample lhs, DynamicExtensionHookExample rhs) {
    return static_cast<DynamicExtensionHookExample>(static_cast<std::underlying_type<DynamicExtensionHookExample>::type>(lhs) &
        static_cast<std::underlying_type<DynamicExtensionHookExample>::type>(rhs));
}
constexpr bool operator!=(DynamicExtensionHookExample lhs, size_t rhs) {
    return static_cast<std::underlying_type<DynamicExtensionHookExample>::type>(lhs) != rhs;
}

// dynamic extension base with hooks
class DynamicExtensionWithHooksBase : public extension_manager::ExtensionWithHooks<DynamicExtensionHookExample> {
public:
    using extension_manager::ExtensionWithHooks<DynamicExtensionHookExample>::ExtensionWithHooks;

    int getBaseVariable() const {
        return m_base_variable;
    }

    // Hook functions with default implementation
    virtual void Hook_0(int&) {}
    virtual void Hook_1(int&) const {}
    virtual void Hook_2(int&) {}
    virtual void Hook_3(int&) const {}
    virtual void Hook_4(int&) {}

private:
    int m_base_variable = 47;
};

// dynamic extension example with hooks
class DynamicExtensionWithHooksExample : public DynamicExtensionWithHooksBase {
    public:
        explicit DynamicExtensionWithHooksExample(bool initstate, int& captured_int)
        : m_state(initstate)
        , m_captured_int(captured_int)
        {
            captured_int = m_state ? 1 : 0;
        }
    
        bool getState() const {
            return m_state;
        }
        void setState(bool state) {
            m_state = state;
            m_captured_int = state ? 1 : 0;
        }

        DynamicExtensionHookExample getSupportedHooks() const override {
            return DynamicExtensionHookExample::Hook_0
                | DynamicExtensionHookExample::Hook_3
                | DynamicExtensionHookExample::Hook_4;
        }
        void Hook_0(int& i) override {
            i = 0;
            m_captured_int = 10;
        }
        void Hook_3(int& i) const override{
            i = 3;
            m_captured_int = 30;
        }
        void Hook_4(int& i) override {
            i = 4;
            m_captured_int = 40;
        }
    private:
        bool m_state = false;
        int& m_captured_int;
};

class DynamicExtensionExampleType {
public:
// Type definitions
    using Extension_manager_t = util::extension_manager::ExtensionManager<
        DynamicExtensionBase
    >;
// Member variables
    Extension_manager_t m_extensionManager;
// Retrieve extension manager
    Extension_manager_t& getExtensionManager() {
        return m_extensionManager;
    }
    const Extension_manager_t& getExtensionManager() const {
        return m_extensionManager;
    }
};

class DynamicExtensionHookExampleType {
public:
// Type definitions
    using Extension_manager_Hooks_t = util::extension_manager::ExtensionManagerWithHooks<
        DynamicExtensionWithHooksBase,
        DynamicExtensionHookExample
    >;
// Member variables
    Extension_manager_Hooks_t m_extensionManagerHooks;
// Retrieve extension manager
    Extension_manager_Hooks_t& getExtensionManager() {
        return m_extensionManagerHooks;
    }
    const Extension_manager_Hooks_t& getExtensionManager() const {
        return m_extensionManagerHooks;
    }

// Member functions
    void testhook0(int &i) {
        m_extensionManagerHooks.callHook(DynamicExtensionHookExample::Hook_0, [this, &i](auto& ext) {
            ext.Hook_0(i);
        });
    }
    void testhook1(int &i) {
        m_extensionManagerHooks.callHook(DynamicExtensionHookExample::Hook_1, [this, &i](const auto& ext) {
            ext.Hook_1(i);
        });
    }
    void testhook2(int &i) {
        m_extensionManagerHooks.callHook(DynamicExtensionHookExample::Hook_2, [this, &i](auto& ext) {
            ext.Hook_2(i);
        });
    }
    void testhook3(int &i) {
        m_extensionManagerHooks.callHook(DynamicExtensionHookExample::Hook_3, [this, &i](const auto& ext) {
            ext.Hook_3(i);
        });
    }
    void testhook4(int &i) {
        m_extensionManagerHooks.callHook(DynamicExtensionHookExample::Hook_4, [this, &i](auto& ext) {
            ext.Hook_4(i);
        });
    }
};

class MiscTestExtension : public ::testing::Test {
protected:
    // Test variables
    using testclass_t = DynamicExtensionExampleType;
    using testclass_hook_t = DynamicExtensionHookExampleType;

    std::unique_ptr<testclass_t> dut;
    std::unique_ptr<testclass_hook_t> dut_hook;

    virtual void SetUp()
    {
        dut = std::make_unique<testclass_t>();
        dut_hook = std::make_unique<testclass_hook_t>();
    }

    virtual void TearDown()
    {
    }
};

#define ASSERT_EQ_BITSET(N, lhs, rhs) ASSERT_EQ(lhs, util::dynamic_bitset<N>(N, rhs))

TEST_F(MiscTestExtension, DynamicExtension0)
{
    // Get reference to extension manager
    auto& ext_manager = dut->getExtensionManager();
    
    // no extension registered
    ASSERT_FALSE(ext_manager.hasExtension<DynamicExtensionExample>());
    auto ext = ext_manager.getExtension<DynamicExtensionExample>();
    ASSERT_TRUE(ext.expired());

    // Register extension with captured reference
    int captured_int = 0;
    ext_manager.registerExtension<DynamicExtensionExample>(true, captured_int);
    
    // Test registration
    ASSERT_EQ(ext_manager.hasExtension<DynamicExtensionExample>(), true);
    ext = ext_manager.getExtension<DynamicExtensionExample>();
    ASSERT_FALSE(ext.expired());
    ASSERT_EQ(ext.lock()->getState(), true);
    ASSERT_EQ(ext.lock()->getBaseVariable(), 47);
    ASSERT_EQ(captured_int, 1);
    ext.lock()->setState(false);
    ASSERT_EQ(ext.lock()->getState(), false);
    ASSERT_EQ(captured_int, 0);
    ext.lock()->setState(true);
    ASSERT_EQ(ext.lock()->getState(), true);
    ASSERT_EQ(captured_int, 1);
    ASSERT_EQ(dut->getExtensionManager().getExtension<DynamicExtensionExample>().lock()->getState(), true);
}

TEST_F(MiscTestExtension, DynamicExtensionHooks0)
{
    // Get reference to extension manager
    auto& ext_manager = dut_hook->getExtensionManager();
    
    // no extension registered
    ASSERT_FALSE(ext_manager.hasExtension<DynamicExtensionWithHooksExample>());
    auto ext = ext_manager.getExtension<DynamicExtensionWithHooksExample>();
    ASSERT_TRUE(ext.expired());

    // Register extension with captured reference
    int captured_int = 0;
    ext_manager.registerExtension<DynamicExtensionWithHooksExample>(true, captured_int);
    
    // Test registration
    ASSERT_EQ(ext_manager.hasExtension<DynamicExtensionWithHooksExample>(), true);
    ext = ext_manager.getExtension<DynamicExtensionWithHooksExample>();
    ASSERT_FALSE(ext.expired());
    ASSERT_EQ(ext.lock()->getState(), true);
    ASSERT_EQ(ext.lock()->getBaseVariable(), 47);
    ASSERT_EQ(captured_int, 1);
    ext.lock()->setState(false);
    ASSERT_EQ(ext.lock()->getState(), false);
    ASSERT_EQ(captured_int, 0);
    ext.lock()->setState(true);
    ASSERT_EQ(ext.lock()->getState(), true);
    ASSERT_EQ(captured_int, 1);
    ASSERT_EQ(dut_hook->getExtensionManager().getExtension<DynamicExtensionWithHooksExample>().lock()->getState(), true);

    // Test hooks
    ASSERT_EQ(captured_int, 1);
    int i = -1;
    dut_hook->testhook0(i);
    ASSERT_EQ(i, 0);
    ASSERT_EQ(captured_int, 10);
    i = -1;
    dut_hook->testhook1(i);
    ASSERT_EQ(i, -1);
    ASSERT_EQ(captured_int, 10);
    i = -1;
    dut_hook->testhook2(i);
    ASSERT_EQ(i, -1);
    ASSERT_EQ(captured_int, 10);
    i = -1;
    dut_hook->testhook3(i);
    ASSERT_EQ(i, 3);
    ASSERT_EQ(captured_int, 30);
    i = -1;
    dut_hook->testhook4(i);
    ASSERT_EQ(i, 4);
    ASSERT_EQ(captured_int, 40);
}
