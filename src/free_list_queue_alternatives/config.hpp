#ifndef EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_CONFIG_HPP
#define EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_CONFIG_HPP

// Options regarding benchmark execution:
uint_fast32_t thread_count;
uint_fast32_t iteration_count;
const uint_fast32_t block_count = 6523;
uint_fast32_t free_batch_size = 0;
uint_fast64_t work_time_ns;
uint_fast64_t timeout_ns;

// Options regarding implementation alternative:
std::string queue;
bool move;

// Options regarding output:
bool extended_output;
bool debug;

#endif //EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_CONFIG_HPP
