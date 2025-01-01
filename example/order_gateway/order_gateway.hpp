#pragma once 

#include "uv.h"
#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include "serializer.hpp"


namespace kse::example::gateway {
	constexpr size_t TCP_BUFFER_SIZE = 64 * 1024 * 1024;

	struct tcp_connection_t {
		uv_tcp_t* handle_ = nullptr;
		std::vector<char> outbound_data_;
		size_t next_send_valid_index_ = 0;
		std::vector<char> inbound_data_;
		size_t next_rcv_valid_index_ = 0;

		explicit tcp_connection_t() : handle_{(uv_tcp_t*)std::malloc(sizeof(uv_tcp_t))} {
			outbound_data_.resize(TCP_BUFFER_SIZE);
			inbound_data_.resize(TCP_BUFFER_SIZE);
		}

		~tcp_connection_t() noexcept {
			if (handle_) {
				if (!uv_is_closing((uv_handle_t*)handle_)) {
					uv_close((uv_handle_t*)handle_, nullptr);
				}
				std::free(handle_);
				handle_ = nullptr;
			}
		}

		auto append_to_outbound_buffer(models::client_request_internal& request, uint64_t sequence_number) -> bool {
			serialize_client_request({sequence_number, std::move(request)}, outbound_data_.data() + next_send_valid_index_);
			next_send_valid_index_ += sizeof(models::client_request_external);
			return outbound_data_.size() - next_send_valid_index_ >= sizeof(models::client_request_external);
		}

		auto shift_inbound_buffer(size_t processed_bytes) -> void {
			if (processed_bytes > 0 && processed_bytes <= next_rcv_valid_index_) {
				std::memmove(inbound_data_.data(), inbound_data_.data() + processed_bytes, next_rcv_valid_index_ - processed_bytes);
				next_rcv_valid_index_ -= processed_bytes;
			}
		}
	};

	auto on_connect(uv_connect_t* req, int status) -> void;
	auto alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void;
	auto on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf [[maybe_unused]] ) -> void;
	auto on_idle(uv_idle_t* req [[maybe_unused]] ) -> void;

	class order_gateway {
	public:
		static order_gateway& get_instance(
			models::client_request_queue* outgoing_requests = nullptr,
			models::client_response_queue* incoming_responses = nullptr,
			std::string_view ip = "",
			int port = 0)
		{
			static order_gateway instance(outgoing_requests, incoming_responses, ip, port);
			return instance;
		}

		order_gateway(order_gateway const&) = delete;
		order_gateway& operator=(order_gateway const&) = delete;

		order_gateway(order_gateway&&) = delete;
		order_gateway& operator=(order_gateway&&) = delete;

		auto get_logger() -> utils::logger& { return logger_; }
		auto get_time_str() -> std::string& { return time_str_; }
		auto get_client_id() const -> models::client_id_t { return client_id_; }

		auto start() -> void {
			auto order_gateway_thread = utils::create_thread(-1, [this]() { run(); });
			utils::ASSERT(order_gateway_thread.joinable(), "failed to create order_gateway thread");
			order_gateway_thread.detach();
		}

		auto run() -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

			loop_ = uv_default_loop();
			uv_tcp_init(loop_, connection_->handle_);

			connection_->handle_->data = connection_.get();

			struct sockaddr_in addr;
			uv_ip4_addr(ip_.c_str(), port_, &addr);

			uv_connect_t* connect_req = (uv_connect_t*)std::malloc(sizeof(uv_connect_t));

			uv_tcp_connect(connect_req, connection_->handle_, (const struct sockaddr*)&addr, on_connect);

			uv_idle_init(loop_, idle_);
			uv_idle_start(idle_, on_idle);

			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto read_data() -> void;
		auto send_request() -> void;
	private:

		std::string ip_;
		int port_;
		models::client_id_t client_id_ = models::INVALID_CLIENT_ID;

		models::client_response_queue* incoming_responses_ = nullptr;
		models::client_request_queue* outgoing_requests_ = nullptr;

		std::string time_str_;
		utils::logger logger_;

		uint64_t next_outgoing_seq_num_ = 1;
		uint64_t next_exp_seq_num_ = 1;

		std::unique_ptr<tcp_connection_t> connection_;
		uv_loop_t* loop_{ nullptr };
		uv_idle_t* idle_{ nullptr };
		order_gateway(
			models::client_request_queue* outgoing_requests,
			models::client_response_queue* incoming_responses,
			std::string_view ip,
			int port)
			:ip_{ ip }, port_{ port }, incoming_responses_{ incoming_responses }, outgoing_requests_{ outgoing_requests }, 
			logger_{ "order_gateway" + std::to_string((uintptr_t)outgoing_requests) + ".log" }, connection_{ std::make_unique<tcp_connection_t>() }, idle_{ (uv_idle_t*)std::malloc(sizeof(uv_idle_t)) } {
		}

		~order_gateway()
		{
			if (idle_) {
				uv_idle_stop(idle_);
				uv_close(reinterpret_cast<uv_handle_t*>(idle_), [](uv_handle_t* handle) {
					std::free(handle);
					});
				idle_ = nullptr;
			}

		     connection_.reset();


			if (loop_ && uv_loop_alive(loop_)) {
				uv_stop(loop_);
				uv_loop_close(loop_);
			}

			loop_ = nullptr;
		}
	};

}