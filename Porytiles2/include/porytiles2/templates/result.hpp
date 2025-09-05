#pragma once

#include <expected>
#include <memory>
#include <string>
#include <type_traits>

#include "porytiles2/templates/error.hpp"
#include "porytiles2/templates/panic.hpp"

namespace porytiles2 {

/**
 * @brief A result with some type `T` on success, otherwise an error of type `E`.
 *
 * @details
 * Many Porytiles operations need to return either an expected result or some description of what went wrong during
 * result computation. This type alias is a convenient wrapper for the stdlib `std::expected`, which provides this exact
 * functionality. The `std::string` error type will typically be some description of what went wrong. However, the alias
 * supports a custom user type for the error type if a string is not sufficient.
 *
 * @tparam T The type of the expected result
 * @tparam E The error type, defaults to `std::string`
 */
template <typename T, typename E = std::string>
using Result = std::expected<T, E>;

/**
 * @brief A result type that maintains a chainable sequence of errors for debugging and error reporting.
 *
 * @details
 * ChainableResult extends the concept of std::expected by maintaining a chain of error propagation through multiple
 * layers of the application. Unlike a simple Result that only stores the immediate error, ChainableResult maintains a
 * full chain of errors from the originating cause up through each layer that adds context. This is particularly useful
 * for debugging complex failures where understanding the root cause requires knowing the full context of how an error
 * propagated through the system.
 *
 * The class enforces that error type E must derive from the Error interface, ensuring all errors in the chain can be
 * properly cloned and formatted. The error chain is stored as a vector of unique_ptr<Error> objects, allowing
 * polymorphic error types while maintaining proper ownership semantics.
 *
 * ChainableResult implements move-only semantics to ensure efficient transfer of the error chain without unnecessary
 * copying of potentially large error chains.
 *
 * @tparam T The type of the expected success value
 * @tparam E The error type, must be derived from Error interface
 */
template <typename T, typename E>
class ChainableResult {
  public:
    /**
     * @brief Default constructor for void specialization.
     *
     * @details
     * This constructor is only enabled when T is void, allowing construction of a successful result with no value using
     * the syntax `return {}`. This matches the behavior of std::expected<void, E> and provides ergonomic success
     * construction for void results.
     */
    ChainableResult()
        requires std::is_void_v<T>
        : result_{}
    {
    }

    /**
     * @brief Constructs a ChainableResult from a success value.
     *
     * @details
     * This constructor allows implicit conversion from a success value of type T to a ChainableResult. The result is
     * stored as a successful value with no error chain. This provides ergonomic construction for success cases, similar
     * to std::expected's implicit construction from T.
     *
     * @param value The success value to store
     */
    ChainableResult(T value) : result_{std::move(value)} {}

    /**
     * @brief Constructs a ChainableResult from an error value.
     *
     * @details
     * This constructor allows implicit conversion from an error value of type E to a ChainableResult. The error is
     * stored as the initial error in the chain. This provides ergonomic construction for error cases at the bottom
     * level of an error chain, similar to std::expected's construction from std::unexpected.
     *
     * @param error The error value to store
     */
    ChainableResult(const E &error) : result_{std::unexpected{error}}
    {
        static_assert(std::is_base_of_v<Error, E>, "ChainableResult error type E must be derived from Error");
        error_chain_.push_back(std::make_unique<E>(error));
    }

    /**
     * @brief Constructs a ChainableResult by chaining a new error with an existing error chain.
     *
     * @details
     * This constructor creates a new error result that includes both a new error message and the complete error chain
     * from a cause result. This is the primary mechanism for building error context as errors propagate up through
     * application layers. The new error is added to the beginning of the chain, followed by all errors from the cause
     * result's chain.
     *
     * @tparam CauseT The success type of the cause result (unused but required for template matching)
     * @tparam CauseE The error type of the cause result, must be derived from Error
     * @param error The new error to add at this level
     * @param cause_result The ChainableResult containing the error chain to chain
     */
    template <typename CauseT, typename CauseE>
    explicit ChainableResult(const E &error, const ChainableResult<CauseT, CauseE> &cause_result)
        : result_{std::unexpected{error}}
    {
        static_assert(std::is_base_of_v<Error, E>, "ChainableResult error type E must be derived from Error");
        error_chain_.push_back(std::make_unique<E>(result_.error()));
        add_cause(cause_result);
    }

    /**
     * @brief Static factory method for chaining errors.
     *
     * @details
     * Provides a more readable way to chain errors compared to direct constructor usage. This method creates a new
     * ChainableResult that combines a new error with an existing error chain from a cause result.
     *
     * @tparam CauseT The success type of the cause result
     * @tparam CauseE The error type of the cause result
     * @param error The new error to add at this level
     * @param cause The ChainableResult containing the error chain to chain
     * @return A new ChainableResult containing the combined error chain
     */
    template <typename CauseT, typename CauseE>
    [[nodiscard]] static ChainableResult chain(const E &error, const ChainableResult<CauseT, CauseE> &cause)
    {
        return ChainableResult{error, cause};
    }

    /*
     * Move-only semantics
     */
    ChainableResult(ChainableResult &&) = default;
    ChainableResult &operator=(ChainableResult &&) = default;
    ChainableResult(const ChainableResult &) = delete;
    ChainableResult &operator=(const ChainableResult &) = delete;

    /**
     * @brief Adds all errors from another ChainableResult's chain to this result's chain.
     *
     * @details
     * This method appends the complete error chain from a cause result to the current error chain. Each error in the
     * cause's chain is cloned to maintain proper ownership semantics. The method will panic if the cause_result
     * contains a success value rather than an error, as this would indicate a programming error.
     *
     * @tparam OtherT The success type of the cause result
     * @tparam OtherE The error type of the cause result
     * @param cause_result The ChainableResult whose error chain should be appended
     */
    template <typename OtherT, typename OtherE>
    void add_cause(const ChainableResult<OtherT, OtherE> &cause_result)
    {
        if (cause_result.has_value()) {
            panic("cause_result has a value, but should have an error");
        }
        // Clone errors from cause_result's chain since we can't move from const
        for (const std::unique_ptr<Error> &err : cause_result.chain()) {
            error_chain_.push_back(err->clone());
        }
    }

    /**
     * @brief Checks whether the result contains a success value.
     *
     * @return True if the result contains a success value, false if it contains an error
     */
    [[nodiscard]] bool has_value() const
    {
        return result_.has_value();
    }

    /**
     * @brief Returns a reference to the contained success value.
     *
     * @details
     * This method provides mutable access to the success value. It will throw std::bad_expected_access if called when
     * the result contains an error rather than a success value.
     *
     * @return A mutable reference to the success value
     */
    [[nodiscard]] T &value()
    {
        return result_.value();
    }

    /**
     * @brief Returns a const reference to the contained success value.
     *
     * @details
     * This method provides immutable access to the success value. It will throw std::bad_expected_access if called when
     * the result contains an error rather than a success value.
     *
     * @return A const reference to the success value
     */
    [[nodiscard]] const T &value() const
    {
        return result_.value();
    }

    /**
     * @brief Returns a const reference to the immediate error.
     *
     * @details
     * This returns only the immediate error at this level, not the full error chain. To access the complete error
     * history, use the chain() method instead.
     *
     * @return A const reference to the error value
     */
    [[nodiscard]] const E &error() const
    {
        return result_.error();
    }

    /**
     * @brief Returns the complete error chain.
     *
     * @details
     * The error chain contains all errors in the chain, starting with the most recent error (added at this level) and
     * proceeding through to the original root cause. Each error in the chain is owned by this ChainableResult through
     * unique_ptr.
     *
     * @return A const reference to the vector of error pointers representing the full error chain
     */
    [[nodiscard]] const std::vector<std::unique_ptr<Error>> &chain() const
    {
        return error_chain_;
    }

  private:
    std::expected<T, E> result_;
    std::vector<std::unique_ptr<Error>> error_chain_;
};

} // namespace porytiles2
