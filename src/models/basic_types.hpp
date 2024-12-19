#pragma once

#include <cstdint>
#include <limits>
#include <string>

namespace kse::models {
	using order_id_t = uint64_t;
	constexpr auto INVALID_ORDER_ID = std::numeric_limits<order_id_t>::max();
	inline auto order_id_to_string(order_id_t order_id) -> std::string {
		if (order_id == INVALID_ORDER_ID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(order_id);
	}

	using instrument_id_t = uint8_t;
	constexpr auto INVALID_INSTRUMENT_ID = std::numeric_limits<instrument_id_t>::max();
	inline auto instrument_id_to_string(instrument_id_t instrument_id) -> std::string {
		if (instrument_id == INVALID_INSTRUMENT_ID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(instrument_id);
	}

	using client_id_t = uint32_t;
	constexpr auto INVALID_CLIENT_ID = std::numeric_limits<client_id_t>::max();
	inline auto client_id_to_string(client_id_t client_id) -> std::string {
		if (client_id == INVALID_CLIENT_ID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(client_id);
	}

	using price_t = int64_t;
	constexpr auto INVALID_PRICE = std::numeric_limits<price_t>::max();
	inline auto price_to_string(price_t price) -> std::string {
		if (price == INVALID_PRICE) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(price);
	}

	using quantity_t = uint32_t;
	constexpr auto INVALID_QUANTITY = std::numeric_limits<quantity_t>::max();
	inline auto quantity_to_string(quantity_t quantity) -> std::string {
		if (quantity == INVALID_QUANTITY) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(quantity);
	}
	
	using priority_t = uint64_t;
	constexpr auto INVALID_PRIORITY = std::numeric_limits<priority_t>::max();
	inline auto priority_to_string(priority_t priority) -> std::string {
		if (priority == INVALID_PRIORITY) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(priority);
	}

	enum class side_t : int8_t {
		INVALID = 0,
		BUY,
		SELL 
	};

	inline auto side_to_string(side_t side) -> std::string {
		switch (side) {
		case side_t::BUY:
			return "BUY";
		case side_t::SELL:
			return "SELL";
		case side_t::INVALID:
			return "INVALID";
		}

		return "UNKNOWN";
	}
}