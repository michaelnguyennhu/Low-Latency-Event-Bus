#include <gtest/gtest.h> 
#include "ring_buffer.h"
#include "event.h"

TEST(SpscRingBufferBasic, ConstructsWithCapacity) {
    SpscRingBuffer<Event> buf(1024); 
    EXPECT_EQ(buf.capacity(), 1024u); 
}