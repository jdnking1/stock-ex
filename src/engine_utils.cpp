#include "engine_utils.hpp"

#include <sstream>


namespace engine::utils {
    std::vector<std::string> parseInput(const std::string& input) {
        std::vector<std::string> parsedEntry;
        std::stringstream stream{ input };
        std::string field;

        while (std::getline(stream, field, ',')) {
            if (!field.empty()) {
                parsedEntry.push_back(field);
            }
        }

        return parsedEntry;
    }

    bool invalidPriceOrQty(const std::string& price, const std::string& qty) {
        return  !common::checkPrice(price) || !common::isSuperiorToZero(qty);
    }
}