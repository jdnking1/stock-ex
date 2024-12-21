#pragma once

#include <array>
#include <sstream>

#include "constants.hpp"
#include "basic_types.hpp"

namespace kse::models {
	struct order {
		instrument_id_t instrument_id_ = INVALID_INSTRUMENT_ID;
		client_id_t client_id_ = INVALID_CLIENT_ID;
		order_id_t client_order_id_ = INVALID_ORDER_ID;
		order_id_t market_order_id_ = INVALID_ORDER_ID;
		side_t side_ = side_t::INVALID;
		price_t price_ = INVALID_PRICE;
		quantity_t qty_ = INVALID_QUANTITY;
		priority_t priority_ = INVALID_PRIORITY;

		order* prev_order_ = nullptr;
		order* next_order_ = nullptr;

		order() = default;

		order(instrument_id_t instrument_id, client_id_t client_id, order_id_t client_order_id, order_id_t market_order_id, 
			  side_t side, price_t price, quantity_t qty, priority_t priority, order* prev_order, order* next_order) noexcept
			: instrument_id_{ instrument_id }, client_id_{ client_id }, client_order_id_{ client_order_id }, market_order_id_{ market_order_id },
			side_{ side }, price_{ price }, qty_{ qty }, priority_{ priority }, prev_order_{ prev_order }, next_order_{ next_order } {}

		auto to_string() const -> std::string {
			std::stringstream ss;
			ss << "order["
				<< "instrument_id:" << instrument_id_to_string(instrument_id_) << " "
				<< "market_order_id:" << order_id_to_string(market_order_id_) << " "
				<< "client_id:" << client_id_to_string(client_id_) << " "
				<< "client_order_id:" << order_id_to_string(client_order_id_) << " "
				<< "side:" << side_to_string(side_) << " "
				<< "price:" << price_to_string(price_) << " "
				<< "qty:" << quantity_to_string(qty_) << " "
				<< "priority:" << priority_to_string(priority_) << "]";
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

	using order_map = std::array<order*, MAX_NUM_INSTRUMENTS>;
	using client_order_map = std::array<order_map, MAX_NUM_CLIENTS>;
	using order_at_price_level_map = std::array<price_level*, MAX_PRICE_LEVELS>;
}