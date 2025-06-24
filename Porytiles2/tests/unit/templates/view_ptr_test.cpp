#include "gtest/gtest.h"

#include <memory>

#include "porytiles2/templates/view_ptr.hpp"

using namespace porytiles;

TEST(ViewPtrTests, ReadOperations) {
    const auto owner{std::make_unique<std::string>("foobar")};

    const ViewPtr viewer{owner.get()};

    EXPECT_EQ("foobar", *viewer);
    EXPECT_EQ("foobar", *viewer.get());
    EXPECT_EQ(6, viewer->size());
}

TEST(ViewPtrTests, WriteOperations) {
    const auto owner{std::make_unique<std::string>("foobar")};

    const ViewPtr viewer{owner.get()};

    // Mutate the string through the view
    viewer->at(0) = 'g';

    // String should look different from view
    EXPECT_EQ("goobar", *viewer);
    EXPECT_EQ("goobar", *viewer.get());
    EXPECT_EQ(6, viewer->size());

    // String should look different from owner
    EXPECT_EQ("goobar", *owner);
    EXPECT_EQ("goobar", *owner.get());
    EXPECT_EQ(6, owner->size());

    // Mutate the string through the owner
    viewer->at(5) = 't';

    // String should look different from view
    EXPECT_EQ("goobat", *viewer);
    EXPECT_EQ("goobat", *viewer.get());
    EXPECT_EQ(6, viewer->size());

    // String should look different from owner
    EXPECT_EQ("goobat", *owner);
    EXPECT_EQ("goobat", *owner.get());
    EXPECT_EQ(6, owner->size());
}
