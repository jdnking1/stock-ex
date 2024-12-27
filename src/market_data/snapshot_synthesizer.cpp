#include "snapshot_synthesizer.hpp"

#include "market_data_encoder.hpp"


auto kse::market_data::snapshot_synthesizer::add_to_buffer(const models::client_market_update& update) -> void
{
	serialize_client_market_update(update, buffer_.data());
	next_send_valid_index_ += sizeof(models::client_market_update);
	utils::DEBUG_ASSERT(next_send_valid_index_ < BUFFER_SIZE, "buffer filled up");
}

auto kse::market_data::snapshot_synthesizer::add_to_snapshot(models::client_market_update* update) -> void
{
	const auto& underlying_market_update = update->update_;

	auto& instrument_updates = updates_by_instrument_.at(underlying_market_update.instrument_id_);

	switch (underlying_market_update.type_) {
		case models::market_update_type::ADD: {
			auto* new_update = instrument_updates.at(underlying_market_update.order_id_);
			utils::DEBUG_ASSERT(new_update == nullptr, "Received:" + underlying_market_update.to_string() + " but market update already exists:" + (new_update ? new_update->to_string() : ""));
			instrument_updates.at(underlying_market_update.order_id_) = market_update_pool_.alloc(underlying_market_update);
		}break;
		case models::market_update_type::MODIFY: {
			auto* stored_update = instrument_updates.at(underlying_market_update.order_id_);

			utils::DEBUG_ASSERT(stored_update != nullptr, "Received:" + underlying_market_update.to_string() + " but update does not exist.");
			utils::DEBUG_ASSERT(stored_update != nullptr && stored_update->order_id_ == underlying_market_update.order_id_, "Expecting existing update to match new one.");
			utils::DEBUG_ASSERT(stored_update != nullptr && stored_update->side_ == underlying_market_update.side_, "Expecting existing update to match new one.");

			if (stored_update != nullptr) [[likely]] {
				stored_update->qty_ = underlying_market_update.qty_;
				stored_update->price_ = underlying_market_update.price_;
			}
		}break;
		case models::market_update_type::CANCEL: {
			auto* stored_update = instrument_updates.at(underlying_market_update.order_id_);
			utils::DEBUG_ASSERT(stored_update != nullptr, "Received:" + underlying_market_update.to_string() + " but update does not exist.");
			utils::DEBUG_ASSERT(stored_update != nullptr && stored_update->order_id_ == underlying_market_update.order_id_, "Expecting existing update to match new one.");
			utils::DEBUG_ASSERT(stored_update != nullptr && stored_update->side_ == underlying_market_update.side_, "Expecting existing update to match new one.");

			market_update_pool_.free(stored_update);
			instrument_updates.at(underlying_market_update.order_id_) = nullptr;
		}break;
		default:break;
	}

	utils::DEBUG_ASSERT(update->sequence_number_ == last_inc_seq_num_ + 1, "Expected incremental seq_nums to increase.");
	last_inc_seq_num_ = update->sequence_number_;
}

auto kse::market_data::snapshot_synthesizer::process_client_market_update() -> void
{
	if (!market_update_queue_->size()) [[unlikely]] {
		return;
	}

	for (auto* market_update = market_update_queue_->get_next_read_element(); market_update_queue_->size() && market_update; market_update = market_update_queue_->get_next_read_element()) {
		logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
			market_update->to_string().c_str());

		add_to_snapshot(market_update);

		market_update_queue_->next_read_index();
	}
}

auto kse::market_data::process_update(uv_idle_t* handle [[maybe_unused]] ) -> void
{
	auto& self = snapshot_synthesizer::get_instance();
	self.process_client_market_update();
}

static auto on_send(uv_udp_send_t* req[[maybe_unused]], int status) -> void
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

auto kse::market_data::snapshot_synthesizer::send_data() -> void
{
	struct sockaddr_in multicast_addr;
	uv_ip4_addr(ip_.c_str(), port_, &multicast_addr);

	uv_buf_t buffer = uv_buf_init(buffer_.data(), static_cast<unsigned int>(next_send_valid_index_));

	uv_udp_send(sender_, socket_, &buffer, 1, (const struct sockaddr*)&multicast_addr, on_send);
}

auto kse::market_data::snapshot_synthesizer::publish_snapshot() -> void
{
	size_t snapshot_size = 0;

	const models::client_market_update start_market_update{ snapshot_size++, {models::market_update_type::SNAPSHOT_START, last_inc_seq_num_} };
	add_to_buffer(start_market_update);
	logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), start_market_update.to_string());

	for (models::instrument_id_t instrument_id = 0; instrument_id < updates_by_instrument_.size(); ++instrument_id) {
		const auto& instrument_updates = updates_by_instrument_.at(instrument_id);

		models::market_update me_market_update;
		me_market_update.type_ = models::market_update_type::CLEAR;
		me_market_update.instrument_id_ = instrument_id;

		const models::client_market_update clear_market_update{ snapshot_size++, me_market_update };
		logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), clear_market_update.to_string());
		add_to_buffer(clear_market_update);

		for (const auto* update : instrument_updates) {
			if (update) {
				const models::client_market_update market_update{ snapshot_size++, *update };
				logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), market_update.to_string());
				add_to_buffer(market_update);
				send_data();
			}
		}
	}

	const models::client_market_update end_market_update{ snapshot_size++, {models::market_update_type::SNAPSHOT_END, last_inc_seq_num_} };
	logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_), end_market_update.to_string());
	add_to_buffer(end_market_update);
	send_data();

	logger_.log("%:% %() % Published snapshot of % orders.\n", __FILE__, __LINE__, __FUNCTION__, utils::get_curren_time_str(&time_str_), snapshot_size - 1);
}

auto kse::market_data::publish(uv_timer_t* handle [[maybe_unused]] ) -> void
{
	auto& self = snapshot_synthesizer::get_instance();
	self.publish_snapshot();
}
