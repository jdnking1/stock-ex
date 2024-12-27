#pragma once 

#include "models/market_update.hpp"
#include "utils/utils.hpp"



namespace kse::market_data {
    inline void serialize_client_market_update(const kse::models::client_market_update& update, char* buffer) {
        size_t offset = 0;

        uint64_t seq_number_be = kse::utils::to_big_endian_64(update.sequence_number_);
        std::memcpy(&buffer[offset], &seq_number_be, sizeof(seq_number_be));
        offset += sizeof(seq_number_be);

        uint8_t type = static_cast<uint8_t>(update.update_.type_);
        std::memcpy(&buffer[offset], &type, sizeof(type));
        offset += sizeof(type);

        uint64_t order_id_be = kse::utils::to_big_endian_64(update.update_.order_id_);
        std::memcpy(&buffer[offset], &order_id_be, sizeof(order_id_be));
        offset += sizeof(order_id_be);

        uint8_t instrument_id = update.update_.instrument_id_;
        std::memcpy(&buffer[offset], &instrument_id, sizeof(instrument_id));
        offset += sizeof(instrument_id);

        uint8_t side = static_cast<uint8_t>(update.update_.side_);
        std::memcpy(&buffer[offset], &side, sizeof(side));
        offset += sizeof(side);

        int64_t price_be = kse::utils::to_big_endian_64(update.update_.price_);
        std::memcpy(&buffer[offset], &price_be, sizeof(price_be));
        offset += sizeof(price_be);

        uint32_t qty_be = kse::utils::to_big_endian_32(update.update_.qty_);
        std::memcpy(&buffer[offset], &qty_be, sizeof(qty_be));
        offset += sizeof(qty_be);

        uint64_t priority_be = kse::utils::to_big_endian_64(update.update_.priority_);
        std::memcpy(&buffer[offset], &priority_be, sizeof(priority_be));
        offset += sizeof(priority_be);
    }
}