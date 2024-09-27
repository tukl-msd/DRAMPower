#ifndef CLI_VALIDATORS_H
#define CLI_VALIDATORS_H

#include <cli11/CLI11.hpp>
#include <stdint.h>

namespace validators
{

#define MAX_SYMLINK_RESOLVE_ITERATIONS 10

struct EnsureFileExists_t : public CLI::Validator {
    EnsureFileExists_t();
};
const static EnsureFileExists_t EnsureFileExists;

} // namespace validators

#endif /* CLI_VALIDATORS_H */
