#include "latency_tracker.h"
#include <iostream> 

void LatencyTracker::summarize() const {
    std::cout << "LatencyTracker: " << samples_.size() << " samples collected (summary TBD)" << std::endl;

}