#pragma once

#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/stream_digest.hpp"

namespace porytiles {

/**
 * @brief Creates a unique temporary directory.
 *
 * @details
 * Creates a new directory in the system's temporary directory path with a random hex suffix. The directory name follows
 * the pattern "porytiles_<random_hex>". If directory creation fails (e.g., due to a collision), it retries up to 1000
 * times before panicking.
 *
 * @return The path to the newly created temporary directory.
 * @post The returned path points to an existing, empty directory.
 */
[[nodiscard]] inline std::filesystem::path create_tmpdir()
{
    int max_tries = 1000;
    auto tmp_dir = std::filesystem::temp_directory_path();
    int i = 0;
    std::random_device random_device;
    std::mt19937 mersenne_prng(random_device());
    std::uniform_int_distribution<uint64_t> uniform_int_distribution(0);
    std::filesystem::path path;
    while (true) {
        std::stringstream string_stream;
        string_stream << std::hex << uniform_int_distribution(mersenne_prng);
        path = tmp_dir / ("porytiles_" + string_stream.str());
        if (std::filesystem::create_directory(path)) {
            break;
        }
        if (i == max_tries) {
            panic("create_tmpdir: exceeded maximum retries");
        }
        i++;
    }
    return path;
}

/**
 * @brief Checks if two files have identical contents.
 *
 * @details
 * Compares two files using a two-stage approach for efficiency. First, it checks if the files have the same size (a
 * fast metadata check). If sizes match, it computes MD5 digests of both files and compares them. Returns false if the
 * second file doesn't exist, if either file cannot be opened, or if the contents differ.
 *
 * @param file_a Path to the first file (must exist).
 * @param file_b Path to the second file (may or may not exist).
 * @return True if both files exist and have identical contents, false otherwise.
 */
[[nodiscard]] inline bool files_are_identical(const std::filesystem::path &file_a, const std::filesystem::path &file_b)
{
    if (!std::filesystem::exists(file_b)) {
        return false;
    }
    if (std::filesystem::file_size(file_a) != std::filesystem::file_size(file_b)) {
        return false;
    }
    StreamDigest digest;
    std::ifstream stream_a{file_a, std::ios::binary};
    std::ifstream stream_b{file_b, std::ios::binary};
    if (!stream_a || !stream_b) {
        return false;
    }
    return digest.digest(stream_a) == digest.digest(stream_b);
}

/**
 * @brief Strips all extensions from a path, returning the path with only the stem.
 *
 * @details
 * This function repeatedly removes extensions from the filename portion of a path until no extensions remain. This is
 * useful for files with multiple extensions like "tiles.4bpp.smol" which should become "tiles". The directory portion
 * of the path is preserved. Dot files (e.g., ".gitignore") are handled correctly - the leading dot is part of the
 * filename, not an extension, so they are returned unchanged.
 *
 * @param path The path to strip extensions from.
 * @return A new path with all extensions removed from the filename.
 *
 * @par Examples
 * - `"tiles.png"` -> `"tiles"`
 * - `"tiles.4bpp.smol"` -> `"tiles"`
 * - `"data/tiles.4bpp.smol"` -> `"data/tiles"`
 * - `".gitignore"` -> `".gitignore"`
 * - `"tiles"` -> `"tiles"`
 */
[[nodiscard]] inline std::filesystem::path strip_all_extensions(const std::filesystem::path &path)
{
    auto dir = path.parent_path();
    auto filename = path.filename();
    while (!filename.extension().empty()) {
        filename = filename.stem();
    }
    if (dir.empty()) {
        return filename;
    }
    return dir / filename;
}

} // namespace porytiles
