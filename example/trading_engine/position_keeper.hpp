#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <sstream>

#include "utils/utils.hpp"
#include "utils/logger.hpp"
#include "models/basic_types.hpp"
#include "models/client_response.hpp"
#include "trading_utils/trading_utils.hpp"

#include "market_order_book.hpp"

namespace kse::example::trading {
	struct position_info_t {
		int32_t position_ = 0;
		double realized_pnl_ = 0; 
		double unrealized_pnl_ = 0;
		double total_pnl_ = 0;
		double buy_open_vwap_ = 0;
		double sell_open_vwap_ = 0;
		models::quantity_t volume_ = 0;
		const bbo_t* bbo_ = nullptr;

		auto to_string() const {
			std::stringstream ss;
			ss << "Position{"
				<< "pos:" << position_
				<< " u-pnl:" << unrealized_pnl_
				<< " r-pnl:" << realized_pnl_
				<< " t-pnl:" << total_pnl_
				<< " vol:" << models::quantity_to_string(volume_)
				<< " vwaps:[" << (position_ ? buy_open_vwap_ / std::abs(position_) : 0)
				<< "X" << (position_ ? sell_open_vwap_ / std::abs(position_) : 0)
				<< "] "
				<< (bbo_ ? bbo_->to_string() : "") << "}";

			return ss.str();
		}

		auto add_fill(const models::client_response_internal* client_response, utils::logger* logger) noexcept {
			const auto old_position = position_;
			auto& open_vwap = client_response->side_ == models::side_t::BUY ? buy_open_vwap_ : sell_open_vwap_;
			auto& opposite_side_open_vwap = client_response->side_ == models::side_t::BUY ? sell_open_vwap_ : buy_open_vwap_;
			const auto side_value = client_response->side_ == models::side_t::BUY ? 1 : -1;
			position_ += client_response->exec_qty_ * side_value;
			volume_ += client_response->exec_qty_;

			if (old_position * side_value >= 0) { // opened / increased position.
				open_vwap += (client_response->price_ * client_response->exec_qty_);
			}
			else { // decreased position.
				const auto opp_side_vwap = opposite_side_open_vwap / std::abs(old_position);
				opposite_side_open_vwap = opp_side_vwap * std::abs(position_);
				realized_pnl_ += std::min(static_cast<int32_t>(client_response->exec_qty_), std::abs(old_position)) *
					(opp_side_vwap - client_response->price_) * side_value;
				if (position_ * old_position < 0) { // flipped position to opposite sign.
					open_vwap = static_cast<double>((client_response->price_ * std::abs(position_)));
					opposite_side_open_vwap = 0;
				}
			}

			if (!position_) { // flat
				buy_open_vwap_ = sell_open_vwap_ = 0;
				unrealized_pnl_ = 0;
			}
			else {
				if (position_ > 0)
					unrealized_pnl_ =
					(client_response->price_ - buy_open_vwap_ / std::abs(position_)) *
					std::abs(position_);
				else
					unrealized_pnl_ =
					(sell_open_vwap_ / std::abs(position_) - client_response->price_) *
					std::abs(position_);
			}

			total_pnl_ = unrealized_pnl_ + realized_pnl_;

			std::string time_str;
			logger->log("%:% %() % % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str),
				to_string().c_str(), client_response->to_string().c_str());
		}

		auto update_bbo(const bbo_t* bbo, utils::logger* logger) noexcept {
			std::string time_str;
			bbo_ = bbo;

			if (position_ && bbo->bid_price_ != models::INVALID_PRICE && bbo->ask_price_ != models::INVALID_PRICE) {
				const auto mid_price = (bbo->bid_price_ + bbo->ask_price_) * 0.5;
				if (position_ > 0)
					unrealized_pnl_ =
					(mid_price - buy_open_vwap_ / std::abs(position_)) *
					std::abs(position_);
				else
					unrealized_pnl_ =
					(sell_open_vwap_ / std::abs(position_) - mid_price) *
					std::abs(position_);

				const auto old_total_pnl = total_pnl_;
				total_pnl_ = unrealized_pnl_ + realized_pnl_;

				if (total_pnl_ != old_total_pnl)
					logger->log("%:% %() % % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str),
						to_string().c_str(), bbo_->to_string().c_str());
			}
		}
	};
	class position_keeper {
	public:
		position_keeper() noexcept {};
		~position_keeper() noexcept {};
	private:
		std::string time_str_;
		utils::logger* logger_ = nullptr;

		std::array<position_info_t, models::MAX_NUM_INSTRUMENTS> instrument_position_;

	public:
		auto add_fill(const models::client_response_internal* client_response) noexcept {
			instrument_position_.at(client_response->instrument_id_).add_fill(client_response, logger_);
		}

		auto update_bbo(models::instrument_id_t instrument_id, const bbo_t* bbo) noexcept {
			instrument_position_.at(instrument_id).update_bbo(bbo, logger_);
		}

		auto get_position_info(models::instrument_id_t instrument_id) const noexcept {
			return &(instrument_position_.at(instrument_id));
		}

		auto to_string() const {
			double total_pnl = 0;
			models::quantity_t total_vol = 0;

			std::stringstream ss;
			for (models::instrument_id_t i = 0; i < instrument_position_.size(); ++i) {
				ss << "insturment id:" << models::instrument_id_to_string(i) << " " << instrument_position_.at(i).to_string() << "\n";

				total_pnl += instrument_position_.at(i).total_pnl_;
				total_vol += instrument_position_.at(i).volume_;
			}
			ss << "Total PnL:" << total_pnl << " Vol:" << total_vol << "\n";

			return ss.str();
		}
	};
}
