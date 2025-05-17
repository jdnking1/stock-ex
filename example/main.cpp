#include "feed_handler/feed_handler.hpp"
#include "order_gateway/order_gateway.hpp"
#include "trading_engine/trade_engine.hpp"
#include <algorithm>
#include <iostream>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <random>

int main(int, char** argv) {
	const auto algo_type = kse::example::trading_utils::string_to_algo_type("RANDOM");

	const int sleep_time = 1000;

	std::string time_str{};

	auto logger = std::make_unique<kse::utils::logger>("trading" + std::to_string((uintptr_t)&sleep_time) + ".log");

	kse::models::client_request_queue requests{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::client_response_queue responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue updates{ kse::models::MAX_MARKET_UPDATES };

	trade_engine_config_map cfgs{};

	//kse::models::instrument_id_t next_instrument_id = 0;

	/*for (int i = 2; i < argc; i += 5, ++next_instrument_id) {
		cfgs.at(next_instrument_id) = { static_cast<kse::models::quantity_t>(std::atoi(argv[i])), std::atof(argv[i + 1]),
										 {static_cast<kse::models::quantity_t>(std::atoi(argv[i + 2])),
										  static_cast<kse::models::quantity_t>(std::atoi(argv[i + 3])),
										  std::atof(argv[i + 4])} };
	}*/

	auto& feed_handler = kse::example::market_data::feed_handler::get_instance(&updates, "233.252.14.1", 54322, "233.252.14.3", 54323);
	feed_handler.start();

	auto& order_gateway = kse::example::gateway::order_gateway::get_instance(&requests, &responses, "127.0.0.1", 54321);
	order_gateway.start();

	logger->log("%:% %() % Starting Trade Engine...\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
	auto trade_engine = std::make_unique<kse::example::trading::trade_engine>(algo_type, cfgs, &requests, &responses, &updates); 
	trade_engine->start();

	trade_engine->init_last_event_time();

	if (algo_type == algo_type_t::RANDOM) {
		kse::models::order_id_t order_id = std::atoi(argv[2]);
		std::vector<kse::models::client_request_internal> client_requests_vec;
		std::vector<kse::models::client_request_internal> below_market_buy_orders; // To track below-market buy orders
		std::array<kse::models::price_t, kse::models::MAX_NUM_INSTRUMENTS> instrument_base_price;

		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution dis_price(100, 199);
		std::uniform_int_distribution dis_below_price(50, 90);
		std::uniform_int_distribution dis_qty(1, 100);
		std::uniform_int_distribution dis_side(0, 99);
		std::uniform_int_distribution<kse::models::instrument_id_t> dis_instrument(0, kse::models::MAX_NUM_INSTRUMENTS - 1);
		std::uniform_int_distribution dis_offset(1, 10);
		std::uniform_int_distribution dis_probability(1, 100); // For 30% chance

		for (size_t i = 0; i < kse::models::MAX_NUM_INSTRUMENTS; ++i) {
			instrument_base_price[i] = dis_price(gen);
		}
		
		for (size_t i = 0; i < 100; ++i) {
			const kse::models::instrument_id_t instrument_id = static_cast<kse::models::instrument_id_t>(dis_instrument(gen));
			const kse::models::price_t trend_adjustment = (i % 2 == 0) ? 1 : -1;
			instrument_base_price[instrument_id] += trend_adjustment;
			const kse::models::price_t price = instrument_base_price[instrument_id] + dis_offset(gen);
			const kse::models::quantity_t qty = dis_qty(gen);

			const kse::models::side_t side = (dis_side(gen) < 50) ? kse::models::side_t::BUY : kse::models::side_t::SELL;

			std::cout << "Regular Order: " << order_id << std::endl;
			kse::models::client_request_internal new_request{
				kse::models::client_request_type::NEW,
				0, instrument_id, order_id++, side, price, qty
			};

			trade_engine->send_client_request(new_request);
			std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));

			client_requests_vec.push_back(new_request);

			// 30% chance to create a below-market-price buy order
			if (dis_probability(gen) <= 30) {
				const kse::models::price_t below_market_price = dis_below_price(gen) - dis_offset(gen); // Below market price
				const kse::models::quantity_t below_market_qty = dis_qty(gen);

				std::cout << "Below Market Buy Order: " << order_id << std::endl;
				kse::models::client_request_internal below_market_request{
					kse::models::client_request_type::NEW,
					0, instrument_id, order_id++, kse::models::side_t::BUY, below_market_price, below_market_qty
				};

				trade_engine->send_client_request(below_market_request);
				below_market_buy_orders.push_back(below_market_request);
			}

			// 30% chance to cancel an existing below-market-price buy order
			if (!below_market_buy_orders.empty() && dis_probability(gen) <= 30) {
				// Select the first below-market buy order for cancellation
				auto cancel_order = below_market_buy_orders.back();
				below_market_buy_orders.pop_back();

				kse::models::client_request_internal cancel_request{
					kse::models::client_request_type::CANCEL,
					cancel_order.client_id_, cancel_order.instrument_id_, cancel_order.order_id_, cancel_order.side_, 0, 0
				};

				std::cout << "Cancelling Below Market Buy Order: " << cancel_order.order_id_ << std::endl;
				trade_engine->send_client_request(cancel_request);
			}
		}
	}
	using namespace std::literals::chrono_literals;

	while (trade_engine->silent_seconds() < 60) {
		logger->log("%:% %() % Waiting till no activity, been silent for % seconds...\n", __FILE__, __LINE__, __func__,
			kse::utils::get_curren_time_str(&time_str), trade_engine->silent_seconds());

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(30s);
	}

	trade_engine->stop();
	
	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(10s);

	return 0;
}
