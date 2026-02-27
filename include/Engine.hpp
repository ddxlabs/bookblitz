#pragma once

/**
 * @file Engine.hpp
 * @brief Event loop handling for book updates and feature computation.
 */ 

#include "OrderBook.hpp"

#include <string>
#include <vector>

struct SimulationResult {
    std::string name;
    std::vector<double> data;
};

class Engine {
private:
  OrderBook book_;

public:  
    /* @brief: Event processing loop.
     */ 
    void run(
        int64_t n_events, const int8_t *actions, const int8_t *sides,
        const int64_t *prices, const int64_t *sizes, const uint64_t *order_ids
    );
};
