#ifndef DRAMPOWER_UTIL_EXTENSION_BASE
#define DRAMPOWER_UTIL_EXTENSION_BASE

namespace DRAMPower::util::extension_manager {

// dynamic extension base
class Extension {
public:
    Extension() = default;
    virtual ~Extension() = default;
};

// dynamic extension with hooks
template <typename Hooks>
class ExtensionWithHooks : public Extension {
public:
    using Extension::Extension;

    // Return supported hooks
    virtual Hooks getSupportedHooks() const = 0;
};

} // namespace DRAMPower::util::extension_manager

#endif /* DRAMPOWER_UTIL_EXTENSION_BASE */
