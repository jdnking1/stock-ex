#pragma once


#include "uv.h"
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include "models/market_update.hpp"

namespace kse::example::market_data {
	constexpr size_t BUFFER_SIZE = 64 * 1024 * 1024;

	struct multicast_connection_t {
		uv_udp_t* handle_ = nullptr;
		std::vector<char> buffer;
		size_t offset = 0;

		explicit multicast_connection_t() : handle_{ (uv_udp_t*)std::malloc(sizeof(uv_udp_t)) } {
			buffer.resize(BUFFER_SIZE);
		}

		~multicast_connection_t() noexcept {
			if (handle_) {
				if (!uv_is_closing((uv_handle_t*)handle_)) {
					uv_close((uv_handle_t*)handle_, nullptr);
				}
				std::free(handle_);
				handle_ = nullptr;
			}
		}

		void shift_inbound_buffer(size_t processed_bytes) {
			if (processed_bytes > 0 && processed_bytes <= offset) {
				std::memmove(buffer.data(), buffer.data() + processed_bytes, offset - processed_bytes);
				offset -= processed_bytes;
			}
		}
	};

	inline auto alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void;
	inline auto on_read(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf [[maybe_unused]], const sockaddr* addr [[maybe_unused]], unsigned flags [[maybe_unused]] ) -> void;

	class feed_handler
	{
	public:
		static feed_handler& get_instance(models::market_update_queue* incoming_updates = nullptr, std::string_view snapshot_ip = "", int snapshot_port = 0, std::string_view incremental_ip = "", int incremental_port = 0) {
			static feed_handler instance{ incoming_updates, snapshot_ip, snapshot_port, incremental_ip, incremental_port };
			return instance;
		}

		auto start() -> void {
			auto feed_handler_thread = utils::create_thread(-1, [this]() { run(); });
			utils::ASSERT(feed_handler_thread.joinable(), "feed handler thread is not joinable");
			feed_handler_thread.detach();
		}

		auto run() -> void {
			loop_ = (uv_loop_t*)std::malloc(sizeof(uv_loop_t));
			uv_loop_init(loop_);
			
			uv_udp_init(loop_, incremental_feed_->handle_);
			struct sockaddr_in addr;
			uv_ip4_addr("0.0.0.0", incremental_multicast_port_, &addr);
			uv_udp_bind(incremental_feed_->handle_, (const struct sockaddr*)&addr, UV_UDP_REUSEADDR);
			uv_udp_set_membership(incremental_feed_->handle_, incremental_multicast_ip_.c_str(), NULL, UV_JOIN_GROUP);
			incremental_feed_->handle_->data = incremental_feed_.get();

			uv_udp_recv_start(incremental_feed_->handle_, alloc_buffer, on_read);

			uv_udp_init(loop_, snapshot_feed_->handle_);
			uv_ip4_addr("0.0.0.0", snapshot_multicast_port_, &addr);
			uv_udp_bind(snapshot_feed_->handle_, (const struct sockaddr*)&addr, UV_UDP_REUSEADDR);
			uv_udp_set_membership(snapshot_feed_->handle_, snapshot_multicast_ip_.c_str(), NULL, UV_JOIN_GROUP);
			snapshot_feed_->handle_->data = snapshot_feed_.get();

			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto sync_snapshot_with_incremental() -> void;
		auto queue_message(bool is_snapshot, const models::client_market_update upd) -> void;
		auto read_data(multicast_connection_t* conn) -> void;
		auto get_logger() -> utils::logger& { return logger_; }
	private:
		uint64_t next_expected_seq_ = 1;
		models::market_update_queue* incoming_updates_ = nullptr;

		std::string time_str_;
		utils::logger logger_;

		uv_loop_t* loop_ = nullptr;
		std::unique_ptr<multicast_connection_t> incremental_feed_ = nullptr;
		std::unique_ptr<multicast_connection_t> snapshot_feed_ = nullptr;

		bool in_recovery_ = false;
		std::string snapshot_multicast_ip_;
		int snapshot_multicast_port_ = 0;
		std::string incremental_multicast_ip_;
		int incremental_multicast_port_ = 0;

		using queued_market_updates = std::map<uint64_t, models::market_update>;
		queued_market_updates snapshot_queued_msgs_;
		queued_market_updates incremental_queued_msgs_;

		feed_handler(models::market_update_queue* incoming_updates, std::string_view snapshot_ip, int snapshot_port, std::string_view incremental_ip, int incremental_port) : incoming_updates_{ incoming_updates }, logger_{"feed_handler" + std::to_string((uintptr_t)this)}, snapshot_multicast_ip_{snapshot_ip}, snapshot_multicast_port_{snapshot_port}, incremental_multicast_ip_{incremental_ip}, incremental_multicast_port_{incremental_port} {
			snapshot_feed_ = std::make_unique<multicast_connection_t>();
			incremental_feed_ = std::make_unique<multicast_connection_t>();
		}

		~feed_handler() noexcept {
			if (loop_) {
				uv_loop_close(loop_);
				std::free(loop_);
				loop_ = nullptr;
			}
		}

		feed_handler(const feed_handler&) = delete;
		feed_handler& operator=(const feed_handler&) = delete;

		feed_handler(feed_handler&&) = delete;
		feed_handler& operator=(feed_handler&&) = delete;
	};
}