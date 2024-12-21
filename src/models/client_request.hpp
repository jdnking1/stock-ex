
#pragma once

#include <sstream>

#include "constants.hpp"
#include "basic_types.hpp"

#include "utils/lock_free_queue.hpp"


namespace kse::models {
#pragma pack(push, 1)
	enum class client_request_type : uint8_t {
		INVALID = 0,
		NEW = 1,
		CANCEL = 2,
		MODIFY = 3
	};

	inline std::string client_request_type_to_string(client_request_type type) {
		switch (type) {
		case client_request_type::NEW:
			return "NEW";
		case client_request_type::CANCEL:
			return "CANCEL";
		case client_request_type::MODIFY:
			return "MODIFY";
		case client_request_type::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct client_request {
		client_request_type type_ = client_request_type::INVALID;

		client_id_t client_id_ = INVALID_CLIENT_ID;
		instrument_id_t instrument_id_ = INVALID_INSTRUMENT_ID;
		order_id_t order_id_ = INVALID_ORDER_ID;
		side_t side_ = side_t::INVALID;
		price_t price_ = INVALID_PRICE;
		quantity_t qty_ = INVALID_QUANTITY;

		auto to_string() const {
			std::stringstream ss;
			ss << "MEClientRequest"
				<< " ["
				<< "type:" << client_request_type_to_string(type_)
				<< " client:" << client_id_to_string(client_id_)
				<< " instrument:" << instrument_id_to_string(instrument_id_)
				<< " oid:" << order_id_to_string(order_id_)
				<< " side:" << side_to_string(side_)
				<< " qty:" << quantity_to_string(qty_)
				<< " price:" << price_to_string(price_)
				<< "]";
			return ss.str();
		}
	};

#pragma pack(pop)

	using client_request_queue = kse::utils::lock_free_queue<client_request>;
}
