#pragma once

#include <string>

#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "models/market_update.hpp"

#include "utils/utils.hpp"
#include "utils/logger.hpp"

#include "order_book.hpp"
#include "message_handler.hpp"


namespace kse::engine {
	class matching_engine {
	public:
		matching_engine(models::client_request_queue* client_requests, models::client_response_queue* client_responses, models::market_update_queue* market_updates);
		~matching_engine();

		matching_engine(const matching_engine&) = delete;
		matching_engine(matching_engine&&) = delete;

		matching_engine& operator=(const matching_engine&) = delete;
		matching_engine& operator=(matching_engine&&) = delete;

		auto start() -> void;
		auto stop() -> void;

		auto process_client_request(const models::client_request& client_request) noexcept -> void {
			auto* order_book = instrument_order_books_.at(client_request.instrument_id_).get();


			switch (client_request.type_) {
				case models::client_request_type::NEW: {
					order_book->add(client_request.client_id_, client_request.order_id_, client_request.side_, client_request.price_, client_request.qty_);
				} break;
				case models::client_request_type::CANCEL: {
					order_book->cancel(client_request.client_id_, client_request.order_id_);
				} break;
				case models::client_request_type::MODIFY: {
					order_book->modify(client_request.client_id_, client_request.order_id_, client_request.price_, client_request.qty_);
				}break;
				default: {
					utils::FATAL("Received invalid client-request-type:" + models::client_request_type_to_string(client_request.type_));
				} 
			}
		}

		auto send_client_response(const models::client_response& client_response) noexcept -> void {
			message_handler_.send_client_response(client_response);
		}

		auto send_market_update(const models::market_update& market_update) noexcept -> void {
			message_handler_.send_market_update(market_update);
		}

		auto run() noexcept -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
			while (running_) {
				const auto* client_request = incoming_requests_->get_next_read_element();
				if (client_request) [[likely]] {
					logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
						client_request->to_string());
					process_client_request(*client_request);
					incoming_requests_->next_read_index();
				}
			}
		}

	private:
		order_book_map instrument_order_books_;

		models::client_request_queue* incoming_requests_ = nullptr;
		models::client_response_queue* outgoing_responses_ = nullptr;
		models::market_update_queue* outgoing_market_updates_ = nullptr;

		volatile bool running_ = true;

		std::string time_str_;
		utils::logger logger_;

		message_handler message_handler_;
	};
}