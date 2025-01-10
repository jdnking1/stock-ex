#pragma once

#include <string>

#include "utils/logger.hpp"
#include "utils/utils.hpp"

#include "models/client_response.hpp"
#include "models/market_update.hpp"

namespace kse::engine {
	class message_handler {
	public:
		message_handler(models::client_response_queue* responses,
			models::market_update_queue* updates,
			utils::logger* logger)
			: outgoing_responses_(responses),
			outgoing_market_updates_(updates),
			logger_(logger) {
		}
		
		message_handler(const message_handler&) = delete;
		message_handler(message_handler&&) = delete;

		message_handler& operator=(const message_handler&) = delete;
		message_handler& operator=(message_handler&&) = delete;

		auto send_client_response(const models::client_response_internal& client_response) noexcept -> void {
			logger_->log("%:% %() % Sending %\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), client_response.to_string());
			auto* next_write = outgoing_responses_->get_next_write_element();
			*next_write = client_response;
			outgoing_responses_->next_write_index();
			TIME_MEASURE(T4t_MatchingEngine_LFQueue_write, (*logger_), time_str_);
		}

		auto send_market_update(const models::market_update& market_update) noexcept -> void {
			logger_->log("%:% %() % Sending %\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), market_update.to_string());
			auto* next_write = outgoing_market_updates_->get_next_write_element();
			*next_write = market_update;
			outgoing_market_updates_->next_write_index();
			TIME_MEASURE(T4_MatchingEngine_LFQueue_write, (*logger_), time_str_);
		}

	private:
		models::client_response_queue* outgoing_responses_ = nullptr;
		models::market_update_queue* outgoing_market_updates_ = nullptr;
		utils::logger* logger_ = nullptr;
		std::string time_str_;
	};
}