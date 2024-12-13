#pragma once

#include <vector>
#include <limits>
#include <string>

namespace engine::common {
    inline constexpr size_t MAX_GLOBAL_ORDER_NUMBER = 10000;

    enum class Side : int {
        BUY = 0,
        SELL
    };

    enum class RequestsName : int {
        INVALID = -1,
        INSERT,
        AMEND,
        PULL
    };

    struct CustomFloat {
        float value;
        std::string strValue;

        bool operator<(const CustomFloat& other) const {
            return (value < other.value) && (std::abs(value - other.value) > std::numeric_limits<float>::epsilon());
        }
    };


    Side toSide(std::string const& str);
    std::string toString(Side side);

    RequestsName toRequestName(std::string const& str);

    bool checkPrice(const std::string& priceStr);

    bool isSuperiorToZero(const std::string& str);
}