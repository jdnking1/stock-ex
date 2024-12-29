#include "market_order_book.hpp"

#include "fmt/format.h"

kse::example::trading::market_order_book::market_order_book(models::instrument_id_t instrument_id, utils::logger* logger):
	instrument_id_{ instrument_id }, price_level_pool_{ models::MAX_PRICE_LEVELS }, order_pool_{ models::MAX_NUM_ORDERS }, logger_{ logger }
{
	orders_.resize(models::MAX_NUM_ORDERS);
}

kse::example::trading::market_order_book::~market_order_book()
{
	logger_->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __func__,
		utils::get_curren_time_str(&time_str_), to_string(false, true));

	trade_engine_ = nullptr;
	bid_ = ask_ = nullptr;
	orders_.clear();
}

auto kse::example::trading::market_order_book::on_market_update(const models::market_update* market_update) noexcept -> void
{
	const auto bid_updated = (bid_ && market_update->side_ == side_t::BUY && market_update->price_ >= bid_->price_);
	const auto ask_updated = (bid_ && market_update->side_ == side_t::SELL && market_update->price_ <= ask_->price_);

	switch (market_update->type_) {
		case models::market_update_type::ADD: {
			auto* order = order_pool_.alloc(market_update->order_id_, market_update->side_, market_update->price_,
				market_update->qty_, market_update->priority_, nullptr, nullptr);
			add_order(order);
		}break;
		case models::market_update_type::MODIFY: {
			auto* order = orders_.at(market_update->order_id_);
			order->price_ = market_update->price_;
			order->qty_ = market_update->qty_;
		}break;
		case models::market_update_type::CANCEL: {
			auto* order = orders_.at(market_update->order_id_);
			remove_order(order);
		}break;
		case models::market_update_type::TRADE: {
			//trading_engine_->on_trade_update(market_update, this);
			return;
		}break;
		case models::market_update_type::CLEAR: {
			for (auto* order : orders_) {
				if (order)
					order_pool_.free(order);
			}
			orders_.clear();

			clear_price_levels(side_t::BUY);
			clear_price_levels(side_t::SELL);

			bid_ = ask_ = nullptr;
		}break;
		default:break;
	}

	update_bbo(bid_updated, ask_updated);

	logger_->log("%:% %() % % %", __FILE__, __LINE__, __FUNCTION__,
		utils::get_curren_time_str(&time_str_), market_update->to_string(), bbo_.to_string());

	//trading_engine_->on_order_book_update(market_update->instrument_id_, market_update->price_, market_update->side_, this);
}

auto kse::example::trading::market_order_book::to_string(bool verbose, bool validity_check) const -> std::string
{
	std::stringstream ss;

	auto printer = [&](std::stringstream& ss, market_price_level* itr, models::side_t side, models::price_t& last_price,
		bool sanity_check) {
			models::quantity_t qty = 0;
			size_t num_orders = 0;

			// Calculate total quantity and number of orders
			for (auto o_itr = itr->first_order_;; o_itr = o_itr->next_order_) {
				qty += o_itr->qty_;
				++num_orders;
				if (o_itr->next_order_ == itr->first_order_)
					break;
			}

			// Print main price entry details
			ss << fmt::format(" <px:{} p:{} n:{}> {} @ {}({})",
				models::price_to_string(itr->price_),
				models::price_to_string(itr->prev_entry_->price_),
				models::price_to_string(itr->next_entry_->price_),
				models::price_to_string(itr->price_),
				models::quantity_to_string(qty),
				num_orders);

			// Print detailed order information if required
			for (auto o_itr = itr->first_order_;; o_itr = o_itr->next_order_) {
				if (verbose) {
					ss << fmt::format(" [oid:{} q:{} p:{} n:{}] ",
						models::order_id_to_string(o_itr->market_order_id_),
						models::quantity_to_string(o_itr->qty_),
						models::order_id_to_string(o_itr->prev_order_ ? o_itr->prev_order_->market_order_id_ : models::INVALID_ORDER_ID),
						models::order_id_to_string(o_itr->next_order_ ? o_itr->next_order_->market_order_id_ : models::INVALID_ORDER_ID));
				}
				if (o_itr->next_order_ == itr->first_order_)
					break;
			}

			ss << "\n";

			if (sanity_check) {
				if ((side == models::side_t::SELL && last_price >= itr->price_) || (side == models::side_t::BUY && last_price <= itr->price_)) {
					ss << fmt::format("Bids/Asks not sorted by ascending/descending prices last:{} itr:{}",
						models::price_to_string(last_price), itr->to_string());
				}
				last_price = itr->price_;
			}
		};

	ss << fmt::format("Ticker:{}\n", models::instrument_id_to_string(instrument_id_));

	// Process asks
	{
		auto ask_itr = ask_;
		auto last_ask_price = std::numeric_limits<models::price_t>::min();
		for (size_t count = 0; ask_itr; ++count) {
			ss << fmt::format("ASKS L:{} => ", count);
			auto next_ask_itr = (ask_itr->next_entry_ == ask_ ? nullptr : ask_itr->next_entry_);
			printer(ss, ask_itr, models::side_t::SELL, last_ask_price, validity_check);
			ask_itr = next_ask_itr;
		}
	}

	ss << "\n                          X\n\n";

	// Process bids
	{
		auto bid_itr = ask_;
		auto last_bid_price = std::numeric_limits<models::price_t>::max();
		for (size_t count = 0; bid_itr; ++count) {
			ss << fmt::format("BIDS L:{} => ", count);
			auto next_bid_itr = (bid_itr->next_entry_ == bid_ ? nullptr : bid_itr->next_entry_);
			printer(ss, bid_itr, models::side_t::BUY, last_bid_price, validity_check);
			bid_itr = next_bid_itr;
		}
	}

	return ss.str();
}
