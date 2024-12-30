#include "trade_engine.hpp"

kse::example::trading::trade_engine::trade_engine(algo_type_t algo_type, const trade_engine_config_map& cfg, models::client_request_queue* client_requests, 
	models::client_response_queue* client_responses, models::market_update_queue* market_updates) : outgoing_ogw_requests_{ client_requests }, incoming_ogw_responses_{ client_responses },
	incoming_md_updates_{ market_updates }, logger_{ "trading_engine_" + std::to_string((uintptr_t)this) + ".log" }, feature_engine_{ &logger_ },
	risk_manager_{ &position_keeper_, cfg }, order_manager_{ &logger_, this, &risk_manager_ }
{
	for (size_t i = 0; i < instrument_order_book_.size(); ++i) {
		instrument_order_book_[i] = std::make_unique<market_order_book>(static_cast<instrument_id_t>(i), &logger_);
		instrument_order_book_[i]->set_trade_engine(this);
	}

	algo_on_order_book_update_ = [this](auto instrument_id, auto price, auto side, auto book) {default_algo_on_order_book_update(instrument_id, price, side, book);};
	algo_on_trade_update_ = [this](auto market_update, auto book) { default_algo_on_trade_update(market_update, book); };
	algo_on_order_update_ = [this](auto client_response) { default_algo_on_order_update(client_response); };

	if (algo_type == algo_type_t::MAKER) {
		mm_algo_ = std::make_unique<market_maker>(&logger_, this, &feature_engine_, &order_manager_, cfg);
	}
	else if (algo_type == algo_type_t::TAKER) {
		taker_algo_ = std::make_unique<liquidity_taker>(&logger_, this, &feature_engine_, &order_manager_, cfg);
	}

	for (instrument_id_t i = 0; i < cfg.size(); ++i) {
		logger_.log("%:% %() % Initialized % Instrument:% %.\n", __FILE__, __LINE__, __func__,
			utils::get_curren_time_str(&time_str_),
			algo_type_to_string(algo_type), i,
			cfg.at(i).to_string());
	}
}

kse::example::trading::trade_engine::~trade_engine()
{
	run_ = false;

	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	for (auto& book : instrument_order_book_) {
		book.reset();
	}

	outgoing_ogw_requests_ = nullptr;
	incoming_ogw_responses_ = nullptr;
	incoming_md_updates_ = nullptr;
}

auto kse::example::trading::trade_engine::run() noexcept -> void
{
	logger_.log("%:% %() %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_));

	while (run_) {
		for (auto client_response = incoming_ogw_responses_->get_next_read_element(); client_response; client_response = incoming_ogw_responses_->get_next_read_element()) {
			logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
				client_response->to_string().c_str());
			on_order_update(client_response);
			incoming_ogw_responses_->next_read_index();
			last_event_time_ = utils::get_current_timestamp();
		}

		for (auto market_update = incoming_md_updates_->get_next_read_element(); market_update; market_update = incoming_md_updates_->get_next_read_element()) {
			logger_.log("%:% %() % Processing %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
				market_update->to_string().c_str());
			utils::DEBUG_ASSERT(market_update->instrument_id_ < instrument_order_book_.size(),
				"Unknown instrument-id on update:" + market_update->to_string());
			instrument_order_book_.at(market_update->instrument_id_)->on_market_update(market_update);
			incoming_md_updates_->next_read_index();
			last_event_time_ = utils::get_current_timestamp();
		}
	}
}

auto kse::example::trading::trade_engine::send_client_request(const client_request_internal& client_request) noexcept -> void
{
	logger_.log("%:% %() % Sending %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
		client_request.to_string().c_str());
	auto next_write = outgoing_ogw_requests_->get_next_write_element();
	*next_write = client_request;
	outgoing_ogw_requests_->next_write_index();

}

auto kse::example::trading::trade_engine::on_order_book_update(instrument_id_t instrument_id, price_t price, side_t side, market_order_book* book) noexcept -> void
{
	logger_.log("%:% %() % instrument:% price:% side:%\n", __FILE__, __LINE__, __func__,
		utils::get_curren_time_str(&time_str_), instrument_id, price_to_string(price).c_str(),
		side_to_string(side).c_str());

	auto& bbo = book->get_bbo();

	position_keeper_.update_bbo(instrument_id, &bbo);

	feature_engine_.on_order_book_update(instrument_id, price, side, book);

	algo_on_order_book_update_(instrument_id, price, side, book);
}

auto kse::example::trading::trade_engine::on_trade_update(const market_update* market_update, market_order_book* book) noexcept -> void
{
	logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
		market_update->to_string().c_str());

	feature_engine_.on_trade_update(market_update, book);

	algo_on_trade_update_(market_update, book);
}

auto kse::example::trading::trade_engine::on_order_update(const client_response_internal* client_response) noexcept -> void
{
	logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
		client_response->to_string().c_str());

	if (client_response->type_ == client_response_type::FILLED) [[unlikely]] {
		position_keeper_.add_fill(client_response);
	}

	algo_on_order_update_(client_response);
}
