#pragma once

#include <string>
#include <vector>

#include "porytiles2/xcut/diagnostics/user_diagnostics.hpp"
#include "porytiles2/xcut/result/error.hpp"

namespace porytiles2 {

/**
 * @brief Testing implementation of UserDiagnostics that buffers all diagnostic output.
 *
 * @details
 * BufferedUserDiagnostics provides a test-friendly implementation of the UserDiagnostics interface that stores all
 * diagnostic messages in memory buffers instead of outputting them. This enables unit tests to verify that appropriate
 * diagnostic messages were generated during execution.
 *
 * Each diagnostic type (notes, warnings, errors, etc.) is stored in its own dedicated buffer as a vector of message
 * lines. Tests can access these buffers to verify both the presence and content of diagnostic messages.
 *
 * This implementation is intended exclusively for testing scenarios where diagnostic output needs to be captured and
 * inspected programmatically.
 */
class BufferedUserDiagnostics final : public UserDiagnostics {
  public:
    void note(const std::vector<std::string> &lines) const override;

    void warn_note(const std::string &tag, const std::vector<std::string> &lines) const override;

    void warn(const std::string &tag, const std::vector<std::string> &lines) const override;

    void err(const std::vector<std::string> &lines) const override;

    void emit_fatal_proximate(const Error &err) const override;

    void emit_fatal_step(const Error &err) const override;

    void emit_fatal_root(const Error &err) const override;

    /**
     * @brief Get the buffered note messages.
     *
     * @details
     * Returns a reference to the vector containing all note messages that were passed to note(). Each element
     * represents one call to note(), stored as a vector of lines.
     *
     * @return Reference to the note buffer
     */
    [[nodiscard]] const std::vector<std::vector<std::string>> &notes() const
    {
        return notes_;
    }

    /**
     * @brief Get the buffered warning note messages.
     *
     * @details
     * Returns a reference to the vector containing all warning note messages that were passed to warn_note(). Each
     * element represents one call to warn_note(), stored as a vector of lines.
     *
     * @return Reference to the warning note buffer
     */
    [[nodiscard]] const std::vector<std::vector<std::string>> &warn_notes() const
    {
        return warn_notes_;
    }

    /**
     * @brief Get the buffered warning messages.
     *
     * @details
     * Returns a reference to the vector containing all warning messages that were passed to warn(). Each element
     * represents one call to warn(), stored as a vector of lines.
     *
     * @return Reference to the warning buffer
     */
    [[nodiscard]] const std::vector<std::vector<std::string>> &warnings() const
    {
        return warnings_;
    }

    /**
     * @brief Get the buffered error messages.
     *
     * @details
     * Returns a reference to the vector containing all error messages that were passed to err(). Each element
     * represents one call to err(), stored as a vector of lines.
     *
     * @return Reference to the error buffer
     */
    [[nodiscard]] const std::vector<std::vector<std::string>> &errors() const
    {
        return errors_;
    }

    /**
     * @brief Get the buffered fatal proximate error messages.
     *
     * @details
     * Returns a reference to the vector containing all fatal proximate error messages that were passed to
     * emit_fatal_proximate(). Each element represents one call to emit_fatal_proximate(), stored as a string.
     *
     * @return Reference to the fatal proximate buffer
     */
    [[nodiscard]] const std::vector<std::string> &fatal_proximates() const
    {
        return fatal_proximates_;
    }

    /**
     * @brief Get the buffered fatal step error messages.
     *
     * @details
     * Returns a reference to the vector containing all fatal step error messages that were passed to
     * emit_fatal_step(). Each element represents one call to emit_fatal_step(), stored as a string.
     *
     * @return Reference to the fatal step buffer
     */
    [[nodiscard]] const std::vector<std::string> &fatal_steps() const
    {
        return fatal_steps_;
    }

    /**
     * @brief Get the buffered fatal root error messages.
     *
     * @details
     * Returns a reference to the vector containing all fatal root error messages that were passed to
     * emit_fatal_root(). Each element represents one call to emit_fatal_root(), stored as a string.
     *
     * @return Reference to the fatal root buffer
     */
    [[nodiscard]] const std::vector<std::string> &fatal_roots() const
    {
        return fatal_roots_;
    }

  private:
    mutable std::vector<std::vector<std::string>> notes_;
    mutable std::vector<std::vector<std::string>> warn_notes_;
    mutable std::vector<std::vector<std::string>> warnings_;
    mutable std::vector<std::vector<std::string>> errors_;
    mutable std::vector<std::string> fatal_proximates_;
    mutable std::vector<std::string> fatal_steps_;
    mutable std::vector<std::string> fatal_roots_;
};

} // namespace porytiles2
