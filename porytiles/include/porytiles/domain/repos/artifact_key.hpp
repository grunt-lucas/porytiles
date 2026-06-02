#pragma once

#include <compare>
#include <functional>
#include <string>

namespace porytiles {

/**
 * @brief A type-safe wrapper for artifact keys.
 *
 * @details
 * The ArtifactKey class provides a strong type wrapper around std::string to represent artifact keys in the compilation
 * system. This improves type safety and makes the concept of artifact keys more explicit in the codebase.
 *
 * The class supports usage in all standard containers, including std::set, std::map, and std::unordered_map, through
 * appropriate comparison operators and hash specialization.
 */
class ArtifactKey {
  public:
    /**
     * @brief Constructs an ArtifactKey from a string value.
     *
     * @param key The string value to wrap
     */
    explicit ArtifactKey(std::string key) : key_{std::move(key)} {}

    [[nodiscard]] const std::string &key() const
    {
        return key_;
    }

    [[nodiscard]] bool operator==(const ArtifactKey &other) const = default;

    [[nodiscard]] auto operator<=>(const ArtifactKey &other) const = default;

  private:
    std::string key_;
};

} // namespace porytiles

// Hash specialization for std::unordered_map support
template <>
struct std::hash<porytiles::ArtifactKey> {
    std::size_t operator()(const porytiles::ArtifactKey &key) const noexcept
    {
        return std::hash<std::string>{}(key.key());
    }
};
