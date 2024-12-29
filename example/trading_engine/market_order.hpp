#pragma once

#include "models/basic_types.hpp"

#include <array>
#include <vector>

using namespace kse::models;

namespace kse::example::trading {
			struct market_order {
				order_id_t market_order_id_ = INVALID_ORDER_ID;
				side_t side_ = side_t::INVALID;
				price_t price_ = INVALID_PRICE;
				quantity_t qty_ = INVALID_QUANTITY;
				priority_t priority_ = INVALID_PRIORITY;

				market_order* prev_order_ = nullptr;
				market_order* next_order_ = nullptr;

				market_order() = default;

				market_order(order_id_t market_order_id,  side_t side, price_t price, quantity_t qty, priority_t priority, market_order* prev_order, market_order* next_order) noexcept
					: market_order_id_{ market_order_id },side_{ side }, price_{ price }, qty_{ qty }, priority_{ priority }, 
					  prev_order_{ prev_order }, next_order_{ next_order } {
				}

				auto to_string() const -> std::string {
					std::stringstream ss;
					ss << "order["
						<< "market_order_id:" << order_id_to_string(market_order_id_) << " "
						<< "side:" << side_to_string(side_) << " "
						<< "price:" << price_to_string(price_) << " "
						<< "qty:" << quantity_to_string(qty_) << " "
						<< "priority:" << priority_to_string(priority_) << "]";
					return ss.str();
				}
			};

			struct market_price_level {
				side_t side_ = side_t::INVALID;
				price_t price_ = INVALID_PRICE;
				market_order* first_order_ = nullptr;
				market_price_level* prev_entry_ = nullptr;
				market_price_level* next_entry_ = nullptr;

				market_price_level() = default;

				market_price_level(side_t side, price_t price, market_order* first_order,
					market_price_level* prev_entry, market_price_level* next_entry)
					: side_(side), price_(price), first_order_(first_order),
					prev_entry_(prev_entry), next_entry_(next_entry) {
				}

				auto to_string() const -> std::string {
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


			struct bbo_t {
				price_t bid_price_ = INVALID_PRICE, ask_price_ = INVALID_PRICE;
				quantity_t bid_qty_ = INVALID_QUANTITY, ask_qty_ = INVALID_QUANTITY;

				auto to_string() const {
					std::stringstream ss;
					ss << "BBO{"
						<< bid_qty_ << "@" << bid_price_
						<< "X"
						<< ask_price_ << "@" << ask_qty_
						<< "}";

					return ss.str();
				}
			};

			using market_order_map = std::vector<market_order*>;
			using market_order_at_price_level_map = std::array<market_price_level*, MAX_PRICE_LEVELS>;
}