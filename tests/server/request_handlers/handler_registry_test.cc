#include "handler_registry.h"

#include "gtest/gtest.h"

TEST(HandlerRegistryTest, TestMissingFactory) {
    const auto &reg = HandlerRegistry::instance();
    EXPECT_EQ(reg.get_factory("does not exist"), nullptr);
}
