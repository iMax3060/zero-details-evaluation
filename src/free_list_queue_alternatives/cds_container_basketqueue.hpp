#ifndef ZERO_DETAILS_EVALUATION_CDS_CONTAINER_BASKETQUEUE_HPP
#define ZERO_DETAILS_EVALUATION_CDS_CONTAINER_BASKETQUEUE_HPP

#include "free_list.hpp"
#include "config.hpp"
#include "helper_functions.hpp"

#include <atomic>
#include <cds/opt/options.h>
#include <cds/container/basket_queue.h>

class CDSContainerBasketQueue : public FreeList {
private:
    cds::container::BasketQueue<cds::gc::HP, uint_fast32_t>*    _freelist;
    std::atomic<uint_fast32_t>                                  _approx_freelist_length;

public:
    CDSContainerBasketQueue() {
        cds::threading::Manager::attachThread();
        _freelist = new cds::container::BasketQueue<cds::gc::HP, uint_fast32_t>;
        for (uint_fast32_t i = 1; i < block_count; i++) {
            _freelist->enqueue(i);
            _approx_freelist_length++;
        }
        cds::threading::Manager::detachThread();
    };

    // http://libcds.sourceforge.net/doc/cds-api/classcds_1_1container_1_1_basket_queue.html
    void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {
        uint_fast32_t pageID;
        bool popSuccessful = _freelist->dequeue(pageID);
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
                        _freelist->enqueue(std::move(pageID));
                        _approx_freelist_length++;
                    } else {
                        _freelist->enqueue(pageID);
                        _approx_freelist_length++;
                    }
                    std::this_thread::sleep_for(std::chrono::nanoseconds(work_time_ns));
                    if (debug) std::cout << _approx_freelist_length << std::endl;
                }
            }
        }
    };

    bool useCDSThreadManagement() {
        return true;
    };

};

#endif //ZERO_DETAILS_EVALUATION_CDS_CONTAINER_BASKETQUEUE_HPP
