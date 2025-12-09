#pragma once 

#include <cstddef> 


template <typename T> 
class SpscRingBuffer {
public: 
    explicit SpscRingBuffer(std::size_t capacity_power_of_two) : capacity_(capacity_power_of_two) {} 

    bool try_push(const T&) {
        //TODO: lock free SPSC push
        return false; 
    }

    bool try_pop(const T&) {
        //TODO: lock free SPSC pop
        return false; 
    }

    std::size_t capacity() const { return capacity_; }

private: 
    std::size_t capacity_;

};