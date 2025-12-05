#pragma once

#include <bitset>
#include <cstddef>
#include <vector>

#include "porytiles2/domain/models/palette.hpp"

namespace porytiles2 {

/**
 * @brief Manages allocation of hardware palette indexes with stack-based checkout semantics.
 *
 * @details
 * PalettePool tracks which hardware palette slots (0-15) are available for use and which have been "checked out" by the
 * packing algorithm. The pool is initialized with a bitset indicating which slots are available (on bits). Indexes can
 * be checked out via check_out() and returned via checkin() in LIFO (stack) order.
 *
 * @invariant checked_out_ is always a subset of available_indexes_
 * @invariant checkout_stack_.size() == checked_out_.count()
 */
class PalettePool {
  public:
    /**
     * @brief Constructs a PalettePool with the specified available indexes.
     *
     * @details
     * The available_indexes bitset determines which hardware palette slots can be used. Only indexes with their
     * corresponding bit set to 1 can be checked out.
     *
     * @param available_indexes Bitset where on-bits indicate available palette slots
     */
    explicit PalettePool(std::bitset<pal::num_pals> available_indexes);

    /**
     * @brief Checks if there is at least one available index that can be checked out.
     *
     * @return true if a subsequent check_out() call will succeed, false otherwise
     */
    [[nodiscard]] bool has_available_index() const
    {
        return (available_indexes_ & ~checked_out_).any();
    }

    /**
     * @brief Checks if a specific index is available for checkout.
     *
     * @param index The hardware palette index to check
     * @pre index must be less than pal::num_pals
     * @return true if the index is in the pool and not currently checked out
     */
    [[nodiscard]] bool is_available(std::size_t index) const;

    /**
     * @brief Checks out the next available hardware palette index.
     *
     * @details
     * Finds the lowest available index that hasn't been checked out, marks it as checked out, and returns it. The index
     * is pushed onto an internal stack to support LIFO checkin behavior.
     *
     * @pre has_available_index() must be true
     * @return The checked-out hardware palette index
     * @post The returned index is marked as checked out
     */
    [[nodiscard]] std::size_t checkout();

    /**
     * @brief Checks out a specific hardware palette index.
     *
     * @details
     * Marks the specified index as checked out and pushes it onto the checkout stack. Use this when a specific palette
     * index is required (e.g., prefilled palettes).
     *
     * @param index The hardware palette index to check out
     * @pre index must be less than pal::num_pals
     * @pre is_available(index) must be true
     * @post The index is marked as checked out
     * @post The index is pushed onto the checkout stack
     */
    void checkout(std::size_t index);

    /**
     * @brief Returns the most recently checked-out index to the pool.
     *
     * @details
     * Pops the most recent index from the internal checkout stack and marks it as no longer checked out. This enables
     * LIFO (stack) semantics for index management.
     *
     * @pre At least one index must have been checked out (checkout_stack_ not empty)
     * @post The popped index is available for checkout again
     */
    void checkin();

  private:
    std::bitset<pal::num_pals> available_indexes_;
    std::bitset<pal::num_pals> checked_out_;
    std::vector<std::size_t> checkout_stack_;
};

} // namespace porytiles2
