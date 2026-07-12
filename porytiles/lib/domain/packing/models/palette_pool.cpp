#include "porytiles/domain/packing/models/palette_pool.hpp"

#include "porytiles/utilities/panic/panic.hpp"

namespace porytiles {

PalettePool::PalettePool(std::bitset<palette::num_palettes> available_indexes)
    : available_indexes_{available_indexes}, checked_out_{}, checkout_stack_{}
{
}

bool PalettePool::is_available(std::size_t hardware_index) const
{
    if (hardware_index >= palette::num_palettes) {
        panic("index out of bounds");
    }
    return available_indexes_.test(hardware_index) && !checked_out_.test(hardware_index);
}

std::size_t PalettePool::checkout()
{
    if (!has_available_palette()) {
        panic("called with no available indexes");
    }

    // Find the lowest available index that is not checked out
    for (std::size_t i = 0; i < palette::num_palettes; ++i) {
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

} // namespace porytiles
