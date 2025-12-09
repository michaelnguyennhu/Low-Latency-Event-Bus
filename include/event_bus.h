#pragma once 

#include "event.h"
#include "ring_buffer.h"
#include "latency_tracker.h"

#include <atomic>
#include <thread>
#include <cstddef>


class EventBus {
public: 
    explicit EventBus(std::size_t capacity_power_of_two); 
    ~EventBus(); 

    void start(std::size_t num_events); 
    void join(); 

private:
    void producer_thread_func(std::size_t num_events);
    void consumer_thread_func(std::size_t num_events); 

    SpscRingBuffer<Event> buffer_; 
    LatencyTracker latency_tracker_; 

    std::thread producer_thread_; 
    std::thread consumer_thread_; 

    std::atomic<bool> running_{false}; 

};