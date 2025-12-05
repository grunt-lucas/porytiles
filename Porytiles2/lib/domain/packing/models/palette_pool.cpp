#include "porytiles2/domain/packing/models/palette_pool.hpp"

#include "porytiles2/utilities/panic/panic.hpp"

namespace porytiles2 {

PalettePool::PalettePool(std::bitset<pal::num_pals> available_indexes)
    : available_indexes_{available_indexes}, checked_out_{}, checkout_stack_{}
{
}

bool PalettePool::is_available(std::size_t index) const
{
    if (index >= pal::num_pals) {
        panic("index out of bounds");
    }
    return available_indexes_.test(index) && !checked_out_.test(index);
}

std::size_t PalettePool::checkout()
{
    if (!has_available_index()) {
        panic("called with no available indexes");
    }

    // Find the lowest available index that is not checked out
    for (std::size_t i = 0; i < pal::num_pals; ++i) {
        if (available_indexes_.test(i) && !checked_out_.test(i)) {
            checked_out_.set(i);
            checkout_stack_.push_back(i);
            return i;
        }
    }

    // Should be unreachable due to has_available_index() check above
    panic("unreachable state");
}

void PalettePool::checkout(std::size_t index)
{
    if (!is_available(index)) {
        panic("index is not available for checkout");
    }

    checked_out_.set(index);
    checkout_stack_.push_back(index);
}

void PalettePool::checkin()
{
    if (checkout_stack_.empty()) {
        panic("called with empty checkout stack");
    }

    std::size_t index = checkout_stack_.back();
    checkout_stack_.pop_back();
    checked_out_.reset(index);
}

} // namespace porytiles2
