#ifndef ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_T_HPP
#define ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_T_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include "queues/include/mpmc-bounded-queue.hpp"
#include "mpmc_bounded_queue.h"

class MPMCBoundedQueueT : public FreeList {
private:
    mpmc_bounded_queue_t<uint_fast32_t> _freelist;
    std::atomic<uint_fast32_t>          _approx_freelist_length;

public:
    MPMCBoundedQueueT() : _freelist(nextPowerOfTwo64(block_count - 1)) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.enqueue(i);
            _approx_freelist_length++;
        }
    };

    // https://github.com/mstump/queues
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
    };

};

#endif //ZERO_DETAILS_EVALUATION_MPMC_BOUNDED_QUEUE_T_HPP
