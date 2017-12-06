#ifndef ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_QUEUE_HPP
#define ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_QUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <tbb/concurrent_queue.h>

class TBBConcurrentQueue : public FreeList {
private:
    tbb::concurrent_queue<uint_fast32_t>    _freelist;

public:
    TBBConcurrentQueue() {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.push(i);
        }
    };

    // https://software.intel.com/en-us/node/506200
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.try_pop(pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist.unsafe_size() << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist.unsafe_size() < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    if (move) {
                        _freelist.push(std::move(pageID));
                    } else {
                        _freelist.push(pageID);
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist.unsafe_size() << std::endl;
                }
            }
        }
    };

};

#endif //ZERO_DETAILS_EVALUATION_TBB_CONCURRENT_QUEUE_HPP
