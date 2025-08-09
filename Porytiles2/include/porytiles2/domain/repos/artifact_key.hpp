#pragma once

#include <compare>
#include <functional>
#include <string>

namespace porytiles2 {

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

    /**
     * @brief Gets the underlying string value.
     *
     * @return The wrapped string value
     */
    [[nodiscard]] const std::string &key() const {
        return key_;
    }

    /**
     * @brief Equality comparison operator.
     *
     * @param other The other ArtifactKey to compare with
     * @return True if the keys are equal, false otherwise
     */
    [[nodiscard]] bool operator==(const ArtifactKey &other) const = default;

    /**
     * @brief Three-way comparison operator for ordered containers.
     *
     * @param other The other ArtifactKey to compare with
     * @return The comparison result (strong ordering)
     */
    [[nodiscard]] std::strong_ordering operator<=>(const ArtifactKey &other) const {
        return key_ <=> other.key_;
    }

  private:
    std::string key_;
};

} // namespace porytiles2

// Hash specialization for std::unordered_map support
template <>
struct std::hash<porytiles2::ArtifactKey> {
    /**
     * @brief Hash function for ArtifactKey.
     *
     * @param key The ArtifactKey to hash
     * @return Hash value for the key
     */
    std::size_t operator()(const porytiles2::ArtifactKey &key) const noexcept {
        return std::hash<std::string>{}(key.key());
    }
};
