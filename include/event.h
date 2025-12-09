#pragma once 

#include <cstdint> 
#include <chrono> 

struct Event {
    std::uint64_t id{}; 
    std::chrono::steady_clock::time_point enqueue_time{}; 
}; 