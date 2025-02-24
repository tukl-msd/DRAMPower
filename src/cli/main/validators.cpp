#include "validators.h"

#include <string>
#include <filesystem>
#include <fstream>
#include <optional>

namespace validators {

struct ResolveSymlinkResult {
    enum class Type {
        FILE,
        DIRECTORY, // included for future default filename creation
        NOT_FOUND // File does not exist but a parent directory could exist
    };
    Type type;
    std::string path;
};

std::optional<ResolveSymlinkResult> resolve_symlink(const std::string &filepath, const uint_fast8_t max_iter)
{
    // Check if path exists and is a symlink
    if (!std::filesystem::is_symlink(filepath)) {
        // File does not exist or is not a symlink
        return std::nullopt;
    }

    // The path exists and is a symlink -> resolve the symlink chain
    std::filesystem::path resolved = filepath;
    for (uint_fast8_t iter = 0; iter < max_iter; ++iter) {
        resolved = std::filesystem::read_symlink(resolved);
        // Check if the provided path exists
        if (!std::filesystem::exists(resolved)) {
            // symlink resolved to a non-existing path
            return std::make_optional(ResolveSymlinkResult{
                ResolveSymlinkResult::Type::NOT_FOUND, // type
                resolved.string() // path
            }); 
        } else

        if (std::filesystem::is_regular_file(resolved)) {
            // symlink resolved to a file
            return std::make_optional(ResolveSymlinkResult{
                ResolveSymlinkResult::Type::FILE, // type
                resolved.string() // path
            }); 
        } else if (std::filesystem::is_directory(resolved)) {
            // symlink resolved to a non-file
            return std::make_optional(ResolveSymlinkResult{
                ResolveSymlinkResult::Type::DIRECTORY, // type 
                resolved.string() // path
            });
        } else if (std::filesystem::is_symlink(resolved)) {
            // symlink resolved to another symlink
            // continue to next iteration
        } else {
            // symlink resolved to an unknown type
            return std::nullopt;
        }
    }

    // The symlink chain was not resolved in the maximum number of iterations
    return std::nullopt;
}

// This function assumes the filepath does not exist
std::string createFileInParentDirectory(const std::filesystem::path& filepath)
{
// Check parent directory exists
    std::filesystem::path path = filepath.parent_path();

    // No path directory provided -> create the file in current directory
    if (path.empty() && filepath.is_relative()) {
        path = std::filesystem::current_path();
    }
    if (!std::filesystem::exists(path)) {
        // parent directory does not exist
        return std::string{"Parent directory does not exist"};
    } else if (!std::filesystem::is_directory(path)) {
        // parent is not a directory. The path is invalid
        return std::string{"Invlid path"};
    }

// File does not exist and the parent directory exists
    // Try to create the file
    try { // Catch std::ofstream exceptions
        std::ofstream file{filepath};
        if (!file.is_open()) {
            // Could not create the file
            // creation failed ofstream closes the file in destructor
            return std::string{"Could not create the file"};
        }
        // explicitly close the file to ensure that the file is created and closed
        file.close();
        if (!file.good()) {
            // Could not create the file
            return std::string{"Could not create the file"};
        }
    } catch (const std::exception&) {
        // Could not create the file
        return std::string{"Could not create the file"};
    }
// File was created successfully
    return std::string{};
}

EnsureFileExists_t::EnsureFileExists_t() {
    name_ = "EnsureFileExists";
    non_modifying_ = false; // The validator modifies the input for a symlink
    func_ = [](std::string &filepath) {
// Check if file exists
        if (std::filesystem::is_symlink(filepath)) {
// resolve symlink chain for at most MAX_SYMLINK_RESOLVE_ITERATIONS iterations
            auto resolved = resolve_symlink(filepath, MAX_SYMLINK_RESOLVE_ITERATIONS);
            if (!resolved) {
                return std::string{"Could not resolve the symlink"};
            }
            // resolved is valid
            if (resolved->type == ResolveSymlinkResult::Type::FILE) {
                // resolved to a file
                filepath = resolved->path;
                return std::string{};
            } else if (resolved->type == ResolveSymlinkResult::Type::DIRECTORY) {
                // resolved to a directory
                return std::string{"Path is a directory"};
            } else if (resolved->type == ResolveSymlinkResult::Type::NOT_FOUND) {
                // file not found but a parent directory could exist
                filepath = resolved->path;
                return createFileInParentDirectory(filepath);
            }
        } else if (std::filesystem::is_regular_file(filepath)) {
            // file exists
            return std::string{};
        } else if(std::filesystem::exists(filepath)) {
            // the path exists but is not a file or a symlink
            return std::string{"Path is not a file"};
        }
        // The filepath does not exist
// Try to create the file in the parent directory
        return createFileInParentDirectory(filepath);
    };
}

} // namespace validators