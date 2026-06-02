#include "porytiles/domain/models/color_set.hpp"

#include <unordered_set>
#include <vector>

#include "gtest/gtest.h"

using namespace porytiles;

TEST(ColorSetBasicTests, DefaultEmpty)
{
    ColorSet set{};
    EXPECT_EQ(color_set_count(set), 0);
}

TEST(ColorSetBasicTests, SetAndTestSingleColor)
{
    ColorSet set{};
    ColorIndex index{5};

    EXPECT_FALSE(set.test(index));
    set.set(index);
    EXPECT_TRUE(set.test(index));
}

TEST(ColorSetBasicTests, SetMultipleColors)
{
    ColorSet set{};
    set.set(ColorIndex{0});
    set.set(ColorIndex{10});
    set.set(ColorIndex{100});

    EXPECT_TRUE(set.test(ColorIndex{0}));
    EXPECT_TRUE(set.test(ColorIndex{10}));
    EXPECT_TRUE(set.test(ColorIndex{100}));
    EXPECT_FALSE(set.test(ColorIndex{1}));
    EXPECT_FALSE(set.test(ColorIndex{50}));
}

TEST(ColorSetBasicTests, SetWithFalse)
{
    ColorSet set{};
    set.set(ColorIndex{5});
    EXPECT_TRUE(set.test(ColorIndex{5}));

    set.set(ColorIndex{5}, false);
    EXPECT_FALSE(set.test(ColorIndex{5}));
}

TEST(ColorSetBasicTests, Reset)
{
    ColorSet set{};
    set.set(ColorIndex{7});
    EXPECT_TRUE(set.test(ColorIndex{7}));

    set.reset(ColorIndex{7});
    EXPECT_FALSE(set.test(ColorIndex{7}));
}

TEST(ColorSetBasicTests, ResetNoOp)
{
    ColorSet set{};
    EXPECT_FALSE(set.test(ColorIndex{3}));
    set.reset(ColorIndex{3});
    EXPECT_FALSE(set.test(ColorIndex{3}));
}

TEST(ColorSetBasicTests, ColorsAccessor)
{
    ColorSet set{};
    set.set(ColorIndex{0});
    set.set(ColorIndex{1});

    const auto &bits = set.colors();
    EXPECT_TRUE(bits.test(0));
    EXPECT_TRUE(bits.test(1));
    EXPECT_FALSE(bits.test(2));
}

TEST(ColorSetEqualityTests, EmptyEquality)
{
    ColorSet a{};
    ColorSet b{};

    EXPECT_EQ(a, b);
}

TEST(ColorSetEqualityTests, SameColorsEqual)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});
    a.set(ColorIndex{10});

    b.set(ColorIndex{1});
    b.set(ColorIndex{5});
    b.set(ColorIndex{10});

    EXPECT_EQ(a, b);
}

TEST(ColorSetEqualityTests, DifferentColorsNotEqual)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    b.set(ColorIndex{2});

    EXPECT_NE(a, b);
}

TEST(ColorSetUnionTests, UnionEmpty)
{
    ColorSet a{};
    ColorSet b{};

    ColorSet result = color_set_union(a, b);

    EXPECT_EQ(color_set_count(result), 0);
}

TEST(ColorSetUnionTests, UnionWithEmpty)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});

    ColorSet result = color_set_union(a, b);

    EXPECT_TRUE(result.test(ColorIndex{1}));
    EXPECT_TRUE(result.test(ColorIndex{5}));
    EXPECT_EQ(color_set_count(result), 2);
}

TEST(ColorSetUnionTests, UnionOfDisjointSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    b.set(ColorIndex{10});
    b.set(ColorIndex{20});

    ColorSet result = color_set_union(a, b);

    EXPECT_TRUE(result.test(ColorIndex{1}));
    EXPECT_TRUE(result.test(ColorIndex{2}));
    EXPECT_TRUE(result.test(ColorIndex{10}));
    EXPECT_TRUE(result.test(ColorIndex{20}));
    EXPECT_EQ(color_set_count(result), 4);
}

TEST(ColorSetUnionTests, UnionOfOverlappingSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});
    a.set(ColorIndex{3});

    b.set(ColorIndex{2});
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    ColorSet result = color_set_union(a, b);

    EXPECT_TRUE(result.test(ColorIndex{1}));
    EXPECT_TRUE(result.test(ColorIndex{2}));
    EXPECT_TRUE(result.test(ColorIndex{3}));
    EXPECT_TRUE(result.test(ColorIndex{4}));
    EXPECT_EQ(color_set_count(result), 4);
}

TEST(ColorSetUnionTests, UnionIsCommutative)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});

    b.set(ColorIndex{5});
    b.set(ColorIndex{10});

    ColorSet result_ab = color_set_union(a, b);
    ColorSet result_ba = color_set_union(b, a);

    EXPECT_EQ(result_ab, result_ba);
}

TEST(ColorSetIntersectionTests, IntersectionEmpty)
{
    ColorSet a{};
    ColorSet b{};

    ColorSet result = color_set_intersection(a, b);

    EXPECT_EQ(color_set_count(result), 0);
}

TEST(ColorSetIntersectionTests, IntersectionWithEmpty)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});

    ColorSet result = color_set_intersection(a, b);

    EXPECT_EQ(color_set_count(result), 0);
}

TEST(ColorSetIntersectionTests, IntersectionDisjoint)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    b.set(ColorIndex{10});
    b.set(ColorIndex{20});

    ColorSet result = color_set_intersection(a, b);

    EXPECT_EQ(color_set_count(result), 0);
}

TEST(ColorSetIntersectionTests, IntersectionOfOverlappingSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});
    a.set(ColorIndex{3});

    b.set(ColorIndex{2});
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    ColorSet result = color_set_intersection(a, b);

    EXPECT_FALSE(result.test(ColorIndex{1}));
    EXPECT_TRUE(result.test(ColorIndex{2}));
    EXPECT_TRUE(result.test(ColorIndex{3}));
    EXPECT_FALSE(result.test(ColorIndex{4}));
    EXPECT_EQ(color_set_count(result), 2);
}

TEST(ColorSetIntersectionTests, IntersectionIsCommutative)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});
    a.set(ColorIndex{10});

    b.set(ColorIndex{5});
    b.set(ColorIndex{10});
    b.set(ColorIndex{15});

    ColorSet result_ab = color_set_intersection(a, b);
    ColorSet result_ba = color_set_intersection(b, a);

    EXPECT_EQ(result_ab, result_ba);
}

TEST(ColorSetIntersectionTests, IntersectionIdentical)
{
    ColorSet a{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});
    a.set(ColorIndex{10});

    ColorSet result = color_set_intersection(a, a);

    EXPECT_EQ(result, a);
}

TEST(ColorSetCountTests, CountEmpty)
{
    ColorSet set{};
    EXPECT_EQ(color_set_count(set), 0);
}

TEST(ColorSetCountTests, CountSingleColor)
{
    ColorSet set{};
    set.set(ColorIndex{42});
    EXPECT_EQ(color_set_count(set), 1);
}

TEST(ColorSetCountTests, CountMultipleColors)
{
    ColorSet set{};
    set.set(ColorIndex{0});
    set.set(ColorIndex{50});
    set.set(ColorIndex{100});
    set.set(ColorIndex{150});
    set.set(ColorIndex{200});

    EXPECT_EQ(color_set_count(set), 5);
}

TEST(ColorSetCountTests, CountAfterReset)
{
    ColorSet set{};
    set.set(ColorIndex{1});
    set.set(ColorIndex{2});
    set.set(ColorIndex{3});
    EXPECT_EQ(color_set_count(set), 3);

    set.reset(ColorIndex{2});
    EXPECT_EQ(color_set_count(set), 2);
}

TEST(ColorSetSubsetTests, EmptySubsetOfAny)
{
    ColorSet empty{};
    ColorSet non_empty{};
    non_empty.set(ColorIndex{1});
    non_empty.set(ColorIndex{5});

    EXPECT_TRUE(is_subset(empty, non_empty));
    EXPECT_TRUE(is_subset(empty, empty));
}

TEST(ColorSetSubsetTests, SubsetOfSelf)
{
    ColorSet set{};
    set.set(ColorIndex{1});
    set.set(ColorIndex{5});
    set.set(ColorIndex{10});

    EXPECT_TRUE(is_subset(set, set));
}

TEST(ColorSetSubsetTests, ProperSubset)
{
    ColorSet subset{};
    ColorSet superset{};

    subset.set(ColorIndex{1});
    subset.set(ColorIndex{5});

    superset.set(ColorIndex{1});
    superset.set(ColorIndex{5});
    superset.set(ColorIndex{10});

    EXPECT_TRUE(is_subset(subset, superset));
    EXPECT_FALSE(is_subset(superset, subset));
}

TEST(ColorSetSubsetTests, DisjointNotSubset)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    b.set(ColorIndex{10});
    b.set(ColorIndex{20});

    EXPECT_FALSE(is_subset(a, b));
    EXPECT_FALSE(is_subset(b, a));
}

TEST(ColorSetSubsetTests, PartialOverlapNotSubset)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});
    a.set(ColorIndex{3});

    b.set(ColorIndex{2});
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    EXPECT_FALSE(is_subset(a, b));
    EXPECT_FALSE(is_subset(b, a));
}

TEST(ColorSetSubsetTests, NonEmptyNotSubsetOfEmpty)
{
    ColorSet non_empty{};
    ColorSet empty{};
    non_empty.set(ColorIndex{1});

    EXPECT_FALSE(is_subset(non_empty, empty));
}

TEST(ColorSetIntersectionSizeTests, IntersectionSizeEmpty)
{
    ColorSet a{};
    ColorSet b{};

    EXPECT_EQ(intersection_size(a, b), 0);
}

TEST(ColorSetIntersectionSizeTests, IntersectionSizeDisjoint)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    b.set(ColorIndex{10});
    b.set(ColorIndex{20});

    EXPECT_EQ(intersection_size(a, b), 0);
}

TEST(ColorSetIntersectionSizeTests, IntersectionSizeOfOverlappingSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});
    a.set(ColorIndex{3});

    b.set(ColorIndex{2});
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    EXPECT_EQ(intersection_size(a, b), 2);
}

TEST(ColorSetIntersectionSizeTests, IntersectionSizeIsCommutative)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});

    b.set(ColorIndex{5});
    b.set(ColorIndex{10});

    EXPECT_EQ(intersection_size(a, b), intersection_size(b, a));
}

TEST(ColorSetUnionSizeTests, UnionSizeEmpty)
{
    ColorSet a{};
    ColorSet b{};

    EXPECT_EQ(union_size(a, b), 0);
}

TEST(ColorSetUnionSizeTests, UnionSizeOfDisjointSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    b.set(ColorIndex{10});
    b.set(ColorIndex{20});

    EXPECT_EQ(union_size(a, b), 4);
}

TEST(ColorSetUnionSizeTests, UnionSizeOfOverlappingSets)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{2});
    a.set(ColorIndex{3});

    b.set(ColorIndex{2});
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    EXPECT_EQ(union_size(a, b), 4);
}

TEST(ColorSetUnionSizeTests, UnionSizeIsCommutative)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});

    b.set(ColorIndex{5});
    b.set(ColorIndex{10});

    EXPECT_EQ(union_size(a, b), union_size(b, a));
}

TEST(ColorSetForEachTests, ForEachEmpty)
{
    ColorSet set{};
    int call_count = 0;

    for_each_color(set, [&call_count](std::size_t) { ++call_count; });

    EXPECT_EQ(call_count, 0);
}

TEST(ColorSetForEachTests, ForEachAllColors)
{
    ColorSet set{};
    set.set(ColorIndex{5});
    set.set(ColorIndex{10});
    set.set(ColorIndex{15});

    std::vector<std::size_t> visited;
    for_each_color(set, [&visited](std::size_t index) { visited.push_back(index); });

    ASSERT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], 5);
    EXPECT_EQ(visited[1], 10);
    EXPECT_EQ(visited[2], 15);
}

TEST(ColorSetForEachTests, ForEachIndexOrder)
{
    ColorSet set{};
    // Insert in non-sorted order
    set.set(ColorIndex{100});
    set.set(ColorIndex{5});
    set.set(ColorIndex{50});

    std::vector<std::size_t> visited;
    for_each_color(set, [&visited](std::size_t index) { visited.push_back(index); });

    ASSERT_EQ(visited.size(), 3);
    EXPECT_EQ(visited[0], 5);
    EXPECT_EQ(visited[1], 50);
    EXPECT_EQ(visited[2], 100);
}

TEST(ColorSetForEachTests, AccumulatorPattern)
{
    ColorSet set{};
    set.set(ColorIndex{1});
    set.set(ColorIndex{2});
    set.set(ColorIndex{3});

    std::size_t sum = 0;
    for_each_color(set, [&sum](std::size_t index) { sum += index; });

    EXPECT_EQ(sum, 6); // 1 + 2 + 3
}

TEST(ColorSetHashTests, HashEquality)
{
    ColorSet a{};
    ColorSet b{};

    a.set(ColorIndex{1});
    a.set(ColorIndex{5});
    a.set(ColorIndex{10});

    b.set(ColorIndex{1});
    b.set(ColorIndex{5});
    b.set(ColorIndex{10});

    std::hash<ColorSet> hasher;
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(ColorSetHashTests, HashInUnorderedSet)
{
    std::unordered_set<ColorSet> set_of_sets;

    ColorSet a{};
    a.set(ColorIndex{1});
    a.set(ColorIndex{2});

    ColorSet b{};
    b.set(ColorIndex{3});
    b.set(ColorIndex{4});

    ColorSet a_copy{};
    a_copy.set(ColorIndex{1});
    a_copy.set(ColorIndex{2});

    set_of_sets.insert(a);
    set_of_sets.insert(b);
    set_of_sets.insert(a_copy); // duplicate of a

    EXPECT_EQ(set_of_sets.size(), 2);
    EXPECT_TRUE(set_of_sets.contains(a));
    EXPECT_TRUE(set_of_sets.contains(b));
}

TEST(ColorSetHashTests, HashEmpty)
{
    ColorSet a{};
    ColorSet b{};

    std::hash<ColorSet> hasher;
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(ColorSetEdgeCaseTests, SetAtBoundaryIndex)
{
    ColorSet set{};

    // Test index 0
    set.set(ColorIndex{0});
    EXPECT_TRUE(set.test(ColorIndex{0}));

    // Test near max index (num_colors - 1)
    // num_colors is pal::max_size * pal::num_pals
    // pal::max_size is typically 16, pal::num_pals is typically 16
    // So num_colors = 256
    set.set(ColorIndex{255});
    EXPECT_TRUE(set.test(ColorIndex{255}));
}

TEST(ColorSetEdgeCaseTests, UnionWithSelf)
{
    ColorSet set{};
    set.set(ColorIndex{1});
    set.set(ColorIndex{5});

    ColorSet result = color_set_union(set, set);

    EXPECT_EQ(result, set);
    EXPECT_EQ(color_set_count(result), 2);
}

TEST(ColorSetEdgeCaseTests, IntersectionWithSelf)
{
    ColorSet set{};
    set.set(ColorIndex{1});
    set.set(ColorIndex{5});

    ColorSet result = color_set_intersection(set, set);

    EXPECT_EQ(result, set);
    EXPECT_EQ(color_set_count(result), 2);
}
