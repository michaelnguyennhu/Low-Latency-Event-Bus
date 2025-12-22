#include <chrono>
#include <cstdint> 
#include <iomanip>
#include <iostream> 


#include "event_bus.h"

int main() {
    
    //Tunables (start conservative; bump for real benchmarking) 
    constexpr std::size_t kRingCapacity = 1 << 16;          // 65,536 events
    constexpr std::size_t kMaxSamples   = 1 << 20;          // 1,048,576 latency samples kept
    constexpr std::uint64_t kNumEvents  = 5'000'000;        // target events to publish 
    constexpr std::uint64_t kWarmupEvents = 300'000;        // warmup (not measured)

    spsc::EventBus bus{kRingCapacity, kMaxSamples}; 


    // Warmup run (ignore stats)
    bus.start(kWarmupEvents); 
    bus.join(); 


    // Measured run 
    const auto t0 = std::chrono::steady_clock::now(); 
    bus.start(kNumEvents); 
    bus.join();
    const auto t1 = std::chrono::steady_clock::now(); 


    const std::chrono::duration<double> elapsed = t1 - t0; 


    const auto stats = bus.latency_stats(); 
    const auto ctrs = bus.counters(); 


    const double secs = elapsed.count(); 
    const double throughput = secs > 0.0 ? (static_cast<double>(ctrs.consumed) / secs) : 0.0; 


    auto ns_to_us = [](std::uint64_t ns) -> double {
        return static_cast<double>(ns) / 1000.0; 
    };


    std::cout << "=== LowLatencyEventBus Benchmark ===\n"; 
    std::cout << "Ring capcity:         " << kRingCapacity << "\n"; 
    std::cout << "Target events:        " << kNumEvents << "\n"; 
    std::cout << "Consumed:             " << ctrs.consumed << "\n"; 
    std::cout << "Elapsed:              " << std::fixed << std::setprecision(6) << secs << "s\n";
    std::cout << "Throughput            " << std::fixed << std::setprecision(0) << throughput << " events/sec\n\n"; 
    
    std::cout << "Latency samples kept: " << stats.count << "\n"; 
    std::cout << "Latency (us):\n"; 
    std::cout << "   min   " << stats.min_ns << "ns (" << std::fixed << std::setprecision(3) << ns_to_us(stats.min_ns) << "us)\n";
    std::cout << "   p50   " << stats.p50_ns << "ns (" << std::fixed << std::setprecision(3) << ns_to_us(stats.p50_ns) << "us)\n";
    std::cout << "   p90   " << stats.p99_ns << "ns (" << std::fixed << std::setprecision(3) << ns_to_us(stats.p99_ns) << "us)\n";
    std::cout << "   p999  " << stats.p999_ns << "ns ("<< std::fixed << std::setprecision(3) << ns_to_us(stats.p999_ns) << "us)\n";
    std::cout << "   max   " << stats.max_ns << "ns ("<< std::fixed << std::setprecision(3) << ns_to_us(stats.max_ns) << "us)\n";
    std::cout << "   mean  " << stats.mean_ns << "ns ("<< std::fixed << std::setprecision(3) << ns_to_us(static_cast<std::uint64_t>(stats.mean_ns)) << "us)\n\n";
    
    std::cout << "Counters:\n";
    std::cout << "   produced:          " << ctrs.produced << "\n"; 
    std::cout << "   push fail spins:   " << ctrs.push_fail_spins << "\n";
    std::cout << "   pop fail spins:    " << ctrs.pop_fail_spins << "\n";
    std::cout << "   seq mismatches:    " << ctrs.seq_mismatch << "\n";

    return 0; 
}