#include <gtest/gtest.h> 


#include <cstdint>
#include <limits> 

#include "latency_tracker.h"


TEST(LatencyTracker, EmptyTrackerReturnsZeros) {
    spsc::LatencyTracker lt(16);


    const auto s = lt.compute(); 
    EXPECT_EQ(s.count, 0u); 
    EXPECT_EQ(s.min_ns, 0u); 
    EXPECT_EQ(s.max_ns, 0u); 
    EXPECT_DOUBLE_EQ(s.mean_ns, 0.0);
    EXPECT_EQ(s.p50_ns, 0u);
    EXPECT_EQ(s.p99_ns, 0u);
    EXPECT_EQ(s.p999_ns, 0u);
}

TEST(LatencyTracker, SingleSampleStats) {
    spsc::LatencyTracker lt(16); 

    lt.record_ns(1234); 

    const auto s = lt.compute();
    EXPECT_EQ(s.count, 1u);
    EXPECT_EQ(s.min_ns, 1234u);
    EXPECT_EQ(s.max_ns, 1234u); 
    EXPECT_DOUBLE_EQ(s.mean_ns, 1234.0);
    EXPECT_EQ(s.p50_ns, 1234u);
    EXPECT_EQ(s.p99_ns, 1234u); 
    EXPECT_EQ(s.p999_ns, 1234u); 
}

TEST(LatencyTracker, BasicStatsMinMaxMean) {
    spsc::LatencyTracker lt(16); 

    lt.record_ns(10);
    lt.record_ns(20);
    lt.record_ns(30);
    lt.record_ns(40);

    const auto s = lt.compute(); 
    EXPECT_EQ(s.count, 4u);
    EXPECT_EQ(s.min_ns, 10u);
    EXPECT_EQ(s.max_ns, 40u); 
    EXPECT_DOUBLE_EQ(s.mean_ns, 25.0);
}

TEST(LatencyTracker, PercentilesUsingCeilIndexRule) {
    // Dataset 1...100 (already deterministic for percentiles)
    spsc::LatencyTracker lt(128);

    for (std::uint64_t i = 1; i <= 100; ++i) {
        lt.record_ns(i);
    }

    const auto s = lt.compute();

    // With index = ceil(p*n) - 1, n = 100
    // p50 => ceil(50) - 1 = 49 => value 50
    // p99 => ceil(99) - 1 = 98 => value 99
    // p999 => ceil(99.9) - 1 = 99 => value 99
    EXPECT_EQ(s.p50_ns, 50u); 
    EXPECT_EQ(s.p99_ns, 99u); 
    EXPECT_EQ(s.p999_ns, 100u); 
}

TEST(LatencyTracker, WrapAroundKeepsMostRecentSamples){
    // Capacity 5, but we record 8 samples: 5 should remain (4, 5, 6, 7, 8)
    spsc::LatencyTracker lt(5); 

    for (std::uint64_t i = 1; i <= 8; ++i){
        lt.record_ns(i);
    }

    const auto s = lt.compute(); 
    EXPECT_EQ(s.count, 5u); 

    // Remaining values should be {4, 5, 6, 7, 8}
    EXPECT_EQ(s.min_ns, 4u);
    EXPECT_EQ(s.max_ns, 8u); 
    EXPECT_DOUBLE_EQ(s.mean_ns, (4.0 + 5.0 + 6.0 + 7.0 + 8.0) / 5.0); 

    // Sorted {4, 5, 6, 7, 8}, n = 5: 
    // p50 => ceil(2.5) - 1 = 2 => 6
    // p99 => ceil(4.95) - 1 = 4 => 8
    // p999 => ceil(4.995) - 1 = 4 => 8
    EXPECT_EQ(s.p50_ns, 6u); 
    EXPECT_EQ(s.p99_ns, 8u); 
    EXPECT_EQ(s.p999_ns, 8u); 
}

TEST(LatencyTracker, ResetClearsState) {
    spsc::LatencyTracker lt(16); 
    lt.record_ns(111); 
    lt.record_ns(222);
    ASSERT_EQ(lt.compute().count, 2u); 

    lt.reset(); 

    const auto s = lt.compute(); 
    EXPECT_EQ(s.count, 0u); 
    EXPECT_EQ(s.min_ns, 0u); 
    EXPECT_EQ(s.max_ns, 0u); 
    EXPECT_DOUBLE_EQ(s.mean_ns, 0.0);
    EXPECT_EQ(s.p50_ns, 0u);
    EXPECT_EQ(s.p99_ns, 0u);
    EXPECT_EQ(s.p999_ns, 0u);
}

TEST(LatencyTracker, ClampsHugeLatencyToUint32Max) {
    spsc::LatencyTracker lt(16);

    const std::uint64_t huge = std::numeric_limits<uint32_t>::max(); 
    lt.record_ns(huge); 

    const auto s = lt.compute(); 
    const auto max_u32 = static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max());

    EXPECT_EQ(s.count, 1u);
    EXPECT_EQ(s.min_ns, max_u32);
    EXPECT_EQ(s.max_ns, max_u32);
    EXPECT_DOUBLE_EQ(s.mean_ns, static_cast<double>(max_u32)); 
    EXPECT_EQ(s.p50_ns, max_u32); 
}