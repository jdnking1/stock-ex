#pragma once 

#include "uv.h"
#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include "utils/memory_pool.hpp"
#include <array>
#include <mutex>
#include <string>
#include <string_view>
#include <stdlib.h>
#include <vector>

#include "fifo_sequencer.hpp"
#include "serializer.hpp"


namespace kse::server {
	constexpr size_t TCP_BUFFER_SIZE = 64 * 1024 * 1024;

	constexpr size_t MAX_BUFFERED_RESPONSE = 1024 * 1024;

	struct tcp_connection_t {
		uv_tcp_t* handle_ = nullptr;
		uv_async_t* async_write_msg_ = nullptr;
		std::vector<char> outbound_data_;
		size_t next_send_valid_index_ = 0;
		std::vector<char> inbound_data_;
		size_t next_rcv_valid_index_ = 0;

		utils::lock_free_queue<size_t> write_queue_; //contains index of the outbound buffer

		explicit tcp_connection_t() : handle_{ (uv_tcp_t*)std::malloc(sizeof(uv_tcp_t)) }, async_write_msg_{ (uv_async_t*)std::malloc(sizeof(uv_async_t)) }, 
			write_queue_{ MAX_BUFFERED_RESPONSE } {
			outbound_data_.resize(sizeof(models::client_response_external) * MAX_BUFFERED_RESPONSE);
			inbound_data_.resize(TCP_BUFFER_SIZE);
		}

		~tcp_connection_t() noexcept {
			if (async_write_msg_) {
				if (!uv_is_closing((uv_handle_t*)async_write_msg_)) {
					uv_close((uv_handle_t*)async_write_msg_, [](uv_handle_t* handle) { std::free(handle); });
				}
				async_write_msg_ = nullptr;
			}

			if (handle_) {
				if (!uv_is_closing((uv_handle_t*)handle_)) {
					uv_close((uv_handle_t*)handle_, [](uv_handle_t* handle) { std::free(handle); });
				}
				handle_ = nullptr;
			}
		}

		auto append_to_outbound_buffer(models::client_response_internal& response, uint64_t sequence_number) -> size_t {
			if (next_send_valid_index_ > MAX_BUFFERED_RESPONSE) [[unlikely]] {
				next_send_valid_index_ = 0;
			}

			*write_queue_.get_next_write_element() = next_send_valid_index_;
			write_queue_.next_write_index();

			serialize_client_response({ sequence_number, std::move(response) }, get_response_buffer(next_send_valid_index_));
			next_send_valid_index_ += 1;

			return next_send_valid_index_ - 1;
		}

		auto get_response_buffer(size_t index) -> char* {
			return outbound_data_.data() + index * sizeof(models::client_response_external);
		}

		auto shift_inbound_buffer(size_t processed_bytes) -> void {
			if (processed_bytes > 0 && processed_bytes <= next_rcv_valid_index_) {
				std::memmove(inbound_data_.data(), inbound_data_.data() + processed_bytes, next_rcv_valid_index_ - processed_bytes);
				next_rcv_valid_index_ -= processed_bytes;
			}
		}
	};

	auto on_new_connection(uv_stream_t* server, int status) -> void;

	auto alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void;

	auto on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf [[maybe_unused]] ) -> void;

	auto write_message(uv_async_t* async) -> void;

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
			running_ = true;
			auto order_server_thread = utils::create_thread(0, [this]() { run(); });
			auto order_server_response_thread = utils::create_thread(4, [this]() { process_responses(); });
			order_server_thread.detach();
			order_server_response_thread.detach();
		}

		auto stop() -> void { 
			running_ = false;
		}

		auto run() -> void {
			logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

			loop_ = uv_default_loop();
			uv_tcp_init(loop_, server_);

			struct sockaddr_in addr;
			uv_ip4_addr(ip_.c_str(), port_, &addr);
			uv_tcp_bind(server_, (const struct sockaddr*)&addr, 0);

			uv_listen((uv_stream_t*)server_, SOMAXCONN, on_new_connection);

			uv_check_init(loop_, check_);
			uv_check_start(check_, on_check);

			uv_run(loop_, UV_RUN_DEFAULT);
		}

		auto push_server_response(models::client_response_internal& r) -> void { 
			auto* response = server_responses_.get_next_write_element();
			*response = std::move(r);
			server_responses_.next_write_index();
		}

		auto send_invalid_response(models::client_id_t client_id) -> void {
			auto response = models::client_response_internal{
				models::client_response_type::INVALID_REQUEST, client_id,
				models::INVALID_INSTRUMENT_ID, models::INVALID_ORDER_ID,
				models::INVALID_ORDER_ID, models::side_t::INVALID,
				models::INVALID_PRICE, models::INVALID_QUANTITY,
				models::INVALID_QUANTITY
			};
			push_server_response(response);
		}

		auto send_connection_accepted_response(models::client_id_t client_id) -> void {
			auto response = models::client_response_internal{ models::client_response_type::ACCEPTED, client_id, models::INVALID_INSTRUMENT_ID, models::INVALID_ORDER_ID,
			models::INVALID_ORDER_ID, models::side_t::INVALID, models::INVALID_PRICE, models::INVALID_QUANTITY, models::INVALID_QUANTITY };
			push_server_response(response);
		}

		auto process_responses() -> void;

		auto handle_new_connection(uv_stream_t* server, int status) -> void;

		auto read_data(tcp_connection_t* conn, utils::nananoseconds_t user_time) -> void;

		auto write_to_socket(tcp_connection_t* conn) -> void;

		std::string ip_;
		int port_;
		models::client_id_t next_client_id_ = 0;

		models::client_response_queue* matching_engine_responses_;
		models::client_response_queue server_responses_;

		std::string time_str_;
		utils::logger logger_;
		utils::logger logger_response_;

		std::array<models::client_id_t, models::MAX_NUM_CLIENTS> client_next_outgoing_seq_num_;
		std::array<models::client_id_t, models::MAX_NUM_CLIENTS> client_next_incoming_seq_num_;
		std::array<std::unique_ptr<tcp_connection_t>, models::MAX_NUM_CLIENTS> client_connections_;
		
		uv_loop_t* loop_ {nullptr};
		uv_tcp_t* server_{ nullptr };
		uv_check_t* check_{ nullptr };
		utils::uv_memory_pool<uv_write_t> writer_pool_;

		fifo_sequencer fifo_sequencer_;

		volatile bool running_ = false;

	private:
		order_server(
			models::client_request_queue* incoming_messages,
			models::client_response_queue* outgoing_messages,
			std::string_view ip,
			int port)
			:ip_{ ip }, port_{ port }, matching_engine_responses_{ outgoing_messages }, server_responses_{ MAX_PENDING_REQUESTS }, 
			logger_{ "kse_order_server.log" }, logger_response_{ "kse_order_server_responses.log" }, server_{ (uv_tcp_t*)std::malloc(sizeof(uv_tcp_t)) }, 
			check_{ (uv_check_t*)std::malloc(sizeof(uv_check_t)) }, writer_pool_{ MAX_BUFFERED_RESPONSE }, fifo_sequencer_{ incoming_messages, &logger_ } {
			client_next_incoming_seq_num_.fill(1);
			client_next_outgoing_seq_num_.fill(1);
		}

		~order_server()
		{
			stop();

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

			for(auto& connection : client_connections_) {
				if (connection) {
					connection.reset();
				}
			}
		}


		auto process_responses_helper(models::client_response_queue& responses) -> void {
			std::string time;
			for (auto* client_response = responses.get_next_read_element();
				responses.size() && client_response;
				client_response = responses.get_next_read_element()) {

				TIME_MEASURE(T5t_OrderServer_LFQueue_read, logger_response_, time);

				auto& next_outgoing_seq_num = client_next_outgoing_seq_num_[client_response->client_id_];

				logger_response_.debug_log("%:% %() % Processing cid:% seq:% %\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time),
					client_response->client_id_, next_outgoing_seq_num, client_response->to_string());

				utils::DEBUG_ASSERT(client_connections_[client_response->client_id_] != nullptr,
					"Don't have a TCPSocket for ClientId:" + std::to_string(client_response->client_id_));

				auto* conn = client_connections_[client_response->client_id_].get();

				START_MEASURE(Exchange_odsSerialization);
				conn->append_to_outbound_buffer(*client_response, next_outgoing_seq_num);
				END_MEASURE(Exchange_odsSerialization, logger_response_, time);
				
				uv_async_send(conn->async_write_msg_);
				
				responses.next_read_index();
				++next_outgoing_seq_num;
			}
		}
	};
}