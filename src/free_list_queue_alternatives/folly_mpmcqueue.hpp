#ifndef ZERO_DETAILS_EVALUATION_FOLLY_MPMCQUEUE_HPP
#define ZERO_DETAILS_EVALUATION_FOLLY_MPMCQUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <folly/MPMCQueue.h>

class FollyMPMCQueue : public FreeList {
private:
    folly::MPMCQueue<uint_fast32_t> _freelist;

public:
    FollyMPMCQueue() : _freelist(block_count - 1) {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.writeIfNotFull(i);
        }
    };

    // https://github.com/facebook/folly
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.readIfNotEmpty(pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist.sizeGuess() << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist.sizeGuess() < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    bool pushSuccessful;
                    if (move) {
                        pushSuccessful = _freelist.writeIfNotFull(std::move(pageID));
                    } else {
                        pushSuccessful = _freelist.writeIfNotFull(pageID);
                    }
                    if (!pushSuccessful) {
                        std::cerr << "There isn't enough memory allocated!" << std::endl;
                        exit(1);
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist.sizeGuess() << std::endl;
                }
            }
        }
    };

};

#endif //ZERO_DETAILS_EVALUATION_FOLLY_MPMCQUEUE_HPP
