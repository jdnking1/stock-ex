#pragma once

#include "utils/logger.hpp"
#include "models/basic_types.hpp"

#include "order_manager.hpp"
#include "feature_engine.hpp"

#include "trading_utils/trading_utils.hpp"

using namespace kse::example::trading_utils;

namespace kse::example::trading {
    class trade_engine;

    class liquidity_taker {
    public:
        liquidity_taker(utils::logger* logger, trade_engine* engine, const feature_engine* f_engine,
            order_manager* om, const trade_engine_config_map& cfgs);

        auto on_order_book_update(models::instrument_id_t instrument_id, models::price_t price, models::side_t side, const market_order_book* book [[maybe_unused]] ) noexcept -> void {
            logger_->log("%:% %() % instrument:% price:% side:%\n", __FILE__, __LINE__, __func__,
                utils::get_curren_time_str(&time_str_), instrument_id, models::price_to_string(price).c_str(),
                models::side_to_string(side).c_str());
        }

        auto on_trade_update(const market_update* market_update, market_order_book* book) noexcept -> void {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                market_update->to_string().c_str());

            const auto& bbo = book->get_bbo();
            const auto agg_qty_ratio = feature_engine_->get_agg_trade_qty_ratio();

            if (bbo.bid_price_ != INVALID_PRICE && bbo.ask_price_ != INVALID_PRICE && agg_qty_ratio != INVALID_FEATURE) {
                logger_->log("%:% %() % % agg-qty-ratio:%\n", __FILE__, __LINE__, __func__,
                    utils::get_curren_time_str(&time_str_),
                    bbo.to_string().c_str(), agg_qty_ratio);

                const auto clip = instruments_cfg_.at(market_update->instrument_id_).clip_;
                const auto threshold = instruments_cfg_.at(market_update->instrument_id_).threshold_;

                if (agg_qty_ratio >= threshold) {
                    if (market_update->side_ == side_t::BUY)
                        order_manager_->move_orders(market_update->instrument_id_, bbo.ask_price_, INVALID_PRICE, clip);
                    else
                        order_manager_->move_orders(market_update->instrument_id_, INVALID_PRICE, bbo.bid_price_, clip);
                }
            }
        }

        auto on_order_update(const client_response_internal* client_response) noexcept -> void {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                client_response->to_string().c_str());

            order_manager_->on_order_update(client_response);
        }

        liquidity_taker(const liquidity_taker&) = delete;

        liquidity_taker(const liquidity_taker&&) = delete;

        liquidity_taker& operator=(const liquidity_taker&) = delete;

        liquidity_taker& operator=(const liquidity_taker&&) = delete;

    private:
        const feature_engine* feature_engine_ = nullptr;

        order_manager* order_manager_ = nullptr;

        std::string time_str_;
        utils::logger* logger_ = nullptr;

        const trade_engine_config_map instruments_cfg_;
    };
}