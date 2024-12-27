#include "market_data_publisher.hpp"

#include "market_data_encoder.hpp"


auto kse::market_data::market_data_publisher::add_to_buffer(const models::client_market_update& update) -> void
{
	serialize_client_market_update(update, buffer_.data());
	next_send_valid_index_ += sizeof(models::client_market_update);
	utils::DEBUG_ASSERT(next_send_valid_index_ < BUFFER_SIZE, "buffer filled up");
}

static auto on_send(uv_udp_send_t* req [[maybe_unused]], int status) -> void
{
	auto& self = kse::market_data::snapshot_synthesizer::get_instance();
	std::string time_str{};
	if (status < 0) {
		self.get_logger().log("%:% %() %  send error: %\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str), uv_strerror(status));
	}
	else {
		self.get_logger().log("%:% %() % data sent successfully\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
	}
}

auto kse::market_data::market_data_publisher::send_data() -> void
{
	struct sockaddr_in multicast_addr;
	uv_ip4_addr(ip_.c_str(), port_, &multicast_addr);

	uv_buf_t buffer = uv_buf_init(buffer_.data(), static_cast<unsigned int>(next_send_valid_index_));

	uv_udp_send(sender_, socket_, &buffer, 1, (const struct sockaddr*)&multicast_addr, on_send);
}


auto kse::market_data::market_data_publisher::process_and_publish() -> void
{
	if (!outgoing_market_update_queue_->size()) [[unlikely]] {
		return;
	}

	for (auto* market_update = outgoing_market_update_queue_->get_next_read_element();
		outgoing_market_update_queue_->size() && market_update; market_update = outgoing_market_update_queue_->get_next_read_element()) {

		logger_.log("%:% %() % Sending seq:% %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), next_inc_seq_num_,
			market_update->to_string().c_str());

		models::client_market_update client_market_update{ next_inc_seq_num_, *market_update};
		add_to_buffer(client_market_update);

		outgoing_market_update_queue_->next_read_index();

		auto* next_write = snapshot_market_update_queue_.get_next_write_element();
		next_write->sequence_number_ = next_inc_seq_num_;
		next_write->update_ = *market_update;
		snapshot_market_update_queue_.next_write_index();

		++next_inc_seq_num_;
	}

	send_data();
	next_send_valid_index_ = 0;
}


auto kse::market_data::process_incremental_update(uv_idle_t* handle [[maybe_unused]] ) -> void
{
	auto& self = market_data_publisher::get_instance();
	self.process_and_publish();
}
