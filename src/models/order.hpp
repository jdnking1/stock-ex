#pragma once

#include <array>
#include <sstream>

#include "constants.hpp"
#include "basic_types.hpp"

namespace kse::models {
    struct order {
        order_id_t market_order_id_ = kse::models::INVALID_ORDER_ID;
        client_id_t client_id_ = kse::models::INVALID_CLIENT_ID;
        order_id_t client_order_id_ = kse::models::INVALID_ORDER_ID;
        side_t side_ = kse::models::side_t::INVALID;
        price_t price_ = kse::models::INVALID_PRICE;
        quantity_t qty_ = kse::models::INVALID_QUANTITY;
        priority_t priority_ = kse::models::INVALID_PRIORITY;

        order* prev_order_ = nullptr;
        order* next_order_ = nullptr;

        order() = default;

        order(kse::models::order_id_t market_order_id, kse::models::client_id_t client_id,
            kse::models::order_id_t client_order_id, kse::models::side_t side,
            kse::models::price_t price, kse::models::quantity_t qty,
            kse::models::priority_t priority, order* prev_order, order* next_order) noexcept
            : market_order_id_(market_order_id), client_id_(client_id),
            client_order_id_(client_order_id), side_(side), price_(price),
            qty_(qty), priority_(priority), prev_order_(prev_order), next_order_(next_order) {
        }

        auto to_string() const -> std::string {
            std::stringstream ss;
            ss << "order["
                << "market_order_id:" << kse::models::order_id_to_string(market_order_id_) << " "
                << "client_id:" << kse::models::client_id_to_string(client_id_) << " "
                << "client_order_id:" << kse::models::order_id_to_string(client_order_id_) << " "
                << "side:" << kse::models::side_to_string(side_) << " "
                << "price:" << kse::models::price_to_string(price_) << " "
                << "qty:" << kse::models::quantity_to_string(qty_) << " "
                << "priority:" << kse::models::priority_to_string(priority_) << "]";
            return ss.str();
        }
    };

    struct price_level {
        side_t side_ = side_t::INVALID;
        price_t price_ = INVALID_PRICE;
        order* first_order_ = nullptr;
        price_level* prev_entry_ = nullptr;
        price_level* next_entry_ = nullptr;

        price_level() = default;

        price_level(side_t side, price_t price, order* first_order,
            price_level* prev_entry, price_level* next_entry)
            : side_(side), price_(price), first_order_(first_order),
            prev_entry_(prev_entry), next_entry_(next_entry) {
        }

        auto toString() const -> std::string {
            std::stringstream ss;
            ss << "price_level["
                << "side:" << side_to_string(side_) << " "
                << "price:" << price_to_string(price_) << " "
                << "first_me_order:" << (first_order_ ? first_order_->to_string() : "null") << " "
                << "prev:" << price_to_string(prev_entry_ ? prev_entry_->price_ : INVALID_PRICE) << " "
                << "next:" << price_to_string(next_entry_ ? next_entry_->price_ : INVALID_PRICE) << "]";
            return ss.str();
        }
    };

    using order_hash_map = std::array<order*, MAX_NUM_INSTRUMENTS>;
    using client_order_hash_map = std::array<order_hash_map, MAX_NUM_CLIENTS>;
    using order_at_price_level_map = std::array<price_level*, MAX_PRICE_LEVELS>;
}