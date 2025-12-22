#include "event_bus.h"


#include <utility> // for std::move


namespace spsc {

EventBus::EventBus(std::size_t ring_capacity, 
                    std::size_t max_latency_samples) 
    : rb_(ring_capacity),
      latency_(max_latency_samples) {}
      

EventBus::~EventBus() {
    stop_and_join(); 
}


void EventBus::start(std::uint64_t target_events) {
    // If already running, do nothing
    if (running_.load(std::memory_order_acquire)) {
        return; 
    }

    // Reset state
    stop_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release); 

    produced_ = 0; 
    consumed_ = 0; 
    push_fail_spins_ = 0; 
    pop_fail_spins_ = 0; 
    seq_mismatch_ = 0; 
    expected_seq_ = 0; 

    latency_.reset(); 

    // Launch threads
    producer_ = std::thread([this, target_events] { producer_loop_(target_events); });
    consumer_ = std::thread( [this] { consumer_loop_(); }); 
}

void EventBus::stop() noexcept {
    stop_.store(true, std::memory_order_release); 
}

void EventBus::join() {
    if (producer_.joinable()) producer_.join(); 
    if (consumer_.joinable()) consumer_.join(); 


    running_.store(false, std::memory_order_release); 
}

void EventBus::stop_and_join() noexcept {
    stop(); 
    join(); 
}

LatencyTracker::Stats EventBus::latency_stats() const {
    return latency_.compute(); 
}

EventBus::Counters EventBus::counters() const noexcept {
    Counters c{}; 
    c.produced = produced_; 
    c.consumed = consumed_; 
    c.push_fail_spins = push_fail_spins_; 
    c.pop_fail_spins = pop_fail_spins_; 
    c.seq_mismatch = seq_mismatch_; 
    return c; 
}


void EventBus::producer_loop_(std::uint64_t target_events) {
    std::uint64_t seq = 0; 

    while (!stop_.load(std::memory_order_acquire)) {
        if (target_events != 0 && seq >= target_events) {
            // Produced the requested number of events; request stop
            stop_.store(true, std::memory_order_release); 
            break; 
        }

        Event e{}; 
        e.enqueue_ns = LatencyTracker::now_ns(); 
        e.seq = seq; 

        // Minimal synthetic payload (can be replaced with real market event gen later) 
        e.instrument_id = static_cast<std::uint32_t>(seq & 0xFFFF); 
        e.qty = 100u + static_cast<std::uint32_t>(seq & 0x3F);
        e.price_ticks = 100'000 + static_cast<std::int64_t>(seq % 1'000); 
        e.type = EventType::Trade; 
        e.side = (seq & 1) ? Side::Buy : Side::Sell; 

        if (rb_.try_push(std::move(e))) {
            ++seq; 
            ++produced_; 
        }
        else {
            ++push_fail_spins_; 
            // In ultra-low-latency code, may add a pause/yield hint here
        }
    }
}

void EventBus::consumer_loop_() {
    expected_seq_ = 0; 

    while (!stop_.load(std::memory_order_acquire) || !rb_.empty()) {
        Event e{}; 
        if (rb_.try_pop(e)) {
            const std::uint64_t now = LatencyTracker::now_ns(); 
            const std::uint64_t lat = now - e.enqueue_ns; 
            
            latency_.record_ns(lat); 
            ++consumed_; 

            //Optional correctness: check FIFO end-to-end
            if (e.seq != expected_seq_) { 
                ++seq_mismatch_; 
                expected_seq_ = e.seq + 1; //resync
            }
            else {
                ++expected_seq_; 
            }
        }
        else {
            ++pop_fail_spins_; 
            //in ultra-low-latency code, may add a pause/yield hint here. 
        }
    }
}



}//namespace spsc