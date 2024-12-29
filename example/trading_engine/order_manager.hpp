#pragma once 

#include "utils/logger.hpp"
#include "models/basic_types.hpp"
#include "models/client_response.hpp"

#include "trading_utils/trading_utils.hpp"

#include "om_order.hpp"
#include "risk_manager.hpp"


namespace kse::example::trading {
    class trade_engine;

    class order_manager {
    public:
        order_manager(utils::logger* logger,/*, trade_engine* engine,*/ risk_manager* rm)
            : /*trade_engine_{engine},*/ risk_manager_{rm}, logger_{logger} {
        }

        order_manager() = delete;

        order_manager(const order_manager&) = delete;

        order_manager(const order_manager&&) = delete;

        order_manager& operator=(const order_manager&) = delete;

        order_manager& operator=(const order_manager&&) = delete;

        auto on_order_update(const models::client_response_internal* client_response) noexcept -> void {
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                client_response->to_string().c_str());
            auto order = &(instrument_side_order_.at(client_response->instrument_id_).at(trading_utils::side_to_index(client_response->side_)));
            logger_->log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                order->to_string().c_str());

            switch (client_response->type_) {
                case models::client_response_type::ACCEPTED: {
                    order->order_state_ = om_order_state::LIVE;
                }break;
                case models::client_response_type::CANCELED: {
                    order->order_state_ = om_order_state::DEAD;
                }break;
                case models::client_response_type::MODIFIED: {
                    order->order_state_ = om_order_state::LIVE;
                }break;
                case models::client_response_type::FILLED: {
                    order->qty_ = client_response->leaves_qty_;
                    if (!order->qty_)
                        order->order_state_ = om_order_state::DEAD;
                }break;
                default: break;
            }
        }

        auto new_order(om_order* order, models::instrument_id_t instrument_id, models::price_t price, models::side_t side, models::quantity_t qty) noexcept -> void;

        auto cancel_order(om_order* order) noexcept -> void;

        auto move_order(om_order* order, models::instrument_id_t instrument_id, models::price_t price, models::side_t side, models::quantity_t qty) noexcept {
            switch (order->order_state_) {
                case om_order_state::LIVE: {
                    if (order->price_ != price)
                        cancel_order(order);
                }break;
                case om_order_state::INVALID:
                case om_order_state::DEAD: {
                    if (price != models::INVALID_PRICE) [[likely]] {
                        const auto risk_result = risk_manager_->check_pretrade_risk(instrument_id, side, qty);
                        if (risk_result == risk_check_result::ALLOWED) [[likely]] {
                            new_order(order, instrument_id, price, side, qty);
                        } 
                        else {
                            logger_->log("%:% %() % Ticker:% Side:% Qty:% RiskCheckResult:%\n", __FILE__, __LINE__, __func__,
                                utils::get_curren_time_str(&time_str_),
                                models::instrument_id_to_string(instrument_id), models::side_to_string(side), models::quantity_to_string(qty),
                                risk_check_result_to_string(risk_result));
                        }

                    }
                }break;
                case om_order_state::PENDING_MODIFIED:
                case om_order_state::PENDING_NEW:
                case om_order_state::PENDING_CANCEL:
                break;
            }
        }

        auto move_orders(models::instrument_id_t instrument_id, models::price_t bid_price, models::price_t ask_price, models::quantity_t clip) noexcept {
            auto bid_order = &(instrument_side_order_.at(instrument_id).at(trading_utils::side_to_index(models::side_t::BUY)));
            move_order(bid_order, instrument_id, bid_price, models::side_t::BUY, clip);

            auto ask_order = &(instrument_side_order_.at(instrument_id).at(trading_utils::side_to_index(models::side_t::SELL)));
            move_order(ask_order, instrument_id, ask_price, models::side_t::SELL, clip);
        }
        auto get_instrument_side_orders(models::instrument_id_t instrument_id) const {
            return &(instrument_side_order_.at(instrument_id));
        }
    private:
        //trade_engine* trade_engine_ = nullptr;
        const risk_manager* risk_manager_ = nullptr;

        std::string time_str_;
        utils::logger* logger_ = nullptr;

        om_order_by_instrument_side instrument_side_order_;
        models::order_id_t next_order_id_ = 1;
    };
}