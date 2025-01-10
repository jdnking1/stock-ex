#include "order_gateway.hpp"


auto kse::example::gateway::on_connect(uv_connect_t* req, int status) -> void {
	std::string time_str;
	auto& self = order_gateway::get_instance();

	if (status < 0) {
		self.get_logger().log("%:% %() %  connection error: %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.get_time_str()), uv_strerror(status));
		return;
	}

	self.get_logger().log("%:% %() % connection established\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.get_time_str()));

	uv_read_start(req->handle, alloc_buffer, on_read);
}

auto kse::example::gateway::alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void
{
	tcp_connection_t* conn = static_cast<tcp_connection_t*>(handle->data);
	buf->base = conn->inbound_data_.data();
	buf->len = static_cast<unsigned long>(conn->inbound_data_.size());
}

auto kse::example::gateway::on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf [[maybe_unused]] ) -> void
{
	auto& self = order_gateway::get_instance();
	tcp_connection_t* conn = static_cast<tcp_connection_t*>(stream->data);

	if (nread < 0) [[unlikely]] {
		if (nread == UV_EOF) {
			self.get_logger().log("%:% %() %   connection closed\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.get_time_str()));
		}
		else {
			self.get_logger().log("%:% %() %   read error\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&self.get_time_str()));
		}

		uv_close((uv_handle_t*)stream, nullptr);
	}
	else if (nread > 0) {
		TIME_MEASURE(T2_OrderGateway_TCP_read, self.get_logger(), self.get_time_str());
		conn->next_rcv_valid_index_ += nread;

		self.get_logger().log("%:% %() % read socket: len:%\n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&self.get_time_str()), conn->next_rcv_valid_index_);

		self.read_data();
	}
}

auto kse::example::gateway::order_gateway::read_data() -> void
{
	if (connection_->next_rcv_valid_index_ >= sizeof(models::client_response_external)) {
		size_t i = 0;
		for (; i + sizeof(models::client_response_external) <= connection_->next_rcv_valid_index_; i += sizeof(models::client_response_external)) {
			auto response = deserialize_client_response(connection_->inbound_data_.data() + i);

			logger_.log("%:% %() % Received %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), response.to_string());

			if (client_id_ != models::INVALID_CLIENT_ID && client_id_ != response.response_.client_id_) [[unlikely]] {
				logger_.log("%:% %() % Invalid clientid received for this ClientResponse\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time_str_));
				continue;
			}


			if (response.sequence_number_ != next_exp_seq_num_) [[unlikely]] { 
				logger_.log("%:% %() % Incorrect sequence number. SeqNum expected:% received:%\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time_str_), next_exp_seq_num_, response.sequence_number_);
				continue;
			}
			else if (response.sequence_number_ == 1) {
				client_id_ = response.response_.client_id_;
			}
			else {
				auto next_write = incoming_responses_->get_next_write_element();
				*next_write = std::move(response.response_);
				incoming_responses_->next_write_index();
			}

			++next_exp_seq_num_;
		}
		connection_->shift_inbound_buffer(i);
	}
}

auto kse::example::gateway::on_idle(uv_idle_t* req [[maybe_unused]] ) -> void
{
	auto& self = order_gateway::get_instance();
	if (self.get_client_id() == models::INVALID_CLIENT_ID) [[unlikely]] {
		return;
	}
	self.send_request();
}

auto kse::example::gateway::order_gateway::send_request() -> void
{
	auto can_continue_write = true;
	for (auto* request = outgoing_requests_->get_next_read_element();
		outgoing_requests_->size() && request && can_continue_write;
		request = outgoing_requests_->get_next_read_element()) {
		request->client_id_ = client_id_;
		logger_.log("%:% %() % Processing seq:% %\n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&time_str_), next_outgoing_seq_num_, request->to_string());

		can_continue_write = connection_->append_to_outbound_buffer(*request, next_outgoing_seq_num_);	
		outgoing_requests_->next_read_index();
		++next_outgoing_seq_num_;
	}

	if (connection_->next_send_valid_index_ != 0) {
		auto* writer = static_cast<uv_write_t*>(std::malloc(sizeof(uv_write_t)));
		uv_buf_t buf = uv_buf_init(connection_->outbound_data_.data(), static_cast<unsigned int>(connection_->next_send_valid_index_));
		uv_write(writer, (uv_stream_t*)connection_->handle_, &buf, 1, [](uv_write_t* req, int status) {
			auto& self = order_gateway::get_instance();
			if (status < 0) [[unlikely]] {
				self.logger_.log("%:% %() % error writing data: %\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&self.time_str_), uv_strerror(status));
				return;
			}
			
			TIME_MEASURE(T1_OrderGateway_TCP_write, self.logger_, self.time_str_);
			self.logger_.log("%:% %() % send data to exchange\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&self.time_str_));

			std::free(req);
		});
	}

	connection_->next_send_valid_index_ = 0;
}


