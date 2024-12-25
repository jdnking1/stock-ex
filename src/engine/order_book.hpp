#pragma once


#include <array>
#include <functional>
#include <memory>
#include <string>
#include "models/constants.hpp"
#include "models/client_response.hpp"
#include "models/market_update.hpp"
#include "models/order.hpp"

#include "utils/logger.hpp"
#include "utils/memory_pool.hpp"
#include "utils/utils.hpp"

#include "message_handler.hpp"


namespace kse::engine {
	class order_book {
	public:
		explicit order_book(models::instrument_id_t instrument_id, utils::logger* logger, message_handler* message_handler);

		~order_book();

		order_book(const order_book&) = delete;
		order_book(order_book&&) = delete;

		order_book& operator=(const order_book&) = delete;
		order_book& operator=(order_book&&) = delete;

		auto add(models::client_id_t client_id, models::order_id_t client_order_id, models::side_t side, models::price_t price, models::quantity_t quantity) noexcept -> void;
		auto cancel(models::client_id_t client_id, models::order_id_t client_order_id) noexcept -> void;
		auto modify(models::client_id_t client_id, models::order_id_t client_order_id, models::price_t price, models::quantity_t quantity) noexcept -> void;
		auto to_string(bool verbose = false, bool validity_check=true) const -> std::string;

		auto get_client_response() const noexcept -> const models::client_response_internal& { return client_response_; }
		auto get_market_update() const noexcept -> const models::market_update& { return market_update_; }

	private:
		models::instrument_id_t instrument_id_ = models::INVALID_INSTRUMENT_ID;
		message_handler* message_handler_ = nullptr;

		models::client_order_map client_orders_{ };

		utils::memory_pool<models::price_level> price_level_pool_;
		models::price_level *bid_ = nullptr;
		models::price_level *ask_ = nullptr;
		models::order_at_price_level_map orders_at_price_levels_{};

		utils::memory_pool<models::order> order_pool_;
		models::client_response_internal client_response_;
		models::market_update market_update_;

		models::order_id_t next_market_order_id_ = 1;

		std::string time_str_;
		utils::logger* logger_ = nullptr;
	
	private:
		auto match(models::client_id_t client_id, models::side_t side, models::order_id_t client_order_id, models::order_id_t market_order_id, models::quantity_t leaves_qty, models::order& order_to_match_with) noexcept -> models::quantity_t;
		auto check_for_match(models::client_id_t client_id, models::order_id_t client_order_id, models::order_id_t new_market_order_id, models::side_t side, models::price_t price, models::quantity_t qty) noexcept -> models::quantity_t;

		auto get_new_market_order_id() noexcept -> models::order_id_t { 
			return next_market_order_id_++; 
		}
		auto price_to_index(models::price_t price) const noexcept { return price % models::MAX_PRICE_LEVELS; }
		auto get_orders_at_price_level(models::price_t price) const noexcept -> models::price_level* { return orders_at_price_levels_.at(price_to_index(price)); }
		auto get_order_priority_at_price_level(models::price_t price) const noexcept -> models::priority_t {
			const auto* orders_at_price_level = get_orders_at_price_level(price);
			return orders_at_price_level ? orders_at_price_level->first_order_->priority_ + 1 : 1;
		}

		auto add_price_level(models::price_level* new_price_level) noexcept -> void {
			orders_at_price_levels_.at(price_to_index(new_price_level->price_)) = new_price_level;

			auto*& best_price_level = new_price_level->side_ == models::side_t::BUY ? bid_ : ask_;


			const auto comparison_func = [](models::price_t a, models::price_t b, models::side_t side) {
				return side == models::side_t::BUY ? a > b : a < b;
			};

			const auto add_after = [](models::price_level* current, models::price_level* new_price_level) noexcept {
				new_price_level->next_entry_ = current->next_entry_;
				new_price_level->prev_entry_ = current;
				current->next_entry_->prev_entry_ = new_price_level;
				current->next_entry_ = new_price_level;
			};

			const auto add_before = [](models::price_level* current, models::price_level* new_price_level) noexcept {
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
			auto *orders_at_price_level = get_orders_at_price_level(price);
			auto*& best_price_level = side == models::side_t::BUY ? bid_ : ask_;

			if (orders_at_price_level->next_entry_ == orders_at_price_level) {
				best_price_level = nullptr;
			}
			else {
				orders_at_price_level->prev_entry_->next_entry_ = orders_at_price_level->next_entry_;
				orders_at_price_level->next_entry_->prev_entry_ = orders_at_price_level->prev_entry_;
				if (best_price_level == orders_at_price_level) {
					best_price_level = orders_at_price_level->next_entry_;
				}

				orders_at_price_level->next_entry_ = orders_at_price_level->prev_entry_ = nullptr;
			}

			orders_at_price_levels_.at(price_to_index(price)) = nullptr;
			price_level_pool_.free(orders_at_price_level);
		}

		auto add_order(models::order* order) noexcept -> void { 
			const auto* orders_at_price_level = get_orders_at_price_level(order->price_);

			if (!orders_at_price_level) {
				order->next_order_ = order->prev_order_ = order;

				auto new_price_level = price_level_pool_.alloc(order->side_, order->price_, order, nullptr, nullptr);

				add_price_level(new_price_level);
			}
			else {
				auto* old_last_order_at_price_level = orders_at_price_level->first_order_->prev_order_;
				old_last_order_at_price_level->next_order_ = order;
				order->prev_order_ = old_last_order_at_price_level;
				order->next_order_ = orders_at_price_level->first_order_;
				orders_at_price_level->first_order_->prev_order_ = order;	
			}

			client_orders_.at(order->client_id_).at(order->client_order_id_) = order;
		}

		auto remove_order(models::order* order) noexcept -> void {
			auto* orders_at_price_level = get_orders_at_price_level(order->price_);

			if (order->prev_order_ == order) {
				remove_order_at_price(order->side_, order->price_);
			}
			else {
				const auto order_before = order->prev_order_;
				const auto order_after = order->next_order_;

				order_before->next_order_ = order_after;
				order_after->prev_order_ = order_before;

				if(orders_at_price_level->first_order_ == order) {
					orders_at_price_level->first_order_ = order_after;
				}

				order->prev_order_ = order->next_order_ = nullptr;
			}

			client_orders_.at(order->client_id_).at(order->client_order_id_) = nullptr;
			order_pool_.free(order);
		}

	};

	using order_book_map = std::array<std::unique_ptr<order_book>, models::MAX_NUM_INSTRUMENTS>;
}


