#include "market_maker.hpp"
#include "trade_engine.hpp"

kse::example::trading::market_maker::market_maker(utils::logger* logger, trade_engine* engine, const feature_engine* f_engine, order_manager* om, const trade_engine_config_map& cfgs):
    feature_engine_ { f_engine }, order_manager_{ om }, logger_{ logger }, instruments_cfg_{ cfgs } {
    engine->algo_on_order_book_update_ = [this](auto instrument_id, auto price, auto side, auto book) { on_order_book_update(instrument_id, price, side, book); };
    engine->algo_on_trade_update_ = [this](auto market_update, auto book) { on_trade_update(market_update, book); };
    engine->algo_on_order_update_ = [this](auto client_response) { on_order_update(client_response); };
}
