#ifndef ZERO_DETAILS_EVALUATION_LEGACY_ZERO_STACK_HPP
#define ZERO_DETAILS_EVALUATION_LEGACY_ZERO_STACK_HPP

#include "config.hpp"
#include "helper_functions.hpp"

#include "tatas.h"

namespace _legacy_zero_stack {

    uint_fast32_t* _freelist;
    uint_fast32_t _approx_freelist_length;
    tatas_lock _freelist_lock;

    // https://github.com/iMax3060/zero/commit/f4f594b744687004f774690e0413663baf8502b0
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        while (true) {
            if (_approx_freelist_length > 0) {
                _freelist_lock.acquire();
                if (_approx_freelist_length > 0) {
                    pageID = _freelist[0];

                    --_approx_freelist_length;
                    if (_approx_freelist_length == 0) {
                        _freelist[0] = 0;
                    } else {
                        _freelist[0] = _freelist[pageID];
                    }
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                    pageUnused[pageID].clear();
                    __asm__ __volatile__(""::"m" (pageID));
                    _freelist_lock.release();
                    break;
                }
                _freelist_lock.release();
                std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            }

            while (_approx_freelist_length < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    _freelist_lock.acquire();
                    ++_approx_freelist_length;
                    _freelist[pageID] = _freelist[0];
                    _freelist[0] = pageID;
                    _freelist_lock.release();
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                }
            }
        }
    }

    void init(const uint_fast32_t& block_count) {
        _freelist = new uint_fast32_t[block_count];
        _freelist[0] = 1;
        for (uint_fast32_t i = 1; i < block_count - 1; i++) {
            _freelist[i] = i + 1;
        }
        _freelist[block_count - 1] = 0;
        _approx_freelist_length = block_count - 1;
    }

    void destroy() {
        delete[] _freelist;
    }

}

#endif //ZERO_DETAILS_EVALUATION_LEGACY_ZERO_STACK_HPP
