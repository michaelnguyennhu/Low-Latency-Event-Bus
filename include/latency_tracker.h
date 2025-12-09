#pragma once 

#include <vector>
#include <chrono> 
#include <cstdint> 
#include <cstddef> 

class LatencyTracker {
public: 
    using clock = std::chrono::steady_clock;

    void record_sample(std::chrono::nanoseconds latency) {
        samples_.push_back(latency.count()); 
    }

    void summarize() const; 

    //For testing / debugging
    std::size_t sample_count() const noexcept {
        return samples_.size(); 
    }

private: 
    std::vector<std::int64_t> samples_;

};