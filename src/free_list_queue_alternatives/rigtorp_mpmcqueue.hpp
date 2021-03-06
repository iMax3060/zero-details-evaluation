#ifndef ZERO_DETAILS_EVALUATION_RIGTORP_MPMCQUEUE_HPP
#define ZERO_DETAILS_EVALUATION_RIGTORP_MPMCQUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include "MPMCQueue/MPMCQueue.h"

class RigtorpMPMCQueue : public FreeList {
private:
    rigtorp::MPMCQueue<uint_fast32_t>   _freelist;
    std::atomic<uint_fast32_t>          _approx_freelist_length;

public:
    RigtorpMPMCQueue() : _freelist(block_count - 1) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.push(i);
            _approx_freelist_length++;
        }
    };

    // https://github.com/rigtorp/MPMCQueue
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.try_pop(pageID);
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
                        _freelist.push(pageID);
                        _approx_freelist_length++;
                    } else {
                        _freelist.push(pageID);
                        _approx_freelist_length++;
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                }
            }
        }
    };

};

#endif //ZERO_DETAILS_EVALUATION_RIGTORP_MPMCQUEUE_HPP
