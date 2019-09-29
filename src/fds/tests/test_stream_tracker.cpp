//
// Created by Kadayam, Hari on Sept 25 2019
//
#include <gtest/gtest.h>
#include <thread>
#include "stream_tracker.hpp"
#include <nlohmann/json.hpp>

using namespace sisl;
THREAD_BUFFER_INIT;

struct TestData {
    TestData(int val) : m_value(val) {}
    int m_value = 0;
};

struct StreamTrackerTest : public testing::Test {
protected:
    StreamTracker< TestData > m_tracker;

public:
    StreamTrackerTest() {}
    size_t get_mem_size() {
        auto json = MetricsFarm::getInstance().get_result_in_json();
        return (size_t)json["StreamTracker"]["StreamTracker"]["Gauges"]["Total Memsize for stream tracker"];
    }
};

TEST_F(StreamTrackerTest, SimpleCompletions) {
    for (auto i = 0; i < 100; ++i) {
        m_tracker.set(i, rand() % 1000);
    }
    EXPECT_EQ(m_tracker.completed_upto(false), -1);
    m_tracker.sweep();
    EXPECT_EQ(m_tracker.completed_upto(false), 99);

    // Do it in reverse
    for (auto i = 150; i >= 100; --i) {
        EXPECT_EQ(m_tracker.completed_upto(true), 99);
        m_tracker.set(i, rand() % 1000);
    }
    EXPECT_EQ(m_tracker.completed_upto(true), 150);

    // Do it in alternate fashion
    auto start_idx = 151;
    auto end_idx = 200;
    bool front = true;
    while (start_idx < end_idx) {
        if (front) {
            m_tracker.set(start_idx++, rand() % 1000);
        } else {
            m_tracker.set(end_idx--, rand() % 1000);
        }
        EXPECT_EQ(m_tracker.completed_upto(true), start_idx - 1);
        front = !front;
    }
    m_tracker.set(start_idx, rand() % 1000);
    EXPECT_EQ(m_tracker.completed_upto(true), 200);
}

TEST_F(StreamTrackerTest, ForceRealloc) {
    auto prev_size = get_mem_size();
    auto far_idx = (int64_t)StreamTracker< TestData >::alloc_blk_size + 1;
    m_tracker.set(far_idx, rand() % 1000);
    EXPECT_EQ(m_tracker.completed_upto(false), -1);
    EXPECT_EQ(get_mem_size(), (prev_size * 2));

    for (int64_t i = 0; i < far_idx; ++i) {
        m_tracker.set(i, rand() % 1000);
    }
    EXPECT_EQ(m_tracker.completed_upto(true), far_idx);
    EXPECT_EQ(get_mem_size(), prev_size);
}

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    auto ret = RUN_ALL_TESTS();
    return ret;
}