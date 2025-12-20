#pragma once 

#include <atomic> 
#include <cstddef> 
#include <cstdint> 
#include <memory> 
#include <new>
#include <type_traits> 
#include <utility>

namespace spsc{
    inline constexpr std::size_t kCacheLine = 64; 

    inline constexpr bool is_power_of_two(std::size_t x) noexcept {
        //x != 0 && x contains only one "1" bit
        return x && ((x & (x - 1)) == 0);
    }

    inline std::size_t round_up_pow2(std::size_t x) {
        if (x < 2) return 2;
        if (is_power_of_two(x)) return x; 
        
        std::size_t p = 1; 
        while (p < x) p <<= 1; //move bits over until it is greater than x
        return p; 
    }

    template <typename T> 
    using Storage = std::aligned_storage_t<sizeof(T), alignof(T)>; 

}//nampspace spsc


template <typename T> 
class SpscRingBuffer final {
public: 
    explicit SpscRingBuffer(std::size_t requested_capacity) 
        : capacity_(spsc::round_up_pow2(requested_capacity)),
          mask_(capacity_ - 1),
          storage_(std::make_unique<spsc::Storage<T>[]>(capacity_)) {
        head_.store(0, std::memory_order_relaxed);
        tail_.store(0, std::memory_order_relaxed); 
    } 

    ~SpscRingBuffer(){ drain_and_destroy_(); }

    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete; 

    std::size_t capacity() const noexcept { return capacity_; }

    bool try_push(const T& value) requires std::copy_constructible<T> { return emplace_(value); }
    bool try_push(T&& value) {return emplace_(std::move(value)); } 
    
    bool try_pop(T& out) {
        const auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_acquire); 

        if (head == tail) return false; // Empty

        T* slot = slot_ptr_(tail); 
        out = std::move(*slot); 
        slot->~T(); 

        tail_.store(tail + 1, std::memory_order_release); 
        return true; 
    }

    
    bool empty() const noexcept {
        const auto tail = tail_.load(std::memory_order_acquire); 
        const auto head = head_.load(std::memory_order_acquire); 
        return head == tail; 
    }

    bool full() const noexcept {
        const auto tail = tail_.load(std::memory_order_acquire);
        const auto head = head_.load(std::memory_order_acquire); 
        return (head - tail) == capacity_;
    }
    

private: 
    template <typename U>
    bool emplace_(U&& value) {
        const auto head = head_.load(std::memory_order_relaxed);
        const auto tail = tail_.load(std::memory_order_acquire);

        if ((head - tail) == capacity_) return false; // full 

        T* slot = slot_ptr_(head); 
        ::new (static_cast<void*>(slot)) T(std::forward<U>(value));

        head_.store(head + 1, std::memory_order_release); 
        return true; 
    } 

    T* slot_ptr_(std::uint64_t idx) noexcept {
        return std::launder(
            reinterpret_cast<T*>(&storage_[static_cast<std::size_t>(idx & mask_)])
        );
    }

    void drain_and_destroy_() noexcept {
        auto tail = tail_.load(std::memory_order_relaxed);
        const auto head = head_.load(std::memory_order_relaxed); 

        while (tail != head) {
            T* slot = slot_ptr_(tail); 
            slot->~T();
            ++tail; 
        }

        tail_.store(head, std::memory_order_relaxed); 
    }

    const std::size_t capacity_;
    const std::size_t mask_;
    std::unique_ptr<spsc::Storage<T>[]> storage_;

    // Producer-owned index (Mostly written by producer, read by consumer)
    alignas(spsc::kCacheLine) std::atomic<std::uint64_t> head_{0}; 
    char pad0_[spsc::kCacheLine - sizeof(std::atomic<std::uint64_t>)]{}; 

    // Consumer-owned index (Mostly written by consumer, read by producer)
    alignas(spsc::kCacheLine) std::atomic<std::uint64_t> tail_{0}; 
    char pad1_[spsc::kCacheLine - sizeof(std::atomic<std::uint64_t>)]{};
};