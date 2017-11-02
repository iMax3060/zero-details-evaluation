#ifndef EVALUATION_RANDOM_GENERATOR_HPP
#define EVALUATION_RANDOM_GENERATOR_HPP

#include "config.hpp"

thread_local bool seed_initialized;
thread_local uint32_t seed_0;

inline uint_fast32_t fast_random() {
    if (!seed_initialized) {
        seed_0 = std::random_device{}();
        seed_initialized = true;
    }
    seed_0 = 214013 * seed_0 + 2531011;
    return ((seed_0 >> 16 & 0x7FFF) % (block_count - 1)) + 1;
}

constexpr uint_fast32_t nextPowerOfTwo64(uint_fast32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;

    if constexpr (sizeof(uint_fast32_t) == 4) {
        v++;
        return v;
    } else if constexpr (sizeof(uint_fast32_t) == 8) {
        v |= v >> 32;
        v++;
        return v;
    } else {
        std::cerr << "sizeof(uint_fast32_t) == " << sizeof(uint_fast32_t) << " -- Only 32 bit and 64 bit architectures are supported" << std::endl;
        exit(64);
    }
}

#endif //EVALUATION_RANDOM_GENERATOR_HPP
