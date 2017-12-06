#ifndef EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_FREE_LIST_HPP
#define EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_FREE_LIST_HPP

#include "config.hpp"

class FreeList {
public:
    virtual void use(std::array<uint_fast32_t, block_count>& pageIDs, std::array<std::atomic_flag, block_count>& pageUnused) {};

    virtual void init() {};

    virtual bool useCDSThreadManagement() {
        return false;
    }
};

#endif //EVALUATION_OF_IMPLEMENTATION_DETAILS_FOR_ZERO_FREE_LIST_HPP
