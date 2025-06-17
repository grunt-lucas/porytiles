#pragma once

#include <cstddef>
#include <utility>

namespace porytiles {

/**
 * @brief A smart pointer that represents a non-owning view of a resource.
 *
 * @details
 * The view_ptr allows for reading and writing and is semantically equivalent to a raw pointer. The purpose of view_ptr
 * is to explicitly signal that the pointed-to resource is not owned by the owner of the pointer.
 */
template <typename T>
class view_ptr {
  public:
    constexpr view_ptr() noexcept : ptr_(nullptr) {}

    explicit constexpr view_ptr(std::nullptr_t) noexcept : ptr_(nullptr) {}

    explicit view_ptr(T *p) noexcept : ptr_(p) {}

    view_ptr(const view_ptr &other) noexcept = default;

    view_ptr &operator=(const view_ptr &other) noexcept = default;

    view_ptr(view_ptr &&other) noexcept : ptr_(other.release()) {}

    view_ptr &operator=(view_ptr &&other) noexcept {
        ptr_ = other.release();
        return *this;
    }

    T *get() const noexcept {
        return ptr_;
    }

    T &operator*() const {
        // AssertOrPanic(ptr_ != nullptr, "Dereferencing a null view_ptr");
        return *ptr_;
    }

    T *operator->() const noexcept {
        // AssertOrPanic(ptr_ != nullptr, "Accessing member of a null view_ptr");
        return ptr_;
    }

    explicit operator bool() const noexcept {
        return ptr_ != nullptr;
    }

    T *release() noexcept {
        return std::exchange(ptr_, nullptr);
    }

    void reset(T *p = nullptr) noexcept {
        ptr_ = p;
    }

    void swap(view_ptr &other) noexcept {
        std::swap(ptr_, other.ptr_);
    }

  private:
    T *ptr_;
};

// Comparison operators
/// @todo make these members? pros vs. cons
template <typename T1, typename T2>
bool operator==(const view_ptr<T1> &lhs, const view_ptr<T2> &rhs) {
    return lhs.get() == rhs.get();
}

template <typename T1, typename T2>
bool operator!=(const view_ptr<T1> &lhs, const view_ptr<T2> &rhs) {
    return !(lhs == rhs);
}

template <typename T>
bool operator==(const view_ptr<T> &lhs, std::nullptr_t) noexcept {
    return !lhs;
}

template <typename T>
bool operator==(std::nullptr_t, const view_ptr<T> &rhs) noexcept {
    return !rhs;
}

template <typename T>
bool operator!=(const view_ptr<T> &lhs, std::nullptr_t) noexcept {
    return static_cast<bool>(lhs);
}

template <typename T>
bool operator!=(std::nullptr_t, const view_ptr<T> &rhs) noexcept {
    return static_cast<bool>(rhs);
}

// Deduction guide (C++17)
template <typename T>
view_ptr(T *) -> view_ptr<T>;

} // namespace porytiles
