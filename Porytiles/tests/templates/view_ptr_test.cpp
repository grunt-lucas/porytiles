#include <gtest/gtest.h>

#include <memory>

#include <porytiles/templates/view_ptr.hpp>

using namespace porytiles;

TEST(ViewPtrTests, ViewPtrReadOperations) {
    std::unique_ptr owner{std::make_unique<std::string>("foobar")};

    view_ptr viewer{owner.get()};

    ASSERT_EQ("foobar", *viewer);
    ASSERT_EQ("foobar", *viewer.get());
    ASSERT_EQ(6, viewer->size());
}

TEST(ViewPtrTests, ViewPtrWriteOperations) {
    std::unique_ptr owner{std::make_unique<std::string>("foobar")};

    view_ptr viewer{owner.get()};

    // Mutate the string through the view
    viewer.get()->at(0) = 'g';

    // String should look different from view
    ASSERT_EQ("goobar", *viewer);
    ASSERT_EQ("goobar", *viewer.get());
    ASSERT_EQ(6, viewer->size());

    // String should look different from owner
    ASSERT_EQ("goobar", *owner);
    ASSERT_EQ("goobar", *owner.get());
    ASSERT_EQ(6, owner->size());

    // Mutate the string through the owner
    viewer.get()->at(5) = 't';

    // String should look different from view
    ASSERT_EQ("goobat", *viewer);
    ASSERT_EQ("goobat", *viewer.get());
    ASSERT_EQ(6, viewer->size());

    // String should look different from owner
    ASSERT_EQ("goobat", *owner);
    ASSERT_EQ("goobat", *owner.get());
    ASSERT_EQ(6, owner->size());
}
