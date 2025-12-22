#pragma once 

#include <atomic> 
#include <cstddef> 
#include <cstdint> 
#include <thread> 


#include "event.h"
#include "latency_tracker.h"
#include "ring_buffer.h"

namespace spsc {

class EventBus final {
public: 
    struct Counters {
        std::uint64_t produced{0}; 
        std::uint64_t consumed{0}; 
        std::uint64_t push_fail_spins{0};
        std::uint64_t pop_fail_spins{0}; 
        std::uint64_t seq_mismatch{0}; 
    };

    // ring_capacity: capacity for SPSC ring buffer (rounded up internally by SpscRingBuffer)
    // max_latency_samples: how many latency samples LatencyTracker keeps (ring semantics) 
    explicit EventBus(std::size_t ring_capacity, std::size_t max_latency_samples); 
    
    EventBus(const EventBus&) = delete; 
    EventBus& operator=(const EventBus&) = delete; 

    ~EventBus(); 


    // Start producer/consumer threads. 
    // If target_events > 0, producer will stop after producing exactly that many events.
    void start(std::uint64_t target_events = 0);

    // Request stop (producer stops producing; consumer drains remaining events). 
    void stop() noexcept; 

    // Join threads (safe to call multiple times).
    void join(); 

    // Convenience: stop + join. 
    void stop_and_join() noexcept; 

    // Offline stats (call after join for stable results).
    LatencyTracker::Stats latency_stats() const; 


    Counters counters() const noexcept; 


    bool running() const noexcept { return running_.load(std::memory_order_acquire); } 

private:
   void producer_loop_(std::uint64_t target_events); 
   void consumer_loop_(); 


   // Infrastructure
   SpscRingBuffer<Event> rb_; 
   LatencyTracker latency_; 


   // Threads
   std::thread producer_; 
   std::thread consumer_; 


   // Control
   std::atomic<bool> stop_{false}; 
   std::atomic<bool> running_{false}; 


   // Counters (written by threads, read after join)
   alignas(64) std::uint64_t produced_{0};              // producer thread only
   alignas(64) std::uint64_t push_fail_spins_{0};       // producer thread only

   alignas(64) std::uint64_t consumed_{0};              // consumer thread only
   alignas(64) std::uint64_t pop_fail_spins_{0};        // consumer thread only
   alignas(64) std::uint64_t seq_mismatch_{0};          // consumer thread only

   // Expected sequence (consumer validation)
   std::uint64_t expected_seq_{0}; 
};

}//namespace spsc