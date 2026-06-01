#include "porytiles2/domain/orchestration/operand_bundle.hpp"

#include <algorithm>
#include <vector>

namespace porytiles2 {

// -- Range-for support --
OperandBundle::iterator OperandBundle::begin() noexcept
{
    return config_.begin();
}

OperandBundle::iterator OperandBundle::end() noexcept
{
    return config_.end();
}

OperandBundle::const_iterator OperandBundle::begin() const noexcept
{
    return config_.begin();
}

OperandBundle::const_iterator OperandBundle::end() const noexcept
{
    return config_.end();
}

OperandBundle::const_iterator OperandBundle::cbegin() const noexcept
{
    return config_.cbegin();
}

OperandBundle::const_iterator OperandBundle::cend() const noexcept
{
    return config_.cend();
}
// -- Range-for support --

std::optional<std::any> OperandBundle::get(const std::string &key) const
{
    if (!contains(key)) {
        return std::nullopt;
    }
    return std::optional{config_.at(key)};
}

std::size_t OperandBundle::size() const
{
    return config_.size();
}

void OperandBundle::put(const std::string &key, const std::any &value)
{
    config_.insert_or_assign(key, value);
}

bool OperandBundle::contains(const std::string &key) const
{
    return config_.contains(key);
}

std::optional<std::type_index> OperandBundle::type_index_of(const std::string &key) const
{
    if (!contains(key)) {
        return std::nullopt;
    }
    return config_.at(key).type();
}

bool OperandBundle::satisfies_declarations(const std::vector<OperandDeclaration> &declarations) const
{
    return std::ranges::all_of(declarations, [this](const auto &decl) {
        return contains(decl.key()) && decl.expected_type() == type_index_of(decl.key());
    });
}

} // namespace porytiles2