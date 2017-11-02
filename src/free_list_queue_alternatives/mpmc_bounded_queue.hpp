#ifndef ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_HPP
#define ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_HPP

#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include "mpmc_queue.h"

namespace _mpmc_bounded_queue {

    mpmc_bounded_queue<uint_fast32_t> _freelist(nextPowerOfTwo64(block_count - 1));
    std::atomic<uint_fast32_t> _freelist_size;

    // http://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.dequeue(pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist_size << std::endl;
            _freelist_size--;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist_size < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    if (move) {
                        _freelist.enqueue(std::move(pageID));
                    } else {
                        _freelist.enqueue(pageID);
                    }
                    _freelist_size++;
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist_size << std::endl;
                }
            }
        }
    }

    void init(const uint_fast32_t& block_count) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.enqueue(i);
            _freelist_size++;
        }
    }

}

#endif //ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_HPP
