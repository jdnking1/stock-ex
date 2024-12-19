#pragma once 


#include "common.hpp"

#include <string>

namespace engine {
    struct Order {
        std::string id;
        size_t volume;
        common::CustomFloat price;
        common::Side side;
    };

    Order makeOrder(std::vector<std::string> const& inputs);
}
