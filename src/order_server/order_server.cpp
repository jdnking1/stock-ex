#include "order_server.hpp"

#include <algorithm>
#include <cstring>



auto kse::server::on_new_connection(uv_stream_t* server, int status) -> void
{
	auto& self = order_server::get_instance();
	self.handle_new_connection(server, status);
}

auto kse::server::order_server::handle_new_connection(uv_stream_t* server, int status) -> void
{
	if (status < 0) [[unlikely]] {
		logger_.log("%:% %() %  new connection error: %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), uv_strerror(status));
		return;
	}

	auto conn = std::make_unique<tcp_connection_t>();
	uv_tcp_init(loop_, conn->handle_);

	if (uv_accept(server, (uv_stream_t*)conn->handle_) == 0) {
		logger_.log("%:% %() % have_new_connection for clientID: %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), next_client_id_);

		conn->handle_->data = conn.get();

		auto* response = server_responses_.get_next_write_element();
		*response = models::client_response_internal{ models::client_response_type::ACCEPTED, next_client_id_, models::INVALID_INSTRUMENT_ID, models::INVALID_ORDER_ID,
			models::INVALID_ORDER_ID, models::side_t::INVALID, models::INVALID_PRICE, models::INVALID_QUANTITY, models::INVALID_QUANTITY };
		server_responses_.next_write_index();

		uv_read_start((uv_stream_t*)conn->handle_, alloc_buffer, on_read);

		client_connections_.at(next_client_id_++) = std::move(conn);
	}
	else {
		uv_close((uv_handle_t*)conn->handle_, nullptr);
		logger_.log("%:% %() %  can't establish connection\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
	}
}

auto kse::server::alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void
{
	tcp_connection_t* conn = static_cast<tcp_connection_t*>(handle->data);
	buf->base = conn->inbound_data_.data();
	buf->len = static_cast<unsigned long>(conn->inbound_data_.size());
}

auto kse::server::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf [[maybe_unused]] ) -> void
{
	auto& self = order_server::get_instance();
	tcp_connection_t* conn = static_cast<tcp_connection_t*>(stream->data);

	if (nread < 0) [[unlikely]] {
		if (nread == UV_EOF) {
			self.logger_.log("%:% %() %   connection closed\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.time_str_));
		}
		else {
			self.logger_.log("%:% %() %   read error\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.time_str_));
		}

		uv_close((uv_handle_t*)stream, nullptr);
	}
	else if (nread > 0) {
		conn->next_rcv_valid_index_ += nread;

		const utils::nananoseconds_t user_time = uv_now(self.loop_) * utils::NANOS_PER_MILLIS;

		self.logger_.log("%:% %() % read socket: len:% utime:% \n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&self.time_str_), conn->next_rcv_valid_index_, user_time);

		self.read_data(conn, user_time);
	}
}

auto kse::server::order_server::read_data(tcp_connection_t* conn, utils::nananoseconds_t user_time) -> void
{
	logger_.log("%:% %() % Received socket:len:% rx:%\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
		conn->next_rcv_valid_index_, user_time);

	if (conn->next_rcv_valid_index_ >= sizeof(models::client_request_external)) {
		size_t i = 0;
		for (; i + sizeof(models::client_request_external) <= conn->next_rcv_valid_index_; i += sizeof(models::client_request_external)) {
			auto request = deserialize_client_request(conn->inbound_data_.data() + i);

			logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), request.to_string());

			if (client_connections_[request.request_.client_id_].get() != conn) {
				logger_.log("%:% %() % Invalid socket for this ClientRequest from ClientId:% \n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time_str_), request.request_.client_id_);
				send_invalid_response(request.request_.client_id_);
				continue;
			}

			auto& next_incoming_seq_num = client_next_incoming_seq_num_[request.request_.client_id_];

			if (request.sequence_number_ != next_incoming_seq_num) { 
				logger_.log("%:% %() % Incorrect sequence number. ClientId:% SeqNum expected:% received:%\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time_str_), request.request_.client_id_, next_incoming_seq_num, request.sequence_number_);
				send_invalid_response(request.request_.client_id_);
				continue;
			}

			++next_incoming_seq_num;

			fifo_sequencer_.add_request(user_time, request.request_);
		}
		conn->shift_inbound_buffer(i);
	}
}

auto kse::server::on_idle(uv_idle_t* req [[maybe_unused]] ) -> void
{
	auto& self = order_server::get_instance();

	auto process_responses = [&](auto* response_queue, auto& next_seq_nums) {
		for (auto* client_response = response_queue->get_next_read_element();
			response_queue->size() && client_response;
			client_response = response_queue->get_next_read_element()) {

			auto& next_outgoing_seq_num = next_seq_nums[client_response->client_id_];
			self.logger_.log("%:% %() % Processing cid:% seq:% %\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&self.time_str_),
				client_response->client_id_, next_outgoing_seq_num, client_response->to_string());

			utils::DEBUG_ASSERT(self.client_connections_[client_response->client_id_] != nullptr,
				"Don't have a TCPSocket for ClientId:" + std::to_string(client_response->client_id_));

			auto* conn = self.client_connections_[client_response->client_id_].get();

			conn->append_to_outbound_buffer(*client_response, next_outgoing_seq_num);

			uv_buf_t buf = uv_buf_init(conn->outbound_data_.data(), static_cast<unsigned int>(conn->next_send_valid_index_));
			uv_write(conn->writer_, (uv_stream_t*)conn->handle_, &buf, 1, [](uv_write_t* req [[maybe_unused]], int status) {
				auto& self = order_server::get_instance();
				if (status < 0) {
					self.logger_.log("%:% %() % error writing data: %\n", __FILE__, __LINE__, __func__,
						utils::get_curren_time_str(&self.time_str_), uv_strerror(status));
					return;
				}

				self.logger_.log("%:% %() % send data to socket\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&self.time_str_));
				});

			response_queue->next_read_index();

			++next_outgoing_seq_num;
		}
	};

	process_responses(self.matching_engine_responses_, self.client_next_outgoing_seq_num_);
	process_responses(&self.server_responses_, self.client_next_outgoing_seq_num_);
}

auto kse::server::on_check(uv_check_t* req [[maybe_unused]] ) -> void
{
	auto& self = order_server::get_instance();
	self.fifo_sequencer_.sequence_and_publish();
}

