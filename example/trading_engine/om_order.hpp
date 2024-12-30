#pragma once

#include <array>
#include <cstdint>
#include <sstream>

#include "models/basic_types.hpp"

namespace kse::example::trading {
        enum class om_order_state : uint8_t {
            INVALID = 0,
            PENDING_NEW = 1,
            LIVE = 2,
            PENDING_CANCEL = 3,
            DEAD = 4,
            PENDING_MODIFIED = 5
        };

        inline auto om_order_state_to_string(om_order_state side) -> std::string {
            switch (side) {
            case om_order_state::PENDING_NEW:
                return "PENDING_NEW";
            case om_order_state::LIVE:
                return "LIVE";
            case om_order_state::PENDING_CANCEL:
                return "PENDING_CANCEL";
            case om_order_state::DEAD:
                return "DEAD";
            case om_order_state::PENDING_MODIFIED:
                return "PENDING_MODIFIED";
            case om_order_state::INVALID:
                return "INVALID";
            }

            return "UNKNOWN";
        }

        struct om_order {
            models::instrument_id_t instrument_id_ = models::INVALID_INSTRUMENT_ID;
            models::order_id_t order_id_ = models::INVALID_ORDER_ID;
            models::side_t side_ = models::side_t::INVALID;
            models::price_t price_ = models::INVALID_PRICE;
            models::quantity_t qty_ = models::INVALID_QUANTITY;
            om_order_state order_state_ = om_order_state::INVALID;

            auto to_string() const {
                std::stringstream ss;
                ss << "OMOrder" << "["
                    << "tid:" << models::instrument_id_to_string(instrument_id_) << " "
                    << "oid:" << models::order_id_to_string(order_id_) << " "
                    << "side:" << models::side_to_string(side_) << " "
                    << "price:" << models::price_to_string(price_) << " "
                    << "qty:" << models::quantity_to_string(qty_) << " "
                    << "state:" << om_order_state_to_string(order_state_) << "]";

                return ss.str();
            }
        };

        using om_order_by_side =  std::array<om_order, 2>;
        using om_order_by_instrument_side = std::array<om_order_by_side, models::MAX_NUM_INSTRUMENTS>;
}