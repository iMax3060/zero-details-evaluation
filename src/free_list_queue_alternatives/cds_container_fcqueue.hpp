#ifndef ZERO_DETAILS_EVALUATION_CDS_CONTAINER_FCQUEUE_HPP
#define ZERO_DETAILS_EVALUATION_CDS_CONTAINER_FCQUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <cds/container/fcqueue.h>

class CDSContainerFCQueue : public FreeList {
private:
    cds::container::FCQueue<uint_fast32_t>  _freelist;

public:
    CDSContainerFCQueue() {
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist.enqueue(i);
        }
    };

    // http://libcds.sourceforge.net/doc/cds-api/classcds_1_1container_1_1_f_c_queue.html
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist.dequeue(pageID);
        if (popSuccessful) {
            if (debug) std::cout << _freelist.size() << std::endl;
            pageUnused[pageID].clear();
            std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
            __asm__ __volatile__(""::"m" (pageID));
        } else {
            while (_freelist.size() < free_batch_size) {
                pageID = fast_random();
                if (!pageUnused[pageID].test_and_set(std::memory_order_consume)) {
                    if (move) {
                        _freelist.enqueue(std::move(pageID));
                    } else {
                        _freelist.enqueue(pageID);
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _freelist.size() << std::endl;
                }
            }
        }
    };

};

#endif //ZERO_DETAILS_EVALUATION_CDS_CONTAINER_FCQUEUE_HPP
