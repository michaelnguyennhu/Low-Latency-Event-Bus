#include <gtest/gtest.h> 

#include <cstdint>
#include <thread>

#include "ring_buffer.h"


TEST(SpscRingBuffer, CapacityRoundsUpToPow2) {
    SpscRingBuffer<int> rb(3);
    EXPECT_EQ(rb.capacity(), std::size_t{4}); 

    SpscRingBuffer<int> rb2(8);
    EXPECT_EQ(rb2.capacity(), std::size_t{8}); 

    SpscRingBuffer<int> rb3(1);
    EXPECT_EQ(rb3.capacity(), std::size_t{2}); 
}

TEST(SpscRingBuffer, StartsEmptyNotFull) {
    SpscRingBuffer<int> rb(8); 
    EXPECT_TRUE(rb.empty()); 
    EXPECT_FALSE(rb.full());
}

TEST(SpscRingBuffer, PopOnEmptyReturnsFalse) {
    SpscRingBuffer<int> rb(8);
    int out = 0; 
    EXPECT_FALSE(rb.try_pop(out));
}

TEST(SpscRingBuffer, PushandPopSingleValue) {
    SpscRingBuffer<int> rb(8);

    EXPECT_TRUE(rb.try_push(42));
    EXPECT_FALSE(rb.empty());

    int out = 0; 
    EXPECT_TRUE(rb.try_pop(out)); 
    EXPECT_EQ(out, 42);

    EXPECT_TRUE(rb.empty());
    EXPECT_FALSE(rb.full());
    EXPECT_FALSE(rb.try_pop(out));
}

TEST(SpscRingBuffer, FifoOrder) {
    SpscRingBuffer<int> rb(8); 

    ASSERT_TRUE(rb.try_push(1));
    ASSERT_TRUE(rb.try_push(2));
    ASSERT_TRUE(rb.try_push(3));
    
    int out = 0; 
    ASSERT_TRUE(rb.try_pop(out));
    EXPECT_EQ(out, 1);

    ASSERT_TRUE(rb.try_pop(out));
    EXPECT_EQ(out, 2);

    ASSERT_TRUE(rb.try_pop(out));
    EXPECT_EQ(out, 3);

    EXPECT_TRUE(rb.empty());
}

TEST(SpscRingBuffer, PushingFailsWhenFull) {
    SpscRingBuffer<int> rb(4); 

    EXPECT_TRUE(rb.try_push(10));
    EXPECT_TRUE(rb.try_push(11));
    EXPECT_TRUE(rb.try_push(12));
    EXPECT_TRUE(rb.try_push(13));

    EXPECT_TRUE(rb.full());
    EXPECT_FALSE(rb.try_push(1)); //should fail when full

    int out = 0; 
    EXPECT_TRUE(rb.try_pop(out)); 
    EXPECT_EQ(out, 10); 
    EXPECT_FALSE(rb.full());

    EXPECT_TRUE(rb.try_push(199));
    EXPECT_TRUE(rb.full());
}

TEST(SpscRingBuffer, WrapAroundCorrectnessManyCycles) {
    SpscRingBuffer<std::uint64_t> rb(4);

    std::uint64_t next = 0;

    for (int cycle = 0; cycle < 50000; ++cycle) {
        //push until full
        std::size_t pushed = 0; 
        while (rb.try_push(next)){
            ++next;
            ++pushed;
        }
        ASSERT_GT(pushed, 0u);
        ASSERT_TRUE(rb.full());

        //pop everything we pushed, verifying order
        for (std::size_t i = 0; i < pushed; ++i){
            std::uint64_t out = 0; 
            ASSERT_TRUE(rb.try_pop(out));
            const std::uint64_t want = (next - pushed) + static_cast<std::uint64_t>(i);
            ASSERT_EQ(out, want); 
        }

        ASSERT_TRUE(rb.empty());
    }
}

TEST(SpscRingBuffer, SupportsMoveOnlyType) {
    struct MoveOnly {
        int x; 
        explicit MoveOnly(int v) : x(v) {}

        MoveOnly(const MoveOnly&) = delete; 
        MoveOnly& operator=(const MoveOnly&) = delete; 

        MoveOnly(MoveOnly&&) noexcept = default; 
        MoveOnly& operator=(MoveOnly&&) noexcept = default; 
    }; 

    SpscRingBuffer<MoveOnly> rb(8);

    EXPECT_TRUE(rb.try_push(MoveOnly{7}));

    MoveOnly out{0}; 
    EXPECT_TRUE(rb.try_pop(out));
    EXPECT_EQ(out.x, 7);

    EXPECT_TRUE(rb.empty());
}

TEST(SpscRingBuffer, ThreadedSpscSanity) {
    //Smoke test for real SPSC usage
    SpscRingBuffer<std::uint64_t> rb(1024);

    constexpr std::uint64_t N = 300000; 

    std::thread producer([&] {
        for (std::uint64_t i = 0; i < N;) {
            if (rb.try_push(i)){
                i++; 
            }
        }
    });

    std::thread consumer([&] {
        std::uint64_t expected = 0; 
        std::uint64_t out = 0;
        
        while (expected < N) {
            if (rb.try_pop(out)) {
                ASSERT_EQ(out, expected);
                ++expected; 
            }
        }
    });

    producer.join();
    consumer.join(); 

    EXPECT_TRUE(rb.empty()); 
}