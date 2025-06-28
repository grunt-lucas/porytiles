#pragma once

#include <memory>
#include <string>

#include "porytiles2/domain/aggregates/PorytilesLayout.hpp"
#include "porytiles2/templates/Result.hpp"

namespace porytiles {

class PorytilesLayoutRepo {
  public:
    virtual ~PorytilesLayoutRepo() = default;

    /**
     * @brief Persists a new or existing PorytilesLayout.
     *
     * @param layout The PorytilesLayout aggregate to save.
     * @return An empty Result on success, otherwise an error description.
     */
    virtual Result<void> Save(const PorytilesLayout &layout) = 0;

    /**
     * @brief Loads an existing PorytilesLayout from storage.
     *
     * @param name The name of the PorytilesLayout aggregate to load.
     * @return A PorytilesLayout Result on success, otherwise an error description.
     */
    virtual Result<std::unique_ptr<PorytilesLayout>> Load(const std::string &name) = 0;
};

} // namespace porytiles
