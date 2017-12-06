#ifndef ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_BOUNDED_QUEUE_HPP
#define ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_BOUNDED_QUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <tbb/concurrent_queue.h>

class TBBConcurrentBoundedQueue : public FreeList {
private:
    tbb::concurrent_bounded_queue<uint_fast32_t>    _freelist;

public:
    TBBConcurrentBoundedQueue() {
        _freelist.set_capacity(block_count - 1);
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.push(i);
        }
    };

    // https://software.intel.com/en-us/node/506201
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.try_pop(pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist.size() << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist.size() < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    bool pushSuccessful;
                    if (move) {
                        pushSuccessful = _freelist.try_push(std::move(pageID));
                    } else {
                        pushSuccessful = _freelist.try_push(pageID);
                    }
                    if (!pushSuccessful) {
                        std::cerr << "There isn't enough memory allocated!" << std::endl;
                        exit(1);
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist.size() << std::endl;
                }
            }
        }
    };

};

#endif //ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_BOUNDED_QUEUE_HPP
