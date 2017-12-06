#include "free_list_queue_alternatives.hpp"
#include "config.hpp"

#include <cds/init.h>
#include <cds/gc/hp.h>

void FreeListQueueAlternatives::setSpecificOptions() {
    specificOptions->add_options()
            ("free_batch,f", po::value<uint_fast32_t>(&freeBatchSize)->default_value(0.1 * block_count)->notifier([](uint_fast32_t value) { if (value <= 0 || value > block_count) {throw po::invalid_option_value(std::to_string(value));}}), "Number of blocks freed at once.")
            ("queue,q", po::value<std::string>(&useQueue)->required(), "Used concurrent queue/stack.\n"
                    "Possible values:\n"
                    "- boost::lockfree::queue\n"
                    "- boost::lockfree::queue_fixed_size\n"
                    "- cds::container::BasketQueue\n"
                    "- cds::container::FCQueue\n"
                    "- cds::container::MoirQueue\n"
                    "- cds::container::MSQueue\n"
                    "- cds::container::OptimisticQueue\n"
                    "- cds::container::RWQueue\n"
                    "- cds::container::SegmentedQueue\n"
                    "- cds::container::VyukovMPMCCycleQueue\n"
                    "- folly::MPMCQueue\n"
                    "- legacy\n"
                    "- lockfree_queue::mpmc_fixed_bounded_value\n"
                    "- moodycamel::ConcurrentQueue\n"
                    "- mpmc_bounded_queue\n"
                    "- mpmc_bounded_queue_t\n"
                    "- rigtorp::MPMCQueue\n"
                    "- tbb::concurrent_bounded_queue\n"
                    "- tbb::concurrent_queue")
            ("move,m", po::bool_switch(&useMove)->default_value(false), "Use std::move on enqueue.")
            ("work,w", po::value<uint_fast64_t>(&workTimeInNS)->default_value(0), "Work time between iterations.");
}

void FreeListQueueAlternatives::setSpecificConfig() {
    thread_count = threadCount;
    iteration_count = iterationsCount;
    free_batch_size = freeBatchSize;
    work_time_ns = workTimeInNS;
    timeout_ns = timeoutInNS;

    move = useMove;

    extended_output = extendedOutput;
    debug = debugOutput;
}

void FreeListQueueAlternatives::initialize() {
    if (extended_output) std::cout << "Start initialization of the free list with " << (block_count - 1) << " free pages." << std::endl;

    cds::Initialize();
    cds::gc::HP garbadge_collector(0, thread_count + 1, 0);

    std::iota(pageIDs.begin(), pageIDs.end(), 0);
    for (uint_fast32_t i = 1; i < block_count; i++) {
        pageUnused[i].test_and_set(std::memory_order_consume);
    }

    if (useQueue == "boost::lockfree::queue")
        queue = new BoostLockFreeQueue();
    else if (useQueue == "boost::lockfree::queue_fixed_size")
        queue = new BoostLockfreeQueueFixedSize();
    else if (useQueue == "cds::container::BasketQueue")
        queue = new CDSContainerBasketQueue();
    else if (useQueue == "cds::container::FCQueue")
        queue = new CDSContainerFCQueue();
    else if (useQueue == "cds::container::MoirQueue")
        queue = new CDSContainerMoirqueue();
    else if (useQueue == "cds::container::MSQueue")
        queue = new CDSContainerMSQueue();
    else if (useQueue == "cds::container::OptimisticQueue")
        queue = new CDSContainerOptimisticQueue();
    else if (useQueue == "cds::container::RWQueue")
        queue = new CDSContainerRWQueue();
    else if (useQueue == "cds::container::SegmentedQueue")
        queue = new CDSContainerSegmentedQueue();
    else if (useQueue == "cds::container::VyukovMPMCCycleQueue")
        queue = new CDSContainerVyukovMPMCCycleQueue();
    else if (useQueue == "folly::MPMCQueue")
        queue = new FollyMPMCQueue();
    else if (useQueue == "legacy")
        queue = new LegacyZeroStack();
    else if (useQueue == "lockfree_queue::mpmc_fixed_bounded_value")
        queue = new LockfreeQueueMPMCFixedBoundedValue();
    else if (useQueue == "moodycamel::ConcurrentQueue")
        queue = new MoodycamelConcurrentQueue();
    else if (useQueue == "mpmc_bounded_queue")
        queue = new MPMCBoundedQueue();
    else if (useQueue == "mpmc_bounded_queue_t")
        queue = new MPMCBoundedQueueT();
    else if (useQueue == "rigtorp::MPMCQueue")
        queue = new RigtorpMPMCQueue();
    else if (useQueue == "tbb::concurrent_bounded_queue")
        queue = new TBBConcurrentBoundedQueue();
    else if (useQueue == "tbb::concurrent_queue")
        queue = new TBBConcurrentQueue();
    else {
        std::cerr << "ERROR: " << "The argument " << useQueue << " is invalid for option --queue." << std::endl << std::endl;
        std::cerr << allOptions << std::endl;
        exit(1);
    }

    if (extended_output) std::cout << "Finished initialization of the free list with " << (block_count - 1) << " free pages." << std::endl;
}

void FreeListQueueAlternatives::printSpecificConfiguration() {
    std::cout << "\t" << block_count << "\t" << freeBatchSize << "\t" << useQueue << "\t" << (useMove ? "move" : "") << "\t" << workTimeInNS;
}

void FreeListQueueAlternatives::printSpecificConfigurationExtended() {
    std::cout << "Blocks: " << block_count << std::endl;
    std::cout << "Free Batch Size: " << freeBatchSize << std::endl;
    std::cout << "Concurrent Queue: " << useQueue << std::endl;
    std::cout << "Use std::move: " << (useMove ? "Yes" : "No") << std::endl;
    std::cout << "Work Time: " << std::chrono::nanoseconds(workTimeInNS) << std::endl;
}

void FreeListQueueAlternatives::work() {
    queue->use(pageIDs, pageUnused);
}

void FreeListQueueAlternatives::before() {
    if (queue->useCDSThreadManagement()) cds::threading::Manager::attachThread();
}

void FreeListQueueAlternatives::after() {
    if (queue->useCDSThreadManagement()) cds::threading::Manager::detachThread();
}

void FreeListQueueAlternatives::printSpecificResult() {}

void FreeListQueueAlternatives::printSpecificResultExtended() {}

void FreeListQueueAlternatives::unInitialize() {
    cds::Terminate();
}

int main(int argc, char *argv[]) {

    FreeListQueueAlternatives evaluation;
    evaluation.run(argc, argv);

}
