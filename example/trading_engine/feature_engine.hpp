#pragma once

#include <limits>
#include <string>

#include "utils/utils.hpp"
#include "utils/logger.hpp"

#include "market_order_book.hpp"


namespace kse::example::trading {
	constexpr auto INVALID_FEATURE = std::numeric_limits<double>::quiet_NaN();

	class feature_engine {
	public:
		feature_engine(utils::logger* logger) noexcept : logger_{ logger } {}
		feature_engine(const feature_engine&) = delete;
		feature_engine(feature_engine&&) = delete;
		feature_engine& operator=(const feature_engine&) = delete;
		feature_engine& operator=(feature_engine&&) = delete;

		auto get_market_price() const noexcept -> double { return market_price_; }
		auto get_agg_trade_qty_ratio() const noexcept -> double { return agg_trade_qty_ratio_; }

		auto on_order_book_update(models::instrument_id_t insturment_id, models::price_t price, models::side_t side, market_order_book* book) noexcept -> void {
			const auto& bbo = book->get_bbo();
			if ((bbo.bid_price_ != models::INVALID_PRICE && bbo.ask_price_ != models::INVALID_PRICE)) [[likely]] {
				market_price_ = (bbo.bid_price_ * bbo. ask_qty_ + bbo.ask_price_ * bbo.bid_qty_) / static_cast<double>(bbo.bid_qty_ + bbo.ask_qty_);
			}

			logger_->log("%:% %() % instrument:% price:% side:% mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_), insturment_id, models::price_to_string(price).c_str(),
				models::side_to_string(side).c_str(), market_price_, agg_trade_qty_ratio_);
		}

		auto on_trade_update(const models::market_update* market_update, market_order_book* book) noexcept -> void {
			const auto& bbo = book->get_bbo();
			if ((bbo.bid_price_ != models::INVALID_PRICE && bbo.ask_price_ != models::INVALID_PRICE)) [[likely]] {
				agg_trade_qty_ratio_ = static_cast<double>(market_update->qty_) / (market_update->side_ == models::side_t::BUY ? bbo.ask_qty_ : bbo.bid_qty_);
			}

			logger_->log("%:% %() % % mkt-price:% agg-trade-ratio:%\n", __FILE__, __LINE__, __func__,
				utils::get_curren_time_str(&time_str_),
				market_update->to_string().c_str(), market_price_, agg_trade_qty_ratio_);
		}

	private:
		std::string time_str_;
		utils::logger* logger_ = nullptr;

		double market_price_ = INVALID_FEATURE;
		double agg_trade_qty_ratio_ = INVALID_FEATURE;
	};

}