#pragma once

#include <expected>
#include <string>
#include <typeindex>

namespace porytiles2 {

/**
 * @brief A specification for artifacts required or produced by orchestration operations.
 *
 * @details
 * ArtifactDeclaration provides a POD-like class that Operations use to declare input and output
 * artifact metadata. Each declaration specifies a key name, expected type, and optional
 * human-readable description. This enables the orchestration framework to validate that
 * ArtifactBundle contents match Operation requirements at runtime.
 */
class ArtifactDeclaration {
  public:
    /**
     * @brief Constructs an artifact declaration with the specified key and type.
     *
     * @details
     * The description is initialized to match the key by default, but can be
     * customized using set_description().
     *
     * @param key The unique identifier for this artifact
     * @param type The expected C++ type represented as `std::type_index`
     */
    ArtifactDeclaration(std::string key, const std::type_index type)
        : key_{std::move(key)}, expected_type_{type}, desc_{key} {}

    /**
     * @brief Gets the artifact's unique identifier.
     *
     * @return const reference to the key string
     */
    [[nodiscard]] const std::string &key() const {
        return key_;
    }

    /**
     * @brief Gets the expected type information for this artifact.
     *
     * @return const reference to the std::type_index representing the expected type
     */
    [[nodiscard]] const std::type_index &expected_type() const {
        return expected_type_;
    }

    /**
     * @brief Gets the human-readable description of this artifact.
     *
     * @return const reference to the description string
     */
    [[nodiscard]] const std::string &description() const {
        return desc_;
    }

    /**
     * @brief Sets a custom description for this artifact.
     *
     * @param desc The new description text to associate with this artifact
     */
    void set_description(const std::string &desc) {
        desc_ = desc;
    }

  private:
    std::string key_;
    std::type_index expected_type_;
    std::string desc_;
};

} // namespace porytiles2
