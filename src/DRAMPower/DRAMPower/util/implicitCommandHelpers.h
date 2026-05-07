#ifndef DRAMPOWER_UTIL_IMPLICITCOMMANDHELPERS_H
#define DRAMPOWER_UTIL_IMPLICITCOMMANDHELPERS_H

#include "DRAMPower/Types.h"
#include "DRAMPower/util/adl_serializer.h"

#include <istream>
#include <ostream>

#define DEFINE_IMPLICIT_COMMAND(ClassName, BridgeFunc, ContainerType)            \
class ClassName : public util::Serialize, public util::Deserialize {             \
public:                                                                          \
    template<typename... Args>                                                   \
    explicit ClassName(Args&&... args)                                           \
        : m_data{std::forward<Args>(args)...} {}                                 \
                                                                                 \
    void operator()(DDR4Core& self) {                                            \
        BridgeFunc(self, m_data);                                                \
    }                                                                            \
                                                                                 \
    void serialize(std::ostream& stream) const override {                        \
        type_utils::ADLSerializer<ContainerType>::serialize(stream, m_data);     \
    }                                                                            \
                                                                                 \
    void deserialize(std::istream& stream) override {                            \
        type_utils::ADLDeserializer<ContainerType>::deserialize(stream, m_data); \
    }                                                                            \
private:                                                                         \
    ContainerType m_data;                                                        \
};                                                                               \
                                                                                 \
inline void serialize(std::ostream& stream, const ClassName& val) {              \
    val.serialize(stream);                                                       \
}                                                                                \
                                                                                 \
inline void deserialize(std::istream& stream, ClassName& val) {                  \
    val.deserialize(stream);                                                     \
}


// Data Containers
namespace DRAMPower {

// Data Container rank, bank, time
struct Container_rbt {
    std::size_t r;
    std::size_t b;
    timestamp_t t;
};
inline void serialize(std::ostream& stream, const Container_rbt& container) {
    stream.write(reinterpret_cast<const char*>(&container.r), sizeof(container.r));
    stream.write(reinterpret_cast<const char*>(&container.b), sizeof(container.b));
    stream.write(reinterpret_cast<const char*>(&container.t), sizeof(container.t));

}
inline void deserialize(std::istream& stream, Container_rbt& container) {
    stream.read(reinterpret_cast<char*>(&container.r), sizeof(container.r));
    stream.read(reinterpret_cast<char*>(&container.b), sizeof(container.b));
    stream.read(reinterpret_cast<char*>(&container.t), sizeof(container.t));

}

// Data Container rank, time
struct Container_rt {
    std::size_t r;
    timestamp_t t;
};
inline void serialize(std::ostream& stream, const Container_rt& container) {
    stream.write(reinterpret_cast<const char*>(&container.r), sizeof(container.r));
    stream.write(reinterpret_cast<const char*>(&container.t), sizeof(container.t));

}
inline void deserialize(std::istream& stream, Container_rt& container) {
    stream.read(reinterpret_cast<char*>(&container.r), sizeof(container.r));
    stream.read(reinterpret_cast<char*>(&container.t), sizeof(container.t));
}

} // namespace DRAMPower

#endif /* DRAMPOWER_UTIL_IMPLICITCOMMANDHELPERS_H */
