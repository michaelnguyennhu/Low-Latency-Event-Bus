#include "latency_tracker.h"


#include <algorithm>
#include <chrono> 
#include <cmath> 
#include <vector> 


namespace spsc {

LatencyTracker::LatencyTracker(std::size_t max_samples) 
    : capacity_(max_samples), 
      samples_(std::make_unique<std::uint32_t[]>(max_samples)) {
    
    reset(); 
}

void LatencyTracker::reset() noexcept {
    write_idx_ = 0; 
    count_ = 0;
    sum_ns_ = 0; 
}

void LatencyTracker::record_ns(std::uint64_t latency_ns) noexcept {
    // Clamp to uint32_t range (defensive, should not happen in practice)
    const std::uint32_t v = 
        latency_ns > std::numeric_limits<std::uint32_t>::max() 
        ? std::numeric_limits<std::uint32_t>::max() : static_cast<std::uint32_t>(latency_ns);

    if (count_ < capacity_) {
        ++count_; 
    }
    else { 
        sum_ns_ -= samples_[write_idx_]; 
    }

    samples_[write_idx_] = v; 
    sum_ns_ += v;
    write_idx_ = (write_idx_ + 1) % capacity_;
}

LatencyTracker::Stats LatencyTracker::compute() const {
    Stats stats{};
    stats.count = count_;

    if (count_ == 0) {
        return stats;
    } 

    // Copy samples (offline work) 
    std::vector<std::uint32_t> data; 
    data.reserve(count_); 

    for (std::size_t i = 0; i < count_; ++i) {
        data.push_back(samples_[i]);
    }

    std::sort(data.begin(), data.end()); 

    

    stats.min_ns = data.front(); 
    stats.max_ns = data.back(); 
    stats.mean_ns = static_cast<double>(sum_ns_) / static_cast<double>(count_); 

    const std::size_t n = data.size(); 

    stats.p50_ns = data[percentile_index_(0.50, n)];
    stats.p99_ns = data[percentile_index_(0.99, n)]; 
    stats.p999_ns = data[percentile_index_(0.999, n)]; 

    return stats; 
}

std::size_t LatencyTracker::percentile_index_(double p, std::size_t n) noexcept {
    if (n == 0) return 0; 

    std::size_t idx = static_cast<std::size_t>(std::ceil(p * static_cast<double>(n))) - 1; 

    if (idx >= n) idx = n - 1; 
    return idx; 
}

std::uint64_t LatencyTracker::now_ns() noexcept { 
    using clock = std::chrono::steady_clock; 
    return std::chrono::duration_cast<std::chrono::nanoseconds>(
                clock::now().time_since_epoch())
                        .count();
}

} //nampspace spsc