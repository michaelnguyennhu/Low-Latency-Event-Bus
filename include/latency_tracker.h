#pragma once 

#include <cstddef> 
#include <cstdint> 
#include <memory> 

namespace spsc {

class LatencyTracker {
public: 
    struct Stats {
        std::uint64_t count{0}; 

        std::uint64_t min_ns{0}; 
        std::uint64_t max_ns{0}; 
        double mean_ns{0.0}; 
        
        std::uint64_t p50_ns{0}; 
        std::uint64_t p99_ns{0}; 
        std::uint64_t p999_ns{0}; 
    };

    // Allocate storage once. No allocations on record_ns hot path
    explicit LatencyTracker(std::size_t max_samples); 

    LatencyTracker(const LatencyTracker&) = delete;
    LatencyTracker& operator=(const LatencyTracker&) = delete; 

    // Hot Path: called once per dequeued message (assumed single threaded)
    void record_ns(std::uint64_t latency_ns) noexcept; 

    // Offline: computes percentiles + summary stats over stored samples. 
    Stats compute() const; 

    // Reset counters/samples (does not free/reallocate storage)
    void reset() noexcept; 

    std::size_t capacity() const noexcept { return capacity_; }
    std::size_t count() const noexcept { return count_; }

    // Monotonic timestamp in nanoseconds 
    static std::uint64_t now_ns() noexcept; 

private: 
    static std::size_t percentile_index_(double p, std::size_t n) noexcept; 

    const std::size_t capacity_; 
    std::unique_ptr<std::uint32_t[]> samples_; 

    std::size_t write_idx_{0};   // next write position
    std::size_t count_{0};      // number of valid samples (<= capacity_)
    std::uint64_t sum_ns_{0};
};

}//nampspace spsc