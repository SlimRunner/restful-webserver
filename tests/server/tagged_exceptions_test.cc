#include "tagged_exceptions.h"

#include <string>

#include "gtest/gtest.h"

TEST(TaggedExceptionTest, LoadsMessageFromChar) {
    expt::base_tagged_exception except("error message");
    ASSERT_EQ(std::string(except.what()), "error message");
}

TEST(TaggedExceptionTest, LoadsMessageFromString) {
    std::string msg = "error message";
    expt::base_tagged_exception except(msg);
    ASSERT_EQ(except.what(), msg);
}
