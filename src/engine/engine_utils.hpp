#pragma once

#include "common.hpp"

#include <vector>

namespace engine::utils {
    std::vector<std::string> parseInput(std::string const& input);

    bool invalidPriceOrQty(const std::string& price, const std::string& qty);
}
