# Low-Latency Event Bus (C++20 SPSC Ring Buffer + Latency Tracker)

A C++20, lock-free single-producer/single-consumer (SPSC) event pipeline
targeting 1M+ messages per second with microsecond-level latency.

Components:
- Cache-aware SPSC ring buffer using `std::atomic`
- Producer/consumer threads simulating a market-data event bus
- Latency tracker with p50, p99, p99.9 metrics
- CMake-based benchmark harness
- GoogleTest unit tests for core components
