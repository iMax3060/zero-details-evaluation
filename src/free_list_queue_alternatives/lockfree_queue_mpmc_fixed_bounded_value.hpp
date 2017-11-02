#ifndef ZERO_DETAILS_EVALUATION_LOCKFREE_QUEUE_MPMC_FIXED_BOUNDED_VALUE_HPP
#define ZERO_DETAILS_EVALUATION_LOCKFREE_QUEUE_MPMC_FIXED_BOUNDED_VALUE_HPP

#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include "mpmc_bounded_queue.h"

namespace _lockfree_queue_mpmc_fixed_bounded_value {

    lockfree_queue::mpmc_fixed_bounded_value<uint_fast32_t, nextPowerOfTwo64(block_count - 1)> _freelist;
    std::atomic<uint_fast32_t> _approx_freelist_length;

    // https://gist.github.com/uecasm/b547db812ae4bba39bb1bd0443801507
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.dequeue(pageID);
        if (popSuccessful) {
            _approx_freelist_length--;
            if (debug) std::cout << _approx_freelist_length << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_approx_freelist_length < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    if (move) {
                        _freelist.enqueue(pageID);
                        _approx_freelist_length++;
                    } else {
                        _freelist.enqueue(pageID);
                        _approx_freelist_length++;
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                }
            }
        }
    }

    void init(const uint_fast32_t& block_count) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.enqueue(i);
            _approx_freelist_length++;
        }
    }

}

#endif //ZERO_DETAILS_EVALUATION_LOCKFREE_QUEUE_MPMC_FIXED_BOUNDED_VALUE_HPP
