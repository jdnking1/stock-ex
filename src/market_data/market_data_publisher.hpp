#pragma once

#include <uv.h>

#include "snapshot_synthesizer.hpp"

#include "models/market_update.hpp"

#include <cstdint>


namespace kse::market_data
{
	auto process_incremental_update(uv_idle_t* handle [[maybe_unused]] ) -> void;

	class market_data_publisher
	{
	public:
		static market_data_publisher& get_instance(models::market_update_queue* market_updates = nullptr, const std::string& snapshot_ip = "", int snapshot_port = 0, const std::string& incremental_ip = "", int incremental_port = 0) {
			static market_data_publisher instance(market_updates, snapshot_ip, snapshot_port, incremental_ip, incremental_port);
			return instance;
		}

		auto start() -> void {
			auto market_data_publisher_thread = utils::create_thread(-1, [this]() { run(); });
			snapshot_synthesizer_->start();
			market_data_publisher_thread.detach();
		}

		auto process_and_publish() -> void;
	private:
		uint64_t next_inc_seq_num_ = 1;
		std::string ip_;
		int port_;

		models::market_update_queue* outgoing_market_update_queue_{ nullptr };
		models::client_market_update_queue snapshot_market_update_queue_;

		utils::logger logger_;

		std::string time_str_;

		uv_loop_t* loop_{ nullptr };
		uv_udp_t* socket_{ nullptr };
		uv_idle_t* idle_{ nullptr };
		uv_udp_send_t* sender_{ nullptr };

		std::vector<char> buffer_;
		size_t next_send_valid_index_{ 0 };

		snapshot_synthesizer* snapshot_synthesizer_{ nullptr };

		market_data_publisher(models::market_update_queue* market_updates,
			                                         const std::string& snapshot_ip, int snapshot_port,
			                                         const std::string& incremental_ip, int incremental_port)
			: ip_{ incremental_ip }, port_{ incremental_port }, outgoing_market_update_queue_(market_updates), snapshot_market_update_queue_{ models::MAX_MARKET_UPDATES },
			logger_{ "market_data_publisher.log" }, loop_{(uv_loop_t*)std::malloc(sizeof(uv_loop_t))}, socket_{(uv_udp_t*)std::malloc(sizeof(uv_udp_t))}, 
			idle_{(uv_idle_t*)std::malloc(sizeof(uv_idle_t))}, sender_{(uv_udp_send_t*)std::malloc(sizeof(uv_udp_send_t))} {
			buffer_.resize(BUFFER_SIZE);
			snapshot_synthesizer_ = &snapshot_synthesizer::get_instance(&snapshot_market_update_queue_, snapshot_ip, snapshot_port);
		}

		~market_data_publisher() {
			if (idle_) {
				uv_idle_stop(idle_);
				uv_close(reinterpret_cast<uv_handle_t*>(idle_), [](uv_handle_t* handle) {
					std::free(handle);
					});
				idle_ = nullptr;
			}

			if (socket_) {
				if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(socket_))) {
					uv_close(reinterpret_cast<uv_handle_t*>(socket_), [](uv_handle_t* handle) {
						std::free(handle);
						});
				}
				socket_ = nullptr;
			}

			if (sender_) {
				std::free(sender_);
				sender_ = nullptr;
			}

			if (loop_ && uv_loop_alive(loop_)) {
				uv_stop(loop_);
				uv_loop_close(loop_);
				std::free(loop_);
				loop_ = nullptr;
			}
		}

		market_data_publisher(market_data_publisher& other) = delete;
		market_data_publisher(market_data_publisher&& other) = delete;
		market_data_publisher& operator=(market_data_publisher& other) = delete;
		market_data_publisher& operator=(market_data_publisher&& other) = delete;

		auto run() -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

			uv_loop_init(loop_);
			uv_udp_init(loop_, socket_);
			uv_udp_set_membership(socket_, ip_.c_str(), NULL, UV_JOIN_GROUP);
			uv_idle_init(loop_, idle_);
			uv_idle_start(idle_, process_incremental_update);

			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto send_data() -> void;
		auto add_to_buffer(const models::client_market_update& update) -> void;
	};
}