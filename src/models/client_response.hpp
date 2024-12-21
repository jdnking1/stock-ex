
#pragma once

#include <sstream>

#include "constants.hpp"
#include "basic_types.hpp"

#include "utils/lock_free_queue.hpp"


namespace kse::models {
#pragma pack(push, 1)
    enum class client_response_type : uint8_t {
        INVALID = 0,
        ACCEPTED = 1,
        CANCELED = 2,
        MODIFIED = 3,
        FILLED = 4,
        CANCEL_REJECTED = 5,
        MODIFY_REJECTED = 6
    };

    inline std::string client_response_type_to_string(client_response_type type) {
        switch (type) {
        case client_response_type::ACCEPTED:
            return "ACCEPTED";
        case client_response_type::CANCELED:
            return "CANCELED";
        case client_response_type::FILLED:
            return "FILLED";
        case client_response_type::CANCEL_REJECTED:
            return "CANCEL_REJECTED";
        case client_response_type::MODIFIED:
            return "MODIFIED";
        case client_response_type::MODIFY_REJECTED:
            return "MODIFY_REJECTED";
        case client_response_type::INVALID:
            return "INVALID";
        }
        return "UNKNOWN";
    }

    struct client_response {
        client_response_type type_ = client_response_type::INVALID;

        client_id_t client_id_ = INVALID_CLIENT_ID;
        instrument_id_t instrument_id_ = INVALID_INSTRUMENT_ID;
        order_id_t client_order_id_ = INVALID_ORDER_ID;
        order_id_t market_order_id_ = INVALID_ORDER_ID;
        side_t side_ = side_t::INVALID;
        price_t price_ = INVALID_PRICE;
        quantity_t exec_qty_ = INVALID_QUANTITY;
        quantity_t leaves_qty_ = INVALID_QUANTITY;

        auto to_string() const {
            std::stringstream ss;
            ss << "MEClientRequest"
                << " ["
                << "type:" << client_response_type_to_string(type_)
                << " client:" << client_id_to_string(client_id_)
                << " instrument:" << instrument_id_to_string(instrument_id_)
                << " coid:" << order_id_to_string(client_order_id_)
                << " moid:" << order_id_to_string(market_order_id_)
                << " side:" << side_to_string(side_)
                << " exec qty:" << quantity_to_string(exec_qty_)
                << " leaves qty:" << quantity_to_string(leaves_qty_)
                << " price:" << price_to_string(price_)
                << "]";
            return ss.str();
        }
    };

#pragma pack(pop)

    using client_response_queue = kse::utils::lock_free_queue<client_response>;
}
