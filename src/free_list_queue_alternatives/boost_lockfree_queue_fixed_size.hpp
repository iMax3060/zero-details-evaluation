#ifndef ZERO_DETAILS_EVALUATION_BOOST_LOCKFREE_QUEUE_FIXED_SIZE_HPP
#define ZERO_DETAILS_EVALUATION_BOOST_LOCKFREE_QUEUE_FIXED_SIZE_HPP

#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include <boost/lockfree/queue.hpp>

namespace _boost_lockfree_queue_fixed_size {

    boost::lockfree::queue<uint_fast32_t, boost::lockfree::capacity<block_count - 1>> _freelist;
    std::atomic<uint_fast32_t> _approx_freelist_length;

    // http://www.boost.org/doc/libs/1_63_0/doc/html/lockfree.html#lockfree.introduction___motivation.data_structure_configuration
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.pop(pageID);
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
                        _freelist.push(std::move(pageID));
                    } else {
                        _freelist.push(pageID);
                    }
                    _approx_freelist_length++;
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                }
            }
        }
    }

    void init(const uint_fast32_t& block_count) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.push(i);
            _approx_freelist_length++;
        }
    }

}

#endif //ZERO_DETAILS_EVALUATION_BOOST_LOCKFREE_QUEUE_FIXED_SIZE_HPP
