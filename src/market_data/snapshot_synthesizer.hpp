#pragma once

#include "uv.h"
#include "models/market_update.hpp"
#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include "utils/memory_pool.hpp"
#include <vector>
#include <cstdint>


namespace kse::market_data
{
	constexpr size_t BUFFER_SIZE = 64 * 1024 * 1024;

	auto process_update(uv_idle_t* handle [[maybe_unused]] ) -> void;
	auto publish(uv_timer_t* handle [[maybe_unused]] ) -> void;

	class snapshot_synthesizer
	{
	public:
		static snapshot_synthesizer& get_instance(
			models::client_market_update_queue* market_update_queue = nullptr,
			std::string_view ip = "",
			int port = 0)
		{
			static snapshot_synthesizer instance(market_update_queue, ip, port);
			return instance;
		}

		auto start() -> void {
			auto snapshot_synthesizer_thread = utils::create_thread(-1, [this]() { run(); });
			snapshot_synthesizer_thread.detach();
		}

		auto run() -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

			uv_loop_init(loop_);

			uv_udp_init(loop_, socket_);
	
			uv_udp_set_membership(socket_, ip_.c_str(), NULL, UV_JOIN_GROUP);

			uv_idle_init(loop_, idle_);
			uv_idle_start(idle_, process_update);

			uv_timer_init(loop_, timer_);
			uv_timer_start(timer_, publish, 0, 60000);

			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto get_buffer() -> std::vector<char>& { return buffer_; }
		auto get_logger() -> utils::logger& { return logger_; }
		
		auto send_data() -> void;
		auto add_to_buffer(const models::client_market_update& update) -> void;
		auto add_to_snapshot(models::client_market_update* update) -> void;
		auto publish_snapshot() -> void;
		auto process_client_market_update() -> void;

	private:
		std::string ip_;
		int port_;

		models::client_market_update_queue* market_update_queue_{ nullptr };

		utils::logger logger_;

		std::string time_str_;

		uv_loop_t* loop_{ nullptr };
		uv_udp_t* socket_{ nullptr };
		uv_idle_t* idle_{ nullptr };
		uv_timer_t* timer_{ nullptr };
		uv_udp_send_t* sender_{nullptr};

		std::vector<char> buffer_;
		size_t next_send_valid_index_{ 0 };

		std::vector<std::vector<models::market_update*>> updates_by_instrument_;
		uint64_t last_inc_seq_num_{ 0 };

		utils::memory_pool<models::market_update> market_update_pool_;

		snapshot_synthesizer(models::client_market_update_queue* market_update_queue, std::string_view ip, int port) : 
			ip_{ ip }, port_{ port }, market_update_queue_{ market_update_queue }, logger_{ "kse_snapshot_synthesizer.log" }, 
			loop_{(uv_loop_t*)std::malloc(sizeof(uv_loop_t))}, socket_{(uv_udp_t*)std::malloc(sizeof(uv_udp_t))}, 
			idle_{(uv_idle_t*)std::malloc(sizeof(uv_idle_t))}, timer_{(uv_timer_t*)std::malloc(sizeof(uv_timer_t))}, 
			sender_{(uv_udp_send_t*)std::malloc(sizeof(uv_udp_send_t))}, market_update_pool_{ models::MAX_NUM_ORDERS } {

			updates_by_instrument_.resize(models::MAX_NUM_INSTRUMENTS);
			buffer_.resize(BUFFER_SIZE);
			for(auto& snapshot : updates_by_instrument_) {
				snapshot.resize(models::MAX_NUM_ORDERS, nullptr);
			}

		};

		~snapshot_synthesizer() {
			if (idle_) {
				uv_idle_stop(idle_);
				uv_close(reinterpret_cast<uv_handle_t*>(idle_), [](uv_handle_t* handle) {
					std::free(handle);
					});
				idle_ = nullptr;
			}

			if(timer_) {
				uv_timer_stop(timer_);
				uv_close(reinterpret_cast<uv_handle_t*>(timer_), [](uv_handle_t* handle) {
					std::free(handle);
				});
				timer_ = nullptr;
			}

			if (socket_) {
				if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(socket_))) {
					uv_close(reinterpret_cast<uv_handle_t*>(socket_), [](uv_handle_t* handle) {
						std::free(handle);
					});
				}
				socket_ = nullptr;
			}

			if(sender_) {
				std::free(sender_);
				sender_ = nullptr;
			}

			std::free(sender_);

			if (loop_ && uv_loop_alive(loop_)) {
				uv_stop(loop_);
				uv_loop_close(loop_);
				std::free(loop_);
			}

			loop_ = nullptr;
		}

		snapshot_synthesizer(snapshot_synthesizer& other) = delete;
		snapshot_synthesizer(snapshot_synthesizer&& other) = delete;

		snapshot_synthesizer& operator=(snapshot_synthesizer& other) = delete;
		snapshot_synthesizer& operator=(snapshot_synthesizer&& other) = delete;

	};
}