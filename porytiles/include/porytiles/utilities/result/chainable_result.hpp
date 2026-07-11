#pragma once

#include <expected>
#include <memory>
#include <string>
#include <type_traits>

#include "porytiles/utilities/panic/panic.hpp"
#include "porytiles/utilities/result/error.hpp"

namespace porytiles {

/// @brief A result type that maintains a chainable sequence of errors for debugging and error reporting.
///
/// @details
/// ChainableResult extends the concept of std::expected by maintaining a chain of error propagation through multiple
/// layers of the application. Unlike a simple std::expected that only stores the immediate error, ChainableResult
/// maintains a full chain of errors from the originating cause up through each layer that adds context. This is
/// particularly useful for debugging complex failures where understanding the root cause requires knowing the full
/// context of how an error propagated through the system.
///
/// The class enforces that error type E must derive from the Error interface, ensuring all errors in the chain can be
/// properly cloned and formatted. The error chain is stored as a vector of unique_ptr<Error> objects, allowing
/// polymorphic error types while maintaining proper ownership semantics.
///
/// @tparam T The type of the expected success value
/// @tparam E The error type, must be derived from Error interface
template <typename T, typename E = FormattableError>
class ChainableResult {
  public:
    /// @brief Constructs a ChainableResult from a success value.
    ///
    /// @details
    /// This constructor allows implicit conversion from a success value of type T to a ChainableResult. The result is
    /// stored as a successful value with no error chain. This provides ergonomic construction for success cases,
    /// similar to std::expected's implicit construction from T.
    ///
    /// @param value The success value to store
    // NOLINTNEXTLINE(google-explicit-constructor)
    ChainableResult(T value) : result_{std::move(value)} {}

    /// @brief Constructs a ChainableResult from an error value.
    ///
    /// @details
    /// This constructor allows implicit conversion from an error value of type E to a ChainableResult. The error is
    /// stored as the initial error in the chain. This provides ergonomic construction for error cases at the bottom
    /// level of an error chain, similar to std::expected's construction from std::unexpected.
    ///
    /// @param error The error value to store
    // NOLINTNEXTLINE(google-explicit-constructor)
    ChainableResult(const E &error) : result_{std::unexpected{error}}
    {
        static_assert(std::is_base_of_v<Error, E>, "ChainableResult error type E must be derived from Error");
        error_chain_.push_back(std::make_unique<E>(error));
    }

    /// @brief Constructs a ChainableResult by chaining a new error with an existing error chain.
    ///
    /// @details
    /// This constructor creates a new error result that includes both a new error message and the complete error chain
    /// from a cause result. This is the primary mechanism for building error context as errors propagate up through
    /// application layers. The new error is added to the beginning of the chain, followed by all errors from the cause
    /// result's chain.
    ///
    /// @tparam CauseT The success type of the cause result (unused but required for template matching)
    /// @tparam CauseE The error type of the cause result, must be derived from Error
    /// @param error The new error to add at this level
    /// @param cause_result The ChainableResult containing the error chain to chain
    template <typename CauseT, typename CauseE>
    explicit ChainableResult(const E &error, const ChainableResult<CauseT, CauseE> &cause_result)
        : result_{std::unexpected{error}}
    {
        static_assert(std::is_base_of_v<Error, E>, "ChainableResult error type E must be derived from Error");
        error_chain_.push_back(std::make_unique<E>(result_.error()));
        add_cause(cause_result);
    }

    /// @brief Constructs a ChainableResult by chaining an existing error chain with a default-constructed error.
    ///
    /// @details
    /// This constructor creates a new error result that includes a default-constructed FormattableError and the
    /// complete error chain from a cause result. This is useful when the current layer doesn't need to add additional
    /// error context but needs to convert the result to a different success type. The default-constructed error is
    /// added to the beginning of the chain, followed by all errors from the cause result's chain.
    ///
    /// @tparam CauseT The success type of the cause result (unused but required for template matching)
    /// @tparam CauseE The error type of the cause result, must be derived from Error
    /// @param cause_result The ChainableResult containing the error chain to chain
    template <typename CauseT, typename CauseE>
    explicit ChainableResult(const ChainableResult<CauseT, CauseE> &cause_result) : result_{std::unexpected{E{}}}
    {
        static_assert(std::is_base_of_v<Error, E>, "ChainableResult error type E must be derived from Error");
        error_chain_.push_back(std::make_unique<E>(result_.error()));
        add_cause(cause_result);
    }

    // Move-only semantics
    ChainableResult(ChainableResult &&) = default;
    ChainableResult &operator=(ChainableResult &&) = default;
    ChainableResult(const ChainableResult &) = delete;
    ChainableResult &operator=(const ChainableResult &) = delete;

    /// @brief Adds all errors from another ChainableResult's chain to this result's chain.
    ///
    /// @details
    /// This method appends the complete error chain from a cause result to the current error chain. Each error in the
    /// cause's chain is cloned to maintain proper ownership semantics. The method will panic if the cause_result
    /// contains a success value rather than an error, as this would indicate a programming error.
    ///
    /// @tparam OtherT The success type of the cause result
    /// @tparam OtherE The error type of the cause result
    /// @param cause_result The ChainableResult whose error chain should be appended
    template <typename OtherT, typename OtherE>
    void add_cause(const ChainableResult<OtherT, OtherE> &cause_result)
    {
        assert_or_panic(!cause_result.has_value(), "cause_result has a value, but should have an error");

        // Clone errors from cause_result's chain since we can't move from const
        for (const std::unique_ptr<Error> &err : cause_result.chain()) {
            error_chain_.push_back(err->clone());
        }
    }

    /// @brief Checks whether the result contains a success value.
    ///
    /// @return True if the result contains a success value, false if it contains an error
    [[nodiscard]] bool has_value() const
    {
        return result_.has_value();
    }

    /// @brief Returns a reference to the contained success value.
    ///
    /// @details
    /// This method provides mutable access to the success value. It will throw std::bad_expected_access if called when
    /// the result contains an error rather than a success value.
    ///
    /// @return A mutable reference to the success value
    [[nodiscard]] T &value() &
    {
        return result_.value();
    }

    /// @brief Returns a const reference to the contained success value.
    ///
    /// @details
    /// This method provides immutable access to the success value. It will throw std::bad_expected_access if called
    /// when the result contains an error rather than a success value.
    ///
    /// @return A const reference to the success value
    [[nodiscard]] const T &value() const &
    {
        return result_.value();
    }

    /// @brief Returns an rvalue reference to the contained success value.
    ///
    /// @details
    /// This method provides move access to the success value when called on an rvalue ChainableResult. It enables
    /// efficient extraction of the value when the ChainableResult itself is a temporary or has been moved from. It will
    /// throw std::bad_expected_access if called when the result contains an error rather than a success value.
    ///
    /// @return An rvalue reference to the success value
    [[nodiscard]] T &&value() &&
    {
        return std::move(result_).value();
    }

    /// @brief Returns a const reference to the immediate error.
    ///
    /// @details
    /// This returns only the immediate error at this level, not the full error chain. To access the complete error
    /// history, use the chain() method instead.
    ///
    /// @return A const reference to the error value
    [[nodiscard]] const E &error() const
    {
        return result_.error();
    }

    /// @brief Returns the complete error chain.
    ///
    /// @details
    /// The error chain contains all errors in the chain, starting with the most recent error (added at this level) and
    /// proceeding through to the original root cause. Each error in the chain is owned by this ChainableResult through
    /// unique_ptr.
    ///
    /// @return A const reference to the vector of error pointers representing the full error chain
    [[nodiscard]] const std::vector<std::unique_ptr<Error>> &chain() const
    {
        return error_chain_;
    }

  protected:
    // Protected default constructor for use by the void specialization
    ChainableResult() = default;

  private:
    std::expected<T, E> result_;
    std::vector<std::unique_ptr<Error>> error_chain_;
};

namespace detail {
// An empty struct to use as a placeholder for `void` in the ChainableResult specialization.
struct Empty {};
} // namespace detail

/// @brief Template specialization of ChainableResult for void success type.
///
/// @details
/// This specialization handles the case where no value needs to be returned on success, similar to
/// std::expected<void, E>. It is implemented as a thin wrapper around the primary template, using a
/// private `detail::Empty` struct as a placeholder for the `void` type. This approach avoids code
/// duplication while providing a `void`-like interface.
///
/// @tparam E The error type, must be derived from Error interface
template <typename E>
class ChainableResult<void, E> : public ChainableResult<detail::Empty, E> {
    using Base = ChainableResult<detail::Empty, E>;

  public:
    /// @brief Default constructor creating a successful void result.
    ///
    /// @details
    /// Constructs a ChainableResult representing success with no value. This allows functions
    /// returning ChainableResult<void, E> to use the syntax `return {}` for successful
    /// completion.
    ChainableResult() : Base{detail::Empty{}} {}

    /// @brief Constructs a ChainableResult from an error value.
    ///
    /// @details
    /// This constructor allows implicit conversion from an error value of type E to a
    /// ChainableResult. The error is stored as the initial error in the chain. This provides
    /// ergonomic construction for error cases at the bottom level of an error chain.
    ///
    /// @param error The error value to store
    ChainableResult(const E &error) : Base{error} {}

    /// @brief Constructs a ChainableResult by chaining a new error with an existing error chain.
    ///
    /// @details
    /// This constructor creates a new error result that includes both a new error message and
    /// the complete error chain from a cause result. This is the primary mechanism for building
    /// error context as errors propagate up through application layers. The new error is added
    /// to the beginning of the chain, followed by all errors from the cause result's chain.
    ///
    /// @tparam CauseT The success type of the cause result (may be void)
    /// @tparam CauseE The error type of the cause result, must be derived from Error
    /// @param error The new error to add at this level
    /// @param cause_result The ChainableResult containing the error chain to chain
    template <typename CauseT, typename CauseE>
    explicit ChainableResult(const E &error, const ChainableResult<CauseT, CauseE> &cause_result)
        : Base{error, cause_result}
    {
    }

    /// @brief Constructs a ChainableResult by chaining an existing error chain with a default-constructed error.
    ///
    /// @details
    /// This constructor creates a new error result that includes a default-constructed FormattableError and the
    /// complete error chain from a cause result. This is useful when the current layer doesn't need to add additional
    /// error context but needs to convert the result to a different success type.
    ///
    /// @tparam CauseT The success type of the cause result (may be void)
    /// @tparam CauseE The error type of the cause result, must be derived from Error
    /// @param cause_result The ChainableResult containing the error chain to chain
    template <typename CauseT, typename CauseE>
    explicit ChainableResult(const ChainableResult<CauseT, CauseE> &cause_result) : Base{cause_result}
    {
    }

    /// @brief Accesses the void success value.
    ///
    /// @details
    /// For the void specialization, this method returns void and simply validates that the
    /// result is indeed a success. It will throw std::bad_expected_access if called when
    /// the result contains an error.
    void value() &
    {
        Base::value();
    }

    /// @brief Accesses the void success value (const version).
    ///
    /// @details
    /// For the void specialization, this method returns void and simply validates that the
    /// result is indeed a success. It will throw std::bad_expected_access if called when
    /// the result contains an error.
    void value() const &
    {
        Base::value();
    }

    /// @brief Accesses the void success value (rvalue version).
    ///
    /// @details
    /// For the void specialization, this method returns void and simply validates that the
    /// result is indeed a success when called on an rvalue ChainableResult. It will throw
    /// std::bad_expected_access if called when the result contains an error.
    void value() &&
    {
        std::move(*this).Base::value();
    }
};

/// @brief Unwraps a ChainableResult, chaining a new error message on failure.
///
/// @details
/// This macro provides a succinct way to handle ChainableResult unwrapping with error propagation. It evaluates the
/// expression, checks if it contains a value, and either assigns the value to the variable or returns early with a new
/// error chained to the existing error chain. This reduces the common 6-line error handling pattern to a single line.
///
/// If the result contains an error, the macro returns from the current function with a ChainableResult containing the
/// original error chain plus the new error message provided.
///
/// The variadic args are forwarded directly to the FormattableError constructor, so you can pass a plain string or a
/// format string with FormatParam arguments. When passing FormatParam inside the macro call, use parentheses instead of
/// braces to avoid preprocessor comma-splitting: @c FormatParam(name, Style::bold) not @c FormatParam{name,
/// Style::bold}.
///
/// @param var The variable name to assign the unwrapped value to
/// @param expr The expression returning a ChainableResult
/// @param return_type The success type of the ChainableResult to return on error
/// @param ... Arguments forwarded to the FormattableError constructor (format string, optional FormatParams)
#define PT_TRY_ASSIGN_CHAIN_ERR(var, expr, return_type, ...)                                                           \
    auto var##_result = (expr);                                                                                        \
    if (!var##_result.has_value()) {                                                                                   \
        return ChainableResult<return_type>{FormattableError{__VA_ARGS__}, var##_result};                              \
    }                                                                                                                  \
    auto var = std::move(var##_result).value();

/// @brief Unwraps a ChainableResult, passing through the error chain with an empty FormattableError when types differ.
///
/// @details
/// This macro provides a succinct way to handle ChainableResult unwrapping with error passthrough when the inner
/// result's success type differs from the outer function's return type. It evaluates the expression, checks if it
/// contains a value, and either assigns the value to the variable or returns early with an empty FormattableError
/// chained to the existing error chain. This is useful when the current layer doesn't need to add additional error
/// context but the result types don't match (e.g., inner function returns ChainableResult<Foo, E> but outer returns
/// ChainableResult<Bar, E>).
///
/// If the result contains an error, the macro returns from the current function with a new ChainableResult<return_type>
/// containing an empty FormattableError chained to the original error chain.
///
/// @param var The variable name to assign the unwrapped value to
/// @param expr The expression returning a ChainableResult
/// @param return_type The success type of the ChainableResult to return on error (differs from expr's success type)
#define PT_TRY_ASSIGN_PASS_ERR(var, expr, return_type)                                                                 \
    auto var##_result = (expr);                                                                                        \
    if (!var##_result.has_value()) {                                                                                   \
        return ChainableResult<return_type>{FormattableError{}, var##_result};                                         \
    }                                                                                                                  \
    auto var = std::move(var##_result).value();

/// @brief Unwraps a ChainableResult, passing through the error unchanged when types match.
///
/// @details
/// This macro provides a succinct way to handle ChainableResult unwrapping with error passthrough when the inner
/// result's type matches the outer function's return type. It evaluates the expression, checks if it contains a value,
/// and either assigns the value to the variable or returns early with the same error result unchanged. This is useful
/// when the current layer doesn't need to add additional error context and the result types match exactly.
///
/// If the result contains an error, the macro returns from the current function with the same error result unchanged,
/// preserving the existing error chain without any modifications.
///
/// @param var The variable name to assign the unwrapped value to
/// @param expr The expression returning a ChainableResult with the same type as the outer function's return type
#define PT_TRY_ASSIGN_PASS_SAME_ERR(var, expr)                                                                         \
    auto var##_result = (expr);                                                                                        \
    if (!var##_result.has_value()) {                                                                                   \
        return var##_result;                                                                                           \
    }                                                                                                                  \
    auto var = std::move(var##_result).value();

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_CHAIN_ERR_EXPAND(expr, return_type, counter, ...)                                           \
    auto pt_try_call_result_##counter = (expr);                                                                        \
    if (!pt_try_call_result_##counter.has_value()) {                                                                   \
        return ChainableResult<return_type>{FormattableError{__VA_ARGS__}, pt_try_call_result_##counter};              \
    }

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_CHAIN_ERR_IMPL(expr, return_type, counter, ...)                                             \
    PT_DETAIL_TRY_CALL_CHAIN_ERR_EXPAND(expr, return_type, counter, __VA_ARGS__)

/// @brief Unwraps a void ChainableResult, chaining a new error message on failure.
///
/// @details
/// This macro provides a succinct way to handle void-returning ChainableResult unwrapping with error propagation. It
/// evaluates the expression, checks if it contains a success value, and either continues execution or returns early
/// with a new error chained to the existing error chain. This is the void equivalent of PT_TRY_ASSIGN_CHAIN_ERR.
///
/// If the result contains an error, the macro returns from the current function with a ChainableResult containing the
/// original error chain plus the new error message provided.
///
/// The variadic args are forwarded directly to the FormattableError constructor, so you can pass a plain string or a
/// format string with FormatParam arguments. When passing FormatParam inside the macro call, use parentheses instead of
/// braces to avoid preprocessor comma-splitting: @c FormatParam(name, Style::bold) not @c FormatParam{name,
/// Style::bold}.
///
/// Uses @c __LINE__ internally to generate unique variable names and avoid naming collisions between multiple
/// expansions within the same scope. Because the uniqueness token is the source line number, this macro must not be
/// invoked more than once on the same physical line — doing so would produce colliding local variable names.
///
/// @param expr The expression returning a ChainableResult<void, E>
/// @param return_type The success type of the ChainableResult to return on error
/// @param ... Arguments forwarded to the FormattableError constructor (format string, optional FormatParams)
#define PT_TRY_CALL_CHAIN_ERR(expr, return_type, ...)                                                                  \
    PT_DETAIL_TRY_CALL_CHAIN_ERR_IMPL(expr, return_type, __LINE__, __VA_ARGS__)

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_PASS_ERR_EXPAND(expr, return_type, counter)                                                 \
    auto pt_try_call_result_##counter = (expr);                                                                        \
    if (!pt_try_call_result_##counter.has_value()) {                                                                   \
        return ChainableResult<return_type>{FormattableError{}, pt_try_call_result_##counter};                         \
    }

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_PASS_ERR_IMPL(expr, return_type, counter)                                                   \
    PT_DETAIL_TRY_CALL_PASS_ERR_EXPAND(expr, return_type, counter)

/// @brief Unwraps a void ChainableResult, passing through the error chain with an empty FormattableError when types
/// differ.
///
/// @details
/// This macro provides a succinct way to handle void-returning ChainableResult unwrapping with error passthrough when
/// the inner result's success type differs from the outer function's return type. It evaluates the expression, checks
/// if it contains a success value, and either continues execution or returns early with an empty FormattableError
/// chained to the existing error chain. This is the void equivalent of PT_TRY_ASSIGN_PASS_ERR and is useful when the
/// current layer doesn't need to add additional error context but the result types don't match.
///
/// If the result contains an error, the macro returns from the current function with a new ChainableResult<return_type>
/// containing an empty FormattableError chained to the original error chain.
///
/// Uses @c __LINE__ internally to generate unique variable names and avoid naming collisions between multiple
/// expansions within the same scope. Because the uniqueness token is the source line number, this macro must not be
/// invoked more than once on the same physical line — doing so would produce colliding local variable names.
///
/// @param expr The expression returning a ChainableResult<void, E>
/// @param return_type The success type of the ChainableResult to return on error (differs from expr's success type)
#define PT_TRY_CALL_PASS_ERR(expr, return_type) PT_DETAIL_TRY_CALL_PASS_ERR_IMPL(expr, return_type, __LINE__)

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_PASS_SAME_ERR_EXPAND(expr, counter)                                                         \
    auto pt_try_call_result_##counter = (expr);                                                                        \
    if (!pt_try_call_result_##counter.has_value()) {                                                                   \
        return pt_try_call_result_##counter;                                                                           \
    }

// Internal implementation detail - do not use directly
#define PT_DETAIL_TRY_CALL_PASS_SAME_ERR_IMPL(expr, counter) PT_DETAIL_TRY_CALL_PASS_SAME_ERR_EXPAND(expr, counter)

/// @brief Unwraps a void ChainableResult, passing through the error unchanged when types match.
///
/// @details
/// This macro provides a succinct way to handle void-returning ChainableResult unwrapping with error passthrough when
/// the inner result's type matches the outer function's return type. It evaluates the expression, checks if it contains
/// a success value, and either continues execution or returns early with the same error result unchanged. This is the
/// void equivalent of PT_TRY_ASSIGN_PASS_SAME_ERR and is useful when the current layer doesn't need to add additional
/// error context and the result types match exactly.
///
/// If the result contains an error, the macro returns from the current function with the same error result unchanged,
/// preserving the existing error chain without any modifications.
///
/// Uses @c __LINE__ internally to generate unique variable names and avoid naming collisions between multiple
/// expansions within the same scope. Because the uniqueness token is the source line number, this macro must not be
/// invoked more than once on the same physical line — doing so would produce colliding local variable names.
///
/// @param expr The expression returning a ChainableResult<void, E> with the same type as the outer function's return
/// type
#define PT_TRY_CALL_PASS_SAME_ERR(expr) PT_DETAIL_TRY_CALL_PASS_SAME_ERR_IMPL(expr, __LINE__)

} // namespace porytiles
