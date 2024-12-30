#include "feed_handler.hpp"

#include "market_data_decoder.hpp"

#include <utility>


inline auto kse::example::market_data::alloc_buffer(uv_handle_t* handle, size_t suggested_size [[maybe_unused]], uv_buf_t* buf) -> void {
	multicast_connection_t* connection = static_cast<multicast_connection_t*>(handle->data);
	buf->base = connection->buffer.data();
	buf->len = static_cast<unsigned long>(connection->buffer.size());
}

inline auto kse::example::market_data::on_read(uv_udp_t* handle, ssize_t nread, const uv_buf_t* buf [[maybe_unused]], const sockaddr* addr [[maybe_unused]], unsigned flags [[maybe_unused]] ) -> void {
	auto& self = feed_handler::get_instance();
	multicast_connection_t* conn = static_cast<multicast_connection_t*>(handle->data);
	std::string time_str{};

	if (nread < 0) [[unlikely]] {
		if (nread == UV_EOF) {
			self.get_logger().log("%:% %() %   connection closed\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str));
		}
		else {
			self.get_logger().log("%:% %() %   read error\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str));
		}

		uv_close((uv_handle_t*)handle, nullptr);
	}
	else if (nread > 0) {
		conn->offset += nread;

		self.get_logger().log("%:% %() % read socket: len:% \n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&time_str), conn->offset);

		self.read_data(conn);
	}
}

auto kse::example::market_data::feed_handler::sync_snapshot_with_incremental() -> void
{
	if (snapshot_queued_msgs_.empty()) {
		return;
	}

	if (snapshot_queued_msgs_.begin()->first != 0) {
		logger_.log("%:% %() % Returning because have not seen a SNAPSHOT_START yet.\n",
			__FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
		snapshot_queued_msgs_.clear();
		return;
	}

	std::vector<models::market_update> synchronized_updates{};

	auto have_complete_snapshot = true;
	uint64_t next_snapshot_seq = 0;

	for (const auto& [seq, update] : snapshot_queued_msgs_) {
		if (seq != next_snapshot_seq) {
			have_complete_snapshot = false;
			logger_.log("%:% %() % Detected gap in snapshot stream expected:% found:% %.\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), next_snapshot_seq, seq, update.to_string());
			break;
		}

		if (seq != 0 && update.type_ != models::market_update_type::SNAPSHOT_END) {
			synchronized_updates.emplace_back(update);
		}

		++next_snapshot_seq;
	}

	if (!have_complete_snapshot) {
		logger_.log("%:% %() % Returning because found gaps in snapshot stream.\n",
			__FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
		snapshot_queued_msgs_.clear();
		return;
	}

	const auto& last_snapshot_msg = snapshot_queued_msgs_.rbegin()->second;
	if (last_snapshot_msg.type_ != models::market_update_type::SNAPSHOT_END) {
		logger_.log("%:% %() % Returning because have not seen a SNAPSHOT_END yet.\n",
			__FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
		return;
	}

	auto have_complete_incremental = true;
	uint64_t num_incremental_msgs = 0;
	next_expected_seq_ = last_snapshot_msg.order_id_ + 1;

	for (const auto& [seq, update] : incremental_queued_msgs_) {
		logger_.log("%:% %() % Checking next_exp:% vs. seq:% %.\n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&time_str_), next_expected_seq_, seq, update.to_string());

		if (seq < next_expected_seq_) continue;

		if (seq != next_expected_seq_) {
			have_complete_incremental = false;
			logger_.log("%:% %() % Detected gap in incremental stream expected:% found:% %.\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), next_expected_seq_, seq, update.to_string());
			break;
		}

		logger_.log("%:% %() % % => %\n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&time_str_), seq, update.to_string());

		synchronized_updates.emplace_back(update);
		++num_incremental_msgs;
		++next_expected_seq_;
	}

	if (!have_complete_incremental) {
		logger_.log("%:% %() % Returning because found gaps in incremental stream.\n",
			__FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));
		snapshot_queued_msgs_.clear();
		return;
	}

	for (auto& upd : synchronized_updates) {
		auto next_write = incoming_updates_->get_next_write_element();
		*next_write = std::move(upd);
		incoming_updates_->next_write_index();
	}

	logger_.log("%:% %() % Synchronized % snapshot and % incremental orders.\n", __FILE__, __LINE__, __func__,
		utils::get_curren_time_str(&time_str_), snapshot_queued_msgs_.size() - 2, num_incremental_msgs);

	snapshot_queued_msgs_.clear();
	incremental_queued_msgs_.clear();
	in_recovery_ = false;

	uv_close((uv_handle_t*)snapshot_feed_->handle_, nullptr);
}

auto kse::example::market_data::feed_handler::queue_message(bool is_snapshot, const models::client_market_update upd) -> void
{
	if (is_snapshot) {
		if (snapshot_queued_msgs_.contains(upd.sequence_number_)) {
			logger_.log("%:% %() % Packet drops on snapshot socket. Received for a 2nd time:%\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), upd.to_string());
			snapshot_queued_msgs_.clear();
		}

		snapshot_queued_msgs_.emplace(upd.sequence_number_, upd.update_);
	}
	else {
		incremental_queued_msgs_.emplace(upd.sequence_number_, upd.update_);
	}

	logger_.log("%:% %() % size snapshot:% incremental:% % => %\n", __FILE__, __LINE__, __func__,
		utils::get_curren_time_str(&time_str_), snapshot_queued_msgs_.size(), incremental_queued_msgs_.size(), upd.sequence_number_, upd.to_string());

	sync_snapshot_with_incremental();
}

auto kse::example::market_data::feed_handler::read_data(multicast_connection_t* conn) -> void
{
	const auto is_snapshot = conn == snapshot_feed_.get();

	if (conn->offset >= sizeof(models::client_market_update)) {
		size_t i = 0;

		for (; i + sizeof(models::client_market_update) <= conn->offset; i += sizeof(models::client_market_update)) {
			auto update = deserialize_market_update(conn->buffer.data() + i);

			logger_.log("%:% %() % Received % socket len:% %\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_),
				(is_snapshot ? "snapshot" : "incremental"), sizeof(models::client_market_update), update.to_string());
			
			auto already_in_recovery = in_recovery_;
			in_recovery_ = already_in_recovery || (update.sequence_number_ != next_expected_seq_);

			if (in_recovery_) [[unlikely]] {
				if (!already_in_recovery) [[unlikely]] {
					logger_.log("%:% %() % Packet drops on % socket. SeqNum expected:% received:%\n", __FILE__, __LINE__, __func__,
						utils::get_curren_time_str(&time_str_), (is_snapshot ? "snapshot" : "incremental"), next_expected_seq_, update.sequence_number_);
					snapshot_queued_msgs_.clear();
					incremental_queued_msgs_.clear();
					uv_udp_recv_start(snapshot_feed_->handle_, alloc_buffer, on_read);
				}
				queue_message(is_snapshot, update);
			}
			else if (!is_snapshot) {
				logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__,
					utils::get_curren_time_str(&time_str_), update.to_string());

				++next_expected_seq_;

				auto* next_write = incoming_updates_->get_next_write_element();
				*next_write = std::move(update.update_);
				incoming_updates_->next_write_index();
			}
		}

		conn->shift_inbound_buffer(i);
	}

}
