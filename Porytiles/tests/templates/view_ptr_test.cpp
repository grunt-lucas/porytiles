#include <gtest/gtest.h>

#include <memory>

#include <porytiles/templates/view_ptr.hpp>

using namespace porytiles;

TEST(ViewPtrTests, BasicViewPtrFunctionality) {
    std::unique_ptr owner{std::make_unique<std::string>("foobar")};

    view_ptr viewer{owner.get()};

    ASSERT_EQ(*viewer, "foobar");
    ASSERT_EQ(*viewer.get(), "foobar");
}
