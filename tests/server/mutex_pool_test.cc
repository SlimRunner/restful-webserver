#include "mutex_pool.h"

#include "gtest/gtest.h"

using MutexPoolStr = MutexPool<std::string>;
using MutexPtr = std::shared_ptr<std::mutex>;

TEST(MutexPoolTest, TestInsertAndRequest) {
    MutexPoolStr mtx_pool;
    mtx_pool.insert("key1");
    EXPECT_NO_THROW(auto mtx = mtx_pool.request("key1"));
}

TEST(MutexPoolTest, TestInsertAndErase) {
    MutexPoolStr mtx_pool;
    mtx_pool.insert("key1");
    mtx_pool.erase("key1");
    EXPECT_THROW(auto mtx = mtx_pool.request("key1"), expt::mutex_missing_exception);
}

TEST(MutexPoolTest, TestReferenceCounting) {
    MutexPoolStr mtx_pool;
    mtx_pool.insert("key1");  // count 1
    mtx_pool.insert("key1");  // count 2
    mtx_pool.insert("key1");  // count 3
    MutexPtr mtx;
    EXPECT_NO_THROW(mtx = mtx_pool.request("key1"));
    mtx_pool.erase("key1");  // count 2
    EXPECT_NO_THROW(mtx = mtx_pool.request("key1"));
    mtx_pool.erase("key1");  // count 1
    EXPECT_NO_THROW(mtx = mtx_pool.request("key1"));
    mtx_pool.erase("key1");  // count 0
    EXPECT_THROW(mtx = mtx_pool.request("key1"), expt::mutex_missing_exception);
    EXPECT_THROW(mtx_pool.erase("key1");, expt::mutex_missing_exception);
}
