#include "event_bus.h"


EventBus::EventBus(std::size_t capacity_power_of_two) 
    : buffer_( capacity_power_of_two ) 
{}

EventBus::~EventBus() {
    //later: shutdown and join threads as needed
}

void EventBus::start(std::size_t /*num_events*/) {
    //TODO: implement producer/consumer startup 
}

void EventBus::join() {
    //TODO: Join threads when implemented
}

void EventBus::producer_thread_func(std::size_t /*num_events*/) {
    //TODO
}

void EventBus::consumer_thread_func(std::size_t /*num_events*/) {
    //TODO
}
