#pragma once


#include <array>
#include <memory>
#include <string>

#include "models/constants.hpp"
#include "models/market_update.hpp"

#include "utils/logger.hpp"
#include "utils/memory_pool.hpp"
#include "utils/utils.hpp"

#include "market_order.hpp"


namespace kse::example::trading {
	class trade_engine;

	class market_order_book {
	public:
		explicit market_order_book(models::instrument_id_t instrument_id, utils::logger* logger);

		~market_order_book();

		market_order_book(const market_order_book&) = delete;
		market_order_book(market_order_book&&) = delete;

		market_order_book& operator=(const market_order_book&) = delete;
		market_order_book& operator=(market_order_book&&) = delete;

		auto on_market_update(const models::market_update* market_update) noexcept -> void;

		auto set_trade_engine(trade_engine* te) {
			trade_engine_ = te;
		}

		auto update_bbo(bool updateBid, bool updateAsk) noexcept  -> void  {
			if (updateBid) {
				update_side(bbo_.bid_price_, bbo_.bid_qty_, bid_);
			}

			if (updateAsk) {
				update_side(bbo_.ask_price_, bbo_.ask_qty_, ask_);
			}
		}

		auto get_bbo() const noexcept -> const bbo_t& {
			return bbo_;
		}

		auto to_string(bool verbose = false, bool validity_check = true) const->std::string;

	private:
		models::instrument_id_t instrument_id_ = models::INVALID_INSTRUMENT_ID;
		trade_engine* trade_engine_ = nullptr;

		market_order_map orders_{};

		utils::memory_pool<market_price_level> price_level_pool_;
		market_price_level* bid_ = nullptr;
		market_price_level* ask_ = nullptr;
		market_order_at_price_level_map price_levels_{};

		utils::memory_pool<market_order> order_pool_;

		bbo_t bbo_;

		std::string time_str_;
		utils::logger* logger_ = nullptr;

	private:
		auto price_to_index(models::price_t price) const noexcept { return price % models::MAX_PRICE_LEVELS; }
		auto get_price_level(models::price_t price) const noexcept -> market_price_level* { return price_levels_.at(price_to_index(price)); }

		auto update_side(models::price_t& price, models::quantity_t& quantity, market_price_level* level) -> void {
			if (level) {
				price = level->price_;
				quantity = level->first_order_->qty_;
				for (auto order = level->first_order_->next_order_; order != level->first_order_; order = order->next_order_) {
					quantity += order->qty_;
				}
			}
			else {
				price = models::INVALID_PRICE;
				quantity = models::INVALID_QUANTITY;
			}
		}

		auto clear_price_levels(models::side_t side) noexcept -> void {
			auto* best_price_level = side == models::side_t::BUY ? bid_ : ask_;
			auto* p = best_price_level->next_entry_;
			while (p != best_price_level) {
				auto* next = p->next_entry_;
				price_level_pool_.free(p);
				p = next;
			}

			price_level_pool_.free(best_price_level);
		}

		auto add_price_level(market_price_level* new_price_level) noexcept -> void {
			price_levels_.at(price_to_index(new_price_level->price_)) = new_price_level;

			auto*& best_price_level = new_price_level->side_ == models::side_t::BUY ? bid_ : ask_;


			const auto comparison_func = [](models::price_t a, models::price_t b, models::side_t side) {
				return side == models::side_t::BUY ? a > b : a < b;
				};

			const auto add_after = [](market_price_level* current, market_price_level* new_price_level) noexcept {
				new_price_level->next_entry_ = current->next_entry_;
				new_price_level->prev_entry_ = current;
				current->next_entry_->prev_entry_ = new_price_level;
				current->next_entry_ = new_price_level;
				};

			const auto add_before = [](market_price_level* current, market_price_level* new_price_level) noexcept {
				new_price_level->prev_entry_ = current->prev_entry_;
				new_price_level->next_entry_ = current;
				current->prev_entry_->next_entry_ = new_price_level;
				current->prev_entry_ = new_price_level;
				};

			if (!best_price_level) [[unlikely]] {
				best_price_level = new_price_level;
				best_price_level->next_entry_ = best_price_level->prev_entry_ = best_price_level;
			}
			else if (comparison_func(new_price_level->price_, best_price_level->price_, new_price_level->side_)) {
				auto* old_best_price_level = best_price_level;
				add_before(old_best_price_level, new_price_level);
				best_price_level = new_price_level;
			}
			else {
				auto add_after_flag = false;
				auto current_price_level = best_price_level->next_entry_;

				while (!add_after_flag && !comparison_func(new_price_level->price_, current_price_level->price_, new_price_level->side_)) {
					current_price_level = current_price_level->next_entry_;
					if (current_price_level->next_entry_ == best_price_level)
						add_after_flag = true;
				}

				if (add_after_flag) {
					add_after(current_price_level, new_price_level);
				}
				else {
					add_before(current_price_level, new_price_level);
				}
			}
		}

		auto remove_order_at_price(models::side_t side, models::price_t price) noexcept -> void {
			auto* price_level = get_price_level(price);
			auto*& best_price_level = side == models::side_t::BUY ? bid_ : ask_;

			if (price_level->next_entry_ == price_level) {
				best_price_level = nullptr;
			}
			else {
				price_level->prev_entry_->next_entry_ = price_level->next_entry_;
				price_level->next_entry_->prev_entry_ = price_level->prev_entry_;
				if (best_price_level == price_level) {
					best_price_level = price_level->next_entry_;
				}

				price_level->next_entry_ = price_level->prev_entry_ = nullptr;
			}

			price_levels_.at(price_to_index(price)) = nullptr;
			price_level_pool_.free(price_level);
		}

		auto add_order(market_order* order) noexcept -> void {
			const auto* price_level = get_price_level(order->price_);

			if (!price_level) {
				order->next_order_ = order->prev_order_ = order;

				auto new_price_level = price_level_pool_.alloc(order->side_, order->price_, order, nullptr, nullptr);

				add_price_level(new_price_level);
			}
			else {
				auto* old_last_order_at_price_level = price_level->first_order_->prev_order_;
				old_last_order_at_price_level->next_order_ = order;
				order->prev_order_ = old_last_order_at_price_level;
				order->next_order_ = price_level->first_order_;
				price_level->first_order_->prev_order_ = order;
			}

			orders_.at(order->market_order_id_) = order;
		}

		auto remove_order(market_order* order) noexcept -> void {
			auto* price_level = get_price_level(order->price_);

			if (order->prev_order_ == order) {
				remove_order_at_price(order->side_, order->price_);
			}
			else {
				const auto order_before = order->prev_order_;
				const auto order_after = order->next_order_;

				order_before->next_order_ = order_after;
				order_after->prev_order_ = order_before;

				if (price_level->first_order_ == order) {
					price_level->first_order_ = order_after;
				}

				order->prev_order_ = order->next_order_ = nullptr;
			}

			orders_.at(order->market_order_id_) = nullptr;
			order_pool_.free(order);
		}

	};

	using order_book_map = std::array<std::unique_ptr<market_order_book>, models::MAX_NUM_INSTRUMENTS>;
}


