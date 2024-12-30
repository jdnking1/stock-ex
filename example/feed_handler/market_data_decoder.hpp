#pragma once 

#include "utils/utils.hpp"
#include "models/market_update.hpp"



namespace kse::example::market_data {

	inline auto deserialize_market_update(char* buffer) -> models::client_market_update {
		models::client_market_update update;
		size_t offset = 0;

		uint64_t seq_number_be;
		std::memcpy(&seq_number_be, &buffer[offset], sizeof(seq_number_be));
		update.sequence_number_ = kse::utils::from_big_endian_64(seq_number_be);
		offset += sizeof(seq_number_be);

		std::memcpy(&update.update_.type_, &buffer[offset], sizeof(update.update_.type_));
		offset += sizeof(update.update_.type_);

		uint64_t order_id_be;
		std::memcpy(&order_id_be, &buffer[offset], sizeof(order_id_be));
		update.update_.order_id_ = kse::utils::from_big_endian_64(order_id_be);
		offset += sizeof(order_id_be);

		std::memcpy(&update.update_.instrument_id_, &buffer[offset], sizeof(update.update_.instrument_id_));
		offset += sizeof(update.update_.instrument_id_);

		std::memcpy(&update.update_.side_, &buffer[offset], sizeof(update.update_.side_));
		offset += sizeof(update.update_.side_);

		int64_t price_be;
		std::memcpy(&price_be, &buffer[offset], sizeof(price_be));
		update.update_.price_ = kse::utils::from_big_endian_64(price_be);
		offset += sizeof(price_be);

		uint32_t qty_be;
		std::memcpy(&qty_be, &buffer[offset], sizeof(qty_be));
		update.update_.qty_ = kse::utils::from_big_endian_32(qty_be);
		offset += sizeof(qty_be);

		uint64_t priority_be;
		std::memcpy(&priority_be, &buffer[offset], sizeof(priority_be));
		update.update_.priority_ = kse::utils::from_big_endian_64(priority_be);

		return update;
	}

}

