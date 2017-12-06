#ifndef __free_list_queue_alternatives_H_
#define __free_list_queue_alternatives_H_

#include "../evaluation_framework.hpp"

#include "free_list.hpp"
#include "boost_lockfree_queue.hpp"
#include "boost_lockfree_queue_fixed_size.hpp"
#include "cds_container_basketqueue.hpp"
#include "cds_container_fcqueue.hpp"
#include "cds_container_moirqueue.hpp"
#include "cds_container_msqueue.hpp"
#include "cds_container_optimisticqueue.hpp"
#include "cds_container_rwqueue.hpp"
#include "cds_container_segmented_queue.hpp"
#include "cds_container_vyukovmpmccyclequeue.hpp"
#include "folly_mpmcqueue.hpp"
#include "legacy_zero_stack.hpp"
#include "lockfree_queue_mpmc_fixed_bounded_value.hpp"
#include "moodycamel_concurrent_queue.hpp"
#include "mpmc_bounded_queue.hpp"
#include "mpmc_bounded_queue_t.hpp"
#include "rigtorp_mpmcqueue.hpp"
#include "tbb_concurrent_bounded_queue.hpp"
#include "tbb_concurrent_queue.hpp"

class FreeListQueueAlternatives : public  Evaluation {
public:
    FreeListQueueAlternatives() : Evaluation("Benchmark Free List Queue Alternatives") {};

    ~FreeListQueueAlternatives() {
        delete(queue);
    }

    std::array<uint_fast32_t, block_count>      pageIDs;
    std::array<std::atomic_flag, block_count>   pageUnused;

protected:
    void setSpecificOptions();

    void setSpecificConfig();

    void initialize();

    void printSpecificConfiguration();

    void printSpecificConfigurationExtended();

    void work();

    void before();

    void after();

    void printSpecificResult();

    void printSpecificResultExtended();

    void unInitialize();

private:
    FreeList*       queue;

    uint_fast32_t   freeBatchSize;
    uint_fast64_t   workTimeInNS;

    std::string     useQueue;
    bool            useMove;

};


#endif // __free_list_queue_alternatives_H_
