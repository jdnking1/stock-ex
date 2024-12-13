#include "order.hpp"

#include <sstream>
#include <string>

namespace engine {
    Order makeOrder(std::vector<std::string> const& req) {
        common::CustomFloat price{ std::stof(req[4]),  req[4] };
        auto volume = std::stoul(req[5]);
        auto id = req[1];
        auto side = common::toSide(req[3]);

        return { id, volume, price, side };
    }
}