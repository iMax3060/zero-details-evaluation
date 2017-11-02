#ifndef ZERO_DETAILS_EVALUATION_MOODYCAMEL_CONCURRENT_QUEUE_HPP
#define ZERO_DETAILS_EVALUATION_MOODYCAMEL_CONCURRENT_QUEUE_HPP

#include "config.hpp"
#include "helper_functions.hpp"

#include "concurrentqueue/concurrentqueue.h"

namespace _moodycamel_concurrent_queue {

    moodycamel::ConcurrentQueue<uint_fast32_t> _freelist(block_count - 1, thread_count, 0);
    thread_local moodycamel::ProducerToken producer_token(_freelist);
    thread_local moodycamel::ConsumerToken consumer_token(_freelist);

    // https://github.com/cameron314/concurrentqueue
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.try_dequeue(consumer_token, pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist.size_approx() << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist.size_approx() < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    if (move) {
                        _freelist.enqueue(producer_token, std::move(pageID));
                    } else {
                        _freelist.enqueue(producer_token, pageID);
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist.size_approx() << std::endl;
                }
            }
        }
    }

    void init(const uint_fast32_t& block_count) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.try_enqueue(i);
        }
    }

}


#endif //ZERO_DETAILS_EVALUATION_MOODYCAMEL_CONCURRENT_QUEUE_HPP
