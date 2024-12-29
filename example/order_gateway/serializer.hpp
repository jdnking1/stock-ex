#pragma once 

#include "models/client_response.hpp"
#include "models/client_request.hpp"
#include "utils/utils.hpp"



namespace kse::example::gateway {
	inline void serialize_client_request(
		const models::client_request_external& request,
		char* buffer) {
		size_t offset = 0;

		uint64_t seq_number_be = kse::utils::to_big_endian_64(request.sequence_number_);
		std::memcpy(&buffer[offset], &seq_number_be, sizeof(seq_number_be));
		offset += sizeof(seq_number_be);

		std::memcpy(&buffer[offset], &request.request_.type_, sizeof(request.request_.type_));
		offset += sizeof(request.request_.type_);

		uint32_t client_id_be = kse::utils::to_big_endian_32(request.request_.client_id_);
		std::memcpy(&buffer[offset], &client_id_be, sizeof(client_id_be));
		offset += sizeof(client_id_be);

		std::memcpy(&buffer[offset], &request.request_.instrument_id_, sizeof(request.request_.instrument_id_));
		offset += sizeof(request.request_.instrument_id_);

		uint64_t order_id_be = kse::utils::to_big_endian_64(request.request_.order_id_);
		std::memcpy(&buffer[offset], &order_id_be, sizeof(order_id_be));
		offset += sizeof(order_id_be);

		std::memcpy(&buffer[offset], &request.request_.side_, sizeof(request.request_.side_));
		offset += sizeof(request.request_.side_);

		int64_t price_be = kse::utils::to_big_endian_64(request.request_.price_);
		std::memcpy(&buffer[offset], &price_be, sizeof(price_be));
		offset += sizeof(price_be);

		uint32_t qty_be = kse::utils::to_big_endian_32(request.request_.qty_);
		std::memcpy(&buffer[offset], &qty_be, sizeof(qty_be));
	}

	inline models::client_request_external deserialize_client_request(char* buffer) {
		models::client_request_external request;
		size_t offset = 0;

		uint64_t seq_number_be;
		std::memcpy(&seq_number_be, &buffer[offset], sizeof(seq_number_be));
		request.sequence_number_ = kse::utils::from_big_endian_64(seq_number_be);
		offset += sizeof(seq_number_be);

		std::memcpy(&request.request_.type_, &buffer[offset], sizeof(request.request_.type_));
		offset += sizeof(request.request_.type_);

		uint32_t client_id_be;
		std::memcpy(&client_id_be, &buffer[offset], sizeof(client_id_be));
		request.request_.client_id_ = kse::utils::from_big_endian_32(client_id_be);
		offset += sizeof(client_id_be);

		std::memcpy(&request.request_.instrument_id_, &buffer[offset], sizeof(request.request_.instrument_id_));
		offset += sizeof(request.request_.instrument_id_);

		uint64_t order_id_be;
		std::memcpy(&order_id_be, &buffer[offset], sizeof(order_id_be));
		request.request_.order_id_ = kse::utils::from_big_endian_64(order_id_be);
		offset += sizeof(order_id_be);

		std::memcpy(&request.request_.side_, &buffer[offset], sizeof(request.request_.side_));
		offset += sizeof(request.request_.side_);

		int64_t price_be;
		std::memcpy(&price_be, &buffer[offset], sizeof(price_be));
		request.request_.price_ = kse::utils::from_big_endian_64(price_be);
		offset += sizeof(price_be);

		uint32_t qty_be;
		std::memcpy(&qty_be, &buffer[offset], sizeof(qty_be));
		request.request_.qty_ = kse::utils::from_big_endian_32(qty_be);

		return request;
	}


	inline void serialize_client_response(
		const models::client_response_external& response,
		char* buffer) {
		size_t offset = 0;

		uint64_t seq_number_be = kse::utils::to_big_endian_64(response.sequence_number_);
		std::memcpy(&buffer[offset], &seq_number_be, sizeof(seq_number_be));
		offset += sizeof(seq_number_be);

		std::memcpy(&buffer[offset], &response.response_.type_, sizeof(response.response_.type_));
		offset += sizeof(response.response_.type_);

		uint32_t client_id_be = kse::utils::to_big_endian_32(response.response_.client_id_);
		std::memcpy(&buffer[offset], &client_id_be, sizeof(client_id_be));
		offset += sizeof(client_id_be);

		std::memcpy(&buffer[offset], &response.response_.instrument_id_, sizeof(response.response_.instrument_id_));
		offset += sizeof(response.response_.instrument_id_);

		uint64_t client_order_id_be = kse::utils::to_big_endian_64(response.response_.client_order_id_);
		std::memcpy(&buffer[offset], &client_order_id_be, sizeof(client_order_id_be));
		offset += sizeof(client_order_id_be);

		uint64_t market_order_id_be = kse::utils::to_big_endian_64(response.response_.market_order_id_);
		std::memcpy(&buffer[offset], &market_order_id_be, sizeof(market_order_id_be));
		offset += sizeof(market_order_id_be);

		std::memcpy(&buffer[offset], &response.response_.side_, sizeof(response.response_.side_));
		offset += sizeof(response.response_.side_);

		int64_t price_be = kse::utils::to_big_endian_64(response.response_.price_);
		std::memcpy(&buffer[offset], &price_be, sizeof(price_be));
		offset += sizeof(price_be);

		uint32_t exec_qty_be = kse::utils::to_big_endian_32(response.response_.exec_qty_);
		std::memcpy(&buffer[offset], &exec_qty_be, sizeof(exec_qty_be));
		offset += sizeof(exec_qty_be);

		uint32_t leaves_qty_be = kse::utils::to_big_endian_32(response.response_.leaves_qty_);
		std::memcpy(&buffer[offset], &leaves_qty_be, sizeof(leaves_qty_be));
	}



	inline models::client_response_external deserialize_client_response(const char* buffer) {
		models::client_response_external response;
		size_t offset = 0;

		uint64_t seq_number_be;
		std::memcpy(&seq_number_be, &buffer[offset], sizeof(seq_number_be));
		response.sequence_number_ = kse::utils::from_big_endian_64(seq_number_be);
		offset += sizeof(seq_number_be);

		std::memcpy(&response.response_.type_, &buffer[offset], sizeof(response.response_.type_));
		offset += sizeof(response.response_.type_);

		uint32_t client_id_be;
		std::memcpy(&client_id_be, &buffer[offset], sizeof(client_id_be));
		response.response_.client_id_ = kse::utils::from_big_endian_32(client_id_be);
		offset += sizeof(client_id_be);

		std::memcpy(&response.response_.instrument_id_, &buffer[offset], sizeof(response.response_.instrument_id_));
		offset += sizeof(response.response_.instrument_id_);

		uint64_t client_order_id_be;
		std::memcpy(&client_order_id_be, &buffer[offset], sizeof(client_order_id_be));
		response.response_.client_order_id_ = kse::utils::from_big_endian_64(client_order_id_be);
		offset += sizeof(client_order_id_be);

		uint64_t market_order_id_be;
		std::memcpy(&market_order_id_be, &buffer[offset], sizeof(market_order_id_be));
		response.response_.market_order_id_ = kse::utils::from_big_endian_64(market_order_id_be);
		offset += sizeof(market_order_id_be);

		std::memcpy(&response.response_.side_, &buffer[offset], sizeof(response.response_.side_));
		offset += sizeof(response.response_.side_);

		int64_t price_be;
		std::memcpy(&price_be, &buffer[offset], sizeof(price_be));
		response.response_.price_ = kse::utils::from_big_endian_64(price_be);
		offset += sizeof(price_be);

		uint32_t exec_qty_be;
		std::memcpy(&exec_qty_be, &buffer[offset], sizeof(exec_qty_be));
		response.response_.exec_qty_ = kse::utils::from_big_endian_32(exec_qty_be);
		offset += sizeof(exec_qty_be);

		uint32_t leaves_qty_be;
		std::memcpy(&leaves_qty_be, &buffer[offset], sizeof(leaves_qty_be));
		response.response_.leaves_qty_ = kse::utils::from_big_endian_32(leaves_qty_be);

		return response;
	}
}