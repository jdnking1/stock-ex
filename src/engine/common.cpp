#include "common.hpp"

#include <iomanip>
#include <sstream>

namespace engine::common {
    Side toSide(std::string const& str) {
        if (str == "BUY") {
            return Side::BUY;
        }
        else {
            return Side::SELL;
        }
    }

    std::string toString(Side side) {
        switch (side) {
        case Side::BUY:
            return "BUY";
            break;
        case Side::SELL:
            return "SELL";
            break;
        default:
            return "INVALID";
        }
    }

    RequestsName toRequestName(const std::string& str) {
        if (str == "INSERT") {
            return RequestsName::INSERT;
        }
        else if (str == "AMEND") {
            return RequestsName::AMEND;
        }
        else if (str == "PULL") {
            return RequestsName::PULL;
        }
        else {
            return RequestsName::INVALID;
        }
    }

    bool checkPrice(const std::string& priceStr) {
        auto decimalPos = priceStr.find('.');
        if (decimalPos != std::string::npos && priceStr.size() - decimalPos - 1 > 4) {
            return false;
        }
        return true;
    }

    bool isSuperiorToZero(const std::string& str) {
        int num;
        std::stringstream iss(str);
        iss >> num;
        if (iss.fail() || !iss.eof()) {
            return false;
        }
        return num > 0;
    }
}