#pragma once

namespace porytiles2 {

class IndexPixel {
  public:
    IndexPixel() : index_{0} {}

    IndexPixel(unsigned int index) : index_{index} {}

    [[nodiscard]] unsigned int index() const {
        return index_;
    }

    [[nodiscard]] bool is_transparent(const IndexPixel &unused) const {
        return index_ == 0;
    }

  private:
    unsigned int index_;
};

} // namespace porytiles2