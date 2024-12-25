#pragma once

#include <sstream>

#include "constants.hpp"
#include "basic_types.hpp"

#include "utils/lock_free_queue.hpp"

namespace kse::models {
	enum class market_update_type : uint8_t {
		INVALID = 0,
		CLEAR = 1,
		ADD = 2,
		MODIFY = 3,
		CANCEL = 4,
		TRADE = 5,
		SNAPSHOT_START = 6,
		SNAPSHOT_END = 7,
	};

	inline std::string market_update_type_to_string(market_update_type type) {
		switch (type) {
		case market_update_type::CLEAR:
			return "CLEAR";
		case market_update_type::ADD:
			return "ADD";
		case market_update_type::MODIFY:
			return "MODIFY";
		case market_update_type::CANCEL:
			return "CANCEL";
		case market_update_type::TRADE:
			return "TRADE";
		case market_update_type::SNAPSHOT_START:
			return "SNAPSHOT_START";
		case market_update_type::SNAPSHOT_END:
			return "SNAPSHOT_END";
		case market_update_type::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

#pragma pack(push, 1)
	struct market_update {
		market_update_type type_ = market_update_type::INVALID;

		order_id_t order_id_ = INVALID_ORDER_ID;
		instrument_id_t instrument_id_ = INVALID_INSTRUMENT_ID;
		side_t side_ = side_t::INVALID;
		price_t price_ = INVALID_PRICE;
		quantity_t qty_ = INVALID_QUANTITY;
		priority_t priority_ = INVALID_PRIORITY;

		auto to_string() const {
			std::stringstream ss;
			ss << "market_update"
				<< " ["
				<< " type:" << market_update_type_to_string(type_)
				<< " instrument:" << instrument_id_to_string(instrument_id_)
				<< " oid:" << order_id_to_string(order_id_)
				<< " side:" << side_to_string(side_)
				<< " qty:" << quantity_to_string(qty_)
				<< " price:" << price_to_string(price_)
				<< " priority:" << priority_to_string(priority_)
				<< "]";
			return ss.str();
		}
	};

	struct client_market_update {
		size_t sequence_number_ = 0;
		market_update update_;

		auto to_string() const {
			std::stringstream ss;
			ss << "client_market_update"
				<< " ["
				<< " sequence number:" << sequence_number_
				<< " update:" << update_.to_string()
				<< "]";
			return ss.str();
		}
	};
#pragma pack(pop)

	using market_update_queue = kse::utils::lock_free_queue<market_update>;
}