#include "gtest/gtest.h"

#include <any>
#include <string>
#include <typeindex>
#include <vector>

#include "porytiles2/infra/orchestration/artifact_bundle.hpp"
#include "porytiles2/infra/orchestration/artifact_declaration.hpp"

using namespace porytiles2;

TEST(ArtifactBundleTests, BasicPutAndGetShouldWork) {
    ArtifactBundle bundle{};

    bundle.put("test_key", 42);

    const auto result = bundle.get("test_key");
    ASSERT_TRUE(result.has_value());

    const auto value = std::any_cast<int>(result.value());
    EXPECT_EQ(42, value);
}

TEST(ArtifactBundleTests, GetNonExistentKeyShouldReturnNullopt) {
    ArtifactBundle bundle{};

    const auto result = bundle.get("non_existent");
    EXPECT_FALSE(result.has_value());
}

TEST(ArtifactBundleTests, GetUnwrappedShouldWork) {
    ArtifactBundle bundle{};

    bundle.put("string_key", std::string{"hello"});
    bundle.put("int_key", 123);

    const auto string_result = bundle.get_unwrapped<std::string>("string_key");
    ASSERT_TRUE(string_result.has_value());
    EXPECT_EQ("hello", string_result.value());

    const auto int_result = bundle.get_unwrapped<int>("int_key");
    ASSERT_TRUE(int_result.has_value());
    EXPECT_EQ(123, int_result.value());
}

TEST(ArtifactBundleTests, GetUnwrappedNonExistentKeyShouldReturnNullopt) {
    ArtifactBundle bundle{};

    const auto result = bundle.get_unwrapped<int>("non_existent");
    EXPECT_FALSE(result.has_value());
}

TEST(ArtifactBundleTests, GetUnwrappedWrongTypeShouldPanic) {
    ArtifactBundle bundle{};

    bundle.put("string_key", std::string{"hello"});

    EXPECT_DEATH(std::ignore = bundle.get_unwrapped<int>("string_key"), "invalid type requested for key: string_key");
}

TEST(ArtifactBundleTests, ContainsShouldWork) {
    ArtifactBundle bundle{};

    EXPECT_FALSE(bundle.contains("test_key"));

    bundle.put("test_key", 42);

    EXPECT_TRUE(bundle.contains("test_key"));
}

TEST(ArtifactBundleTests, TypeIndexOfShouldWork) {
    ArtifactBundle bundle{};

    bundle.put("int_key", 42);
    bundle.put("string_key", std::string{"hello"});

    const auto int_type = bundle.type_index_of("int_key");
    ASSERT_TRUE(int_type.has_value());
    EXPECT_EQ(typeid(int), int_type.value());

    const auto string_type = bundle.type_index_of("string_key");
    ASSERT_TRUE(string_type.has_value());
    EXPECT_EQ(typeid(std::string), string_type.value());
}

TEST(ArtifactBundleTests, TypeIndexOfNonExistentKeyShouldReturnNullopt) {
    ArtifactBundle bundle{};

    const auto result = bundle.type_index_of("non_existent");
    EXPECT_FALSE(result.has_value());
}

TEST(ArtifactBundleTests, SatisfiesDeclarationsWithMatchingBundleShouldReturnTrue) {
    ArtifactBundle bundle{};
    bundle.put("num1", 42);
    bundle.put("text", std::string{"hello"});

    std::vector<ArtifactDeclaration> declarations = {ArtifactDeclaration{"num1", typeid(int)},
                                                     ArtifactDeclaration{"text", typeid(std::string)}};

    EXPECT_TRUE(bundle.satisfies_declarations(declarations));
}

TEST(ArtifactBundleTests, SatisfiesDeclarationsWithMissingKeyShouldReturnFalse) {
    ArtifactBundle bundle{};
    bundle.put("num1", 42);

    std::vector<ArtifactDeclaration> declarations = {ArtifactDeclaration{"num1", typeid(int)},
                                                     ArtifactDeclaration{"missing_key", typeid(std::string)}};

    EXPECT_FALSE(bundle.satisfies_declarations(declarations));
}

TEST(ArtifactBundleTests, SatisfiesDeclarationsWithWrongTypeShouldReturnFalse) {
    ArtifactBundle bundle{};
    bundle.put("num1", 42);
    bundle.put("text", std::string{"hello"});

    std::vector<ArtifactDeclaration> declarations = {
        ArtifactDeclaration{"num1", typeid(int)}, ArtifactDeclaration{"text", typeid(int)} // Wrong type
    };

    EXPECT_FALSE(bundle.satisfies_declarations(declarations));
}

TEST(ArtifactBundleTests, EmptyDeclarationsShouldReturnTrue) {
    ArtifactBundle bundle{};

    std::vector<ArtifactDeclaration> declarations{};

    EXPECT_TRUE(bundle.satisfies_declarations(declarations));
}

TEST(ArtifactBundleTests, IteratorSupportShouldWork) {
    ArtifactBundle bundle{};
    bundle.put("key1", 1);
    bundle.put("key2", 2);
    bundle.put("key3", 3);

    int count = 0;
    for (const auto &[key, value] : bundle) {
        count++;
        EXPECT_TRUE(bundle.contains(key));
    }

    EXPECT_EQ(3, count);
}

TEST(ArtifactBundleTests, ConstIteratorShouldWork) {
    ArtifactBundle bundle{};
    bundle.put("key1", 1);
    bundle.put("key2", 2);

    const ArtifactBundle &const_bundle = bundle;

    int count = 0;
    for (auto it = const_bundle.cbegin(); it != const_bundle.cend(); ++it) {
        count++;
    }

    EXPECT_EQ(2, count);
}

TEST(ArtifactBundleTests, PutShouldOverwriteExistingValue) {
    ArtifactBundle bundle{};

    bundle.put("key", 42);
    EXPECT_EQ(42, bundle.get_unwrapped<int>("key").value());

    bundle.put("key", 100);
    EXPECT_EQ(100, bundle.get_unwrapped<int>("key").value());
}