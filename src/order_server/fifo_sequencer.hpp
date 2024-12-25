#pragma once

#include <algorithm>

namespace kse::server
{
	constexpr size_t MAX_PENDING_REQUESTS = 1024;

	class fifo_sequencer
	{
	public:
		fifo_sequencer(models::client_request_queue* incoming_messsages, utils::logger* logger): 
			incoming_requests_{ incoming_messsages }, logger_{ logger } {};
		fifo_sequencer(fifo_sequencer const&) = delete;
		fifo_sequencer(fifo_sequencer&&) = delete;
		fifo_sequencer& operator=(fifo_sequencer const&) = delete;
		fifo_sequencer& operator=(fifo_sequencer&&) = delete;


		auto add_request(utils::nananoseconds_t rx_time, const models::client_request_internal& request) -> void {
			if (pending_size_ >= pending_client_requests_.size()) {
				utils::FATAL("Too many pending requests");
			}
			pending_client_requests_.at(pending_size_++) = timed_client_request{ rx_time, request };
		}

		auto sequence_and_publish() -> void {
			if (!pending_size_) [[unlikely]]
				return;

			logger_->log("%:% %() % Processing % requests.\n", __FILE__, __LINE__, __FUNCTION__, utils::get_curren_time_str(&time_str_), pending_size_);

			
			std::sort(pending_client_requests_.begin(), pending_client_requests_.begin() + pending_size_);

			for (size_t i = 0; i < pending_size_; ++i) {
				const auto& client_request = pending_client_requests_.at(i);

				logger_->log("%:% %() % Writing RX:% Req:% to FIFO.\n", __FILE__, __LINE__, __FUNCTION__, utils::get_curren_time_str(&time_str_),
					client_request.recv_time_, client_request.request_.to_string());

				auto next_write = incoming_requests_->get_next_write_element();
				*next_write = client_request.request_;
				incoming_requests_->next_write_index();
			}

			pending_size_ = 0;
		}
	private:
		models::client_request_queue* incoming_requests_ = nullptr;

		std::string time_str_;
		utils::logger* logger_ = nullptr;

		struct timed_client_request {
			utils::nananoseconds_t recv_time_ = 0;
			models::client_request_internal request_;

			auto operator<(const timed_client_request& rhs)  {
				return recv_time_ < rhs.recv_time_;
			}
		};

		std::array<timed_client_request, MAX_PENDING_REQUESTS> pending_client_requests_;
		size_t pending_size_ = 0;
	};

}