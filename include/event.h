#pragma once 

#include <cstdint> 
#include <type_traits> 

namespace spsc {

    enum class Side : std::uint8_t { Buy = 0, Sell = 1};

    enum class EventType : std::uint8_t { Trade = 0, Quote = 1, Heartbeat = 2}; 

struct Event {
   // For Latency Tracker: producer sets at enqueue, consumer reads at dequeue
   std::uint64_t enqueue_ns{0}; 

   // Infrastructure correctness (FIFO / drop detection)
   std::uint64_t seq{0}; 

   // Minimal payload (fixed size, no allocations)
   std::int64_t price_ticks{0};         // Integer price representation (signed)
   std::uint32_t instrument_id{0};      // Mapped symbol id
   std::uint32_t qty{0}; 
   

   EventType type{EventType::Trade}; 
   Side side{Side::Buy}; 

   // Padding to keep the struct size/alignment predictable
   std::uint16_t _pad{0}; 
}; 

static_assert(std::is_trivially_copyable_v<Event>, "Event should stay trivially copyable (no std::string, no heap).");
static_assert(sizeof(Event) <= 64, "Event should fit in one cache line"); 

}//namespace spsc