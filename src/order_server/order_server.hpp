#pragma once 

#include "uv.h"
#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include <array>
#include <string>
#include <string_view>
#include <stdlib.h>
#include <vector>

#include "fifo_sequencer.hpp"
#include "serializer.hpp"


namespace kse::server {
	constexpr size_t TCP_BUFFER_SIZE = 64 * 1024 * 1024;

	struct tcp_connection_t {
		uv_tcp_t* handle_ = nullptr;
		uv_write_t* writer_ = nullptr;
		std::vector<char> outbound_data_;
		size_t next_send_valid_index_ = 0;
		std::vector<char> inbound_data_;
		size_t next_rcv_valid_index_ = 0;

		explicit tcp_connection_t() : handle_{(uv_tcp_t*)std::malloc(sizeof(uv_tcp_t))}, writer_{ (uv_write_t*)std::malloc(sizeof(uv_write_t))} {
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

			if (writer_) {
				std::free(writer_);
				writer_ = nullptr;
			}
		}

		void append_to_outbound_buffer(models::client_response_internal& response, uint64_t sequence_number) {
			serialize_client_response({sequence_number, std::move(response)}, outbound_data_.data() + next_send_valid_index_);
			next_send_valid_index_ += sizeof(models::client_response_external);
		}

		void shift_inbound_buffer(size_t processed_bytes) {
			if (processed_bytes > 0 && processed_bytes <= next_rcv_valid_index_) {
				std::memmove(inbound_data_.data(), inbound_data_.data() + processed_bytes, next_rcv_valid_index_ - processed_bytes);
				next_rcv_valid_index_ -= processed_bytes;
			}
		}
	};

	auto on_new_connection(uv_stream_t* server, int status) -> void;

	auto alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void;

	auto on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf [[maybe_unused]] ) -> void;

	auto on_idle(uv_idle_t* req [[maybe_unused]] ) -> void;

	auto on_check(uv_check_t* req [[maybe_unused]] ) -> void;

	class order_server {
	public:
		static order_server& get_instance(
			models::client_request_queue* incoming_messages = nullptr,
			models::client_response_queue* outgoing_messages = nullptr,
			std::string_view ip = "",
			int port = 0)
		{
			static order_server instance(incoming_messages, outgoing_messages, ip, port);
			return instance;
		}

		order_server(order_server const&) = delete;
		order_server& operator=(order_server const&) = delete;

		order_server(order_server&&) = delete;
		order_server& operator=(order_server&&) = delete;

		auto start() -> void {
			auto order_server_thread = utils::create_thread(0, [this]() { run(); });
			order_server_thread.detach();
		}

		auto run() -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

			loop_ = uv_default_loop();
			uv_tcp_init(loop_, server_);

			struct sockaddr_in addr;
			uv_ip4_addr(ip_.c_str(), port_, &addr);
			uv_tcp_bind(server_, (const struct sockaddr*)&addr, 0);

			uv_listen((uv_stream_t*)server_, SOMAXCONN, on_new_connection);

			uv_idle_init(loop_, idle_);
			uv_idle_start(idle_, on_idle);

			uv_check_init(loop_, check_);
			uv_check_start(check_, on_check);

			std::cout << "Order Server listening on " << ip_ << ":" << port_ << "...\n";
			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto send_invalid_response(models::client_id_t client_id) -> void {
			auto* response = server_responses_.get_next_write_element();
			*response = models::client_response_internal{
				models::client_response_type::INVALID_REQUEST, client_id,
				models::INVALID_INSTRUMENT_ID, models::INVALID_ORDER_ID,
				models::INVALID_ORDER_ID, models::side_t::INVALID,
				models::INVALID_PRICE, models::INVALID_QUANTITY,
				models::INVALID_QUANTITY
			};
			server_responses_.next_write_index();
		}

		auto handle_new_connection(uv_stream_t* server, int status) -> void;

		auto read_data(tcp_connection_t* conn, utils::nananoseconds_t user_time) -> void;

		std::string ip_;
		int port_;
		models::client_id_t next_client_id_ = 0;
		models::client_id_t last_client_id_connected_ = 0;

		models::client_response_queue* matching_engine_responses_;
		models::client_response_queue server_responses_;

		std::string time_str_;
		utils::logger logger_;

		std::array<models::client_id_t, models::MAX_NUM_CLIENTS> client_next_outgoing_seq_num_;
		std::array<models::client_id_t, models::MAX_NUM_CLIENTS> client_next_incoming_seq_num_;
		std::array<std::unique_ptr<tcp_connection_t>, models::MAX_NUM_CLIENTS> client_connections_;

		uv_loop_t* loop_ {nullptr};
		uv_tcp_t* server_{ nullptr };
		uv_idle_t* idle_ {nullptr};
		uv_check_t* check_ {nullptr};

		fifo_sequencer fifo_sequencer_;

	private:
		order_server(
			models::client_request_queue* incoming_messages,
			models::client_response_queue* outgoing_messages,
			std::string_view ip,
			int port)
			:ip_{ ip }, port_{ port }, matching_engine_responses_{ outgoing_messages }, server_responses_{ MAX_PENDING_REQUESTS }, 
			logger_("exchange_order_server.log"), server_{ (uv_tcp_t*)std::malloc(sizeof(uv_tcp_t)) }, idle_{ (uv_idle_t*)std::malloc(sizeof(uv_idle_t)) }, 
			check_{ (uv_check_t*)std::malloc(sizeof(uv_check_t)) }, fifo_sequencer_{ incoming_messages, &logger_ } {
			client_next_incoming_seq_num_.fill(1);
			client_next_outgoing_seq_num_.fill(1);
		}

		~order_server()
		{
			if (idle_) {
				uv_idle_stop(idle_);
				uv_close(reinterpret_cast<uv_handle_t*>(idle_), [](uv_handle_t* handle) {
					std::free(handle);
					});
				idle_ = nullptr;
			}

			if (check_) {
				uv_check_stop(check_);
				uv_close(reinterpret_cast<uv_handle_t*>(check_), [](uv_handle_t* handle) {
					std::free(handle);
					});
				check_ = nullptr;
			}

			if (server_) {
				if (!uv_is_closing(reinterpret_cast<uv_handle_t*>(server_))) {
					uv_close(reinterpret_cast<uv_handle_t*>(server_), [](uv_handle_t* handle) {
						std::free(handle);
						});
				}
				server_ = nullptr;
			}

			for (auto& connection : client_connections_) {
				if (connection) {
					connection.reset();
				}
			}

			if (loop_ && uv_loop_alive(loop_)) {
				uv_stop(loop_);
				uv_loop_close(loop_);
			}

			loop_ = nullptr;
		}
	};

}