#include <chrono>
#include <random>
#include <thread>
#include <iostream>
#include <cstring>
#include <cds/init.h>
#include <cds/gc/hp.h>
#include <cmath>
#include <boost/program_options.hpp>

#include "config.hpp"

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

std::array<uint_fast32_t, block_count> pageIDs;
std::array<std::atomic_flag, block_count> pageUnused;

template <void (*execute_queue)(std::array<uint_fast32_t, block_count>&, std::array<std::atomic_flag, block_count>&)>
void worker_thread(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused, bool use_cds_thread_management = false) {
    if (use_cds_thread_management) cds::threading::Manager::attachThread();
    for (unsigned int i = 0; i < iteration_count; i++) {
        execute_queue(pageIDs, pageUnused);
    }
    if (use_cds_thread_management) cds::threading::Manager::detachThread();
}

void timeout(uint_fast64_t timeout_ns, uint_fast32_t thread_count, uint_fast32_t iteration_count, bool extended_output) {
    std::this_thread::sleep_for(std::chrono::nanoseconds(thread_count * iteration_count * timeout_ns));
    if (extended_output) {
        std::cout << "##################################################################################################################" << std::endl
                  << "Results:" << std::endl
                  << "Time Elapsed: " << "timeout" << std::endl
                  << "##################################################################################################################" << std::endl;
    } else {
        std::cout << "    timeout" << std::endl;
    }
    exit(512);
}

int main(int argc, char *argv[]) {
    /*
     * Validate program options:
     */
    namespace po = boost::program_options;
    po::options_description options_description("Options");
    options_description.add_options()
            ("help,h", "Print this help messages.")
            ("threads,t", po::value<uint_fast32_t>(&thread_count)->default_value(std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : 1)->notifier([](uint_fast32_t value) { if (value <= 0) {throw po::invalid_option_value(std::to_string(value));}}), "Number of threads to use.")
            ("iterations,i", po::value<uint_fast32_t>(&iteration_count)->default_value(1000000)->notifier([](uint_fast32_t value) { if (value <= 0) {throw po::invalid_option_value(std::to_string(value));}}), "Number of iterations per thread.")
            ("free_batch,f", po::value<uint_fast32_t>(&free_batch_size)->default_value(0.1 * block_count)->notifier([](uint_fast32_t value) { if (value <= 0 || value > block_count) {throw po::invalid_option_value(std::to_string(value));}}), "Number of blocks managed by the free list.")
            ("queue,q", po::value<std::string>(&queue)->required(), "Used concurrent queue/stack.\n"
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
            ("move,m", po::bool_switch(&move)->default_value(false), "Use std::move on enqueue.")
            ("work,w", po::value<uint_fast64_t>(&work_time_ns)->default_value(0), "Work time between iterations.")
            ("timeout", po::value<uint_fast64_t>(&timeout_ns)->default_value(10000), "Timeout per thread and iteration until the running threads get terminated (0 is no timeout).")
            ("debug,d", po::bool_switch(&debug)->default_value(false), "Print additional debug information.")
            ("extended,e", po::bool_switch(&extended_output)->default_value(false), "Print extended output.");

    po::variables_map options;
    try {
        po::store(po::parse_command_line(argc, argv, options_description), options); // can throw

        if (options.count("help")) {
            std::cout << "Benchmark Free List Queue Alternatives" << std::endl
                      << options_description << std::endl;
            exit(0);
        }

        po::notify(options); // throws on error, so do after help in case there are any problems
    } catch(po::error& e) {
        std::cerr << "ERROR: " << e.what() << std::endl << std::endl;
        std::cerr << options_description << std::endl;
        exit(1);
    }

    if (extended_output) {
        std::cout << "##################################################################################################################" << std::endl
                  << "Configuration:" << std::endl
                  << "Threads: " << thread_count << std::endl
                  << "Iterations: " << iteration_count << std::endl
                  << "Block Count: " << block_count << std::endl
                  << "Concurrent Queue: " << queue << std::endl
                  << "Use std::move: " << (move ? "yes" : "no") << std::endl
                  << "##################################################################################################################" << std::endl;
    } else {
        std::cout << queue << (move ? " move" : "");
    }

    if (extended_output) std::cout << "Start initialization of the free list with " << (block_count - 1) << " free pages." << std::endl;

    cds::Initialize();
    cds::gc::HP garbadge_collector(0, thread_count + 1, 0);

    std::iota(pageIDs.begin(), pageIDs.end(), 0);
    for (uint_fast32_t i = 1; i < block_count; i++) {
        pageUnused[i].test_and_set(std::memory_order_consume);
    }

    _boost_lockfree_queue::init(block_count);
    _boost_lockfree_queue_fixed_size::init(block_count);
    _cds_container_basketqueue::init(block_count);
    _cds_container_fcqueue::init(block_count);
    _cds_container_moirqueue::init(block_count);
    _cds_container_msqueue::init(block_count);
    _cds_container_optimisticqueue::init(block_count);
    _cds_container_rwqueue::init(block_count);
    _cds_container_segmentedqueue::init(block_count);
    _cds_container_vyukovmpmccyclequeue::init(block_count);
    _folly_mpmcqueue::init(block_count);
    _legacy_zero_stack::init(block_count);
    _lockfree_queue_mpmc_fixed_bounded_value::init(block_count);
    _moodycamel_concurrent_queue::init(block_count);
    _mpmc_bounded_queue::init(block_count);
    _mpmc_bounded_queue_t::init(block_count);
    _rigtrop_mpmcqueue::init(block_count);
    _tbb_concurrent_bounded_queue::init(block_count);
    _tbb_concurrent_queue::init(block_count);

    if (extended_output) std::cout << "Finished initialization of the free list with " << (block_count - 1) << " free pages." << std::endl;

    u_long start;
    u_long end;

    std::thread* worker_threads[thread_count];

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    std::thread timeout_thread = std::thread(timeout, timeout_ns, thread_count, iteration_count, extended_output);

    start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    if (extended_output) std::cout << "Start spawning " << thread_count << " threads ..." << std::endl;
    for (uint_fast32_t i = 0; i < thread_count; i++) {
        if (queue == "boost::lockfree::queue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_boost_lockfree_queue::use>(pageIDs, pageUnused);});
        } else if (queue == "boost::lockfree::queue_fixed_size") {
            worker_threads[i] = new std::thread([&]{worker_thread<_boost_lockfree_queue_fixed_size::use>(pageIDs, pageUnused);});
        } else if (queue == "cds::container::BasketQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_basketqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::FCQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_fcqueue::use>(pageIDs, pageUnused);});
        } else if (queue == "cds::container::MoirQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_moirqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::MSQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_msqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::OptimisticQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_optimisticqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::RWQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_rwqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::SegmentedQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_segmentedqueue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "cds::container::VyukovMPMCCycleQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_cds_container_vyukovmpmccyclequeue::use>(pageIDs, pageUnused, true);});
        } else if (queue == "folly::MPMCQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_folly_mpmcqueue::use>(pageIDs, pageUnused);});
        } else if (queue == "legacy") {
            worker_threads[i] = new std::thread([&]{worker_thread<_legacy_zero_stack::use>(pageIDs, pageUnused);});
        } else if (queue == "lockfree_queue::mpmc_fixed_bounded_value") {
            worker_threads[i] = new std::thread([&]{worker_thread<_lockfree_queue_mpmc_fixed_bounded_value::use>(pageIDs, pageUnused);});
        } else if (queue == "moodycamel::ConcurrentQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_moodycamel_concurrent_queue::use>(pageIDs, pageUnused);});
        } else if (queue == "mpmc_bounded_queue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_mpmc_bounded_queue::use>(pageIDs, pageUnused);});
        } else if (queue == "mpmc_bounded_queue_t") {
            worker_threads[i] = new std::thread([&]{worker_thread<_mpmc_bounded_queue_t::use>(pageIDs, pageUnused);});
        } else if (queue == "rigtorp::MPMCQueue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_rigtrop_mpmcqueue::use>(pageIDs, pageUnused);});
        } else if (queue == "tbb::concurrent_bounded_queue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_tbb_concurrent_bounded_queue::use>(pageIDs, pageUnused);});
        } else if (queue == "tbb::concurrent_queue") {
            worker_threads[i] = new std::thread([&]{worker_thread<_tbb_concurrent_queue::use>(pageIDs, pageUnused);});
        } else {
            std::cerr << "The argument " << argv[1] << " is unknown! Valid arguments are boost::lockfree::queue, boost::lockfree::queue_fixed_size, cds::container::BasketQueue, cds::container::FCQueue, cds::container::MoirQueue, cds::container::MSQueue, cds::container::OptimisticQueue, cds::container::RWQueue, cds::container::SegmentedQueue, cds::container::VyukovMPMCCycleQueue, folly::MPMCQueue, legacy, lockfree_queue::mpmc_fixed_bounded_value, moodycamel::ConcurrentQueue, mpmc_bounded_queue, mpmc_bounded_queue_t, rigtorp::MPMCQueue, tbb::concurrent_bounded_queue and tbb::concurrent_queue." << std::endl;
            exit(2);
        }
    }
    if (extended_output) std::cout << "Finished spawning " << thread_count << " threads ..." << std::endl;

    if (extended_output) std::cout << "Waiting for " << thread_count << " threads to complete ..." << std::endl;
    for (uint_fast32_t i = 0; i < thread_count; i++) {
        worker_threads[i]->join();
    }
    if (extended_output) std::cout << "All " << thread_count << " threads completed ..." << std::endl;
    end = std::chrono::high_resolution_clock::now().time_since_epoch().count();

    cds::Terminate();

    if (extended_output) {
        std::cout << "##################################################################################################################" << std::endl
                  << "Results:" << std::endl
                  << "Time Elapsed: " << (end - start) << "ns" << std::endl
                  << "##################################################################################################################" << std::endl;
    } else {
        std::cout << "    " << (end - start) << std::endl;
    }

    _legacy_zero_stack::destroy();

    exit(0);
}