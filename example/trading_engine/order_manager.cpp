#include "order_manager.hpp"


#include "models/client_request.hpp"
//#include "trade_engine.hpp"


auto kse::example::trading::order_manager::new_order(om_order* order, models::instrument_id_t instrument_id, models::price_t price, models::side_t side, models::quantity_t qty) noexcept -> void {
    const models::client_request_internal new_request{ models::client_request_type::NEW, 0, instrument_id,
                                            next_order_id_, side, price, qty };
    //trade_engine_->sendClientRequest(&new_request);

    *order = { instrument_id, next_order_id_, side, price, qty, om_order_state::PENDING_NEW };
    ++next_order_id_;

    logger_->log("%:% %() % Sent new order % for %\n", __FILE__, __LINE__, __func__,
        utils::get_curren_time_str(&time_str_),
        new_request.to_string().c_str(), order->to_string().c_str());
}

auto kse::example::trading::order_manager::cancel_order(om_order* order) noexcept -> void {
    const models::client_request_internal cancel_request{ models::client_request_type::CANCEL, 0, order->instrument_id_,
                                        order->order_id_, order->side_, order->price_, order->qty_ };

    //trade_engine_->sendClientRequest(&cancel_request);

    logger_->log("%:% %() % canceled order % with reques %\n", __FILE__, __LINE__, __func__,
        utils::get_curren_time_str(&time_str_), order->to_string().c_str(), cancel_request.to_string().c_str());
}
