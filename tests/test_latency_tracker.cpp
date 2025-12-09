#include <gtest/gtest.h> 
#include "latency_tracker.h"

TEST(LatencyTrackerBasic, RecordsSamples) {
    LatencyTracker tracker; 
    EXPECT_EQ(tracker.sample_count(), 0u);

    tracker.record_sample(std::chrono::nanoseconds{100});
    tracker.record_sample(std::chrono::nanoseconds{200});

    EXPECT_EQ(tracker.sample_count(), 2u); 
}