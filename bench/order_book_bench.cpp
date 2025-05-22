#include "utils/utils.hpp"
#include <engine/order_book.hpp>

#include <random>




auto launch() -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	std::random_device rd;
	std::mt19937 gen(rd());

	std::uniform_int_distribution<> dis_price(100, 199);
	std::uniform_int_distribution<> dis_qty(1, 100);
	std::uniform_int_distribution<> dis_action(0, 99); // 0–49 = add, 50–99 = match

	using OrderInfo = std::tuple<kse::models::order_id_t, kse::models::side_t, kse::models::price_t>;
	std::vector<OrderInfo> resting_orders;

	kse::models::order_id_t next_order_id = 0;

	auto start = kse::utils::rdtsc();

	for (int i = 0; i < 1'000'000; ++i) {
		int action = dis_action(gen);

		if (action < 50 || resting_orders.empty()) {
			// ADD: create a non-marketable order to grow the book
			kse::models::side_t side = (i % 2 == 0) ? kse::models::side_t::BUY : kse::models::side_t::SELL;
			kse::models::price_t price = (side == kse::models::side_t::BUY) ? dis_price(gen) - 10
				: dis_price(gen) + 10;
			kse::models::quantity_t qty = dis_qty(gen);

			order_book->add(i % 10, next_order_id, side, price, qty);

			resting_orders.emplace_back(next_order_id, side, price);
			++next_order_id;

		}
		else {
			auto [rest_id, rest_side, rest_price] = resting_orders.back();
			resting_orders.pop_back();

			kse::models::side_t match_side = (rest_side == kse::models::side_t::BUY)
				? kse::models::side_t::SELL
				: kse::models::side_t::BUY;

			// Submit marketable price
			kse::models::price_t match_price = rest_price;

			kse::models::quantity_t qty = dis_qty(gen); // could be partial or full

			order_book->add(1, next_order_id, match_side, match_price, qty);
			++next_order_id;
		}
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}

auto add_only_benchmark(int n_orders) -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis_price(150, 160);  // Narrow range so no crossing
	std::uniform_int_distribution<> dis_qty(1, 100);
	kse::models::side_t side = kse::models::side_t::BUY;  // Only buy side

	auto start = kse::utils::rdtsc();

	for (int i = 0; i < n_orders; ++i) {
		auto price = dis_price(gen);
		auto qty = dis_qty(gen);
		order_book->add(i % 10, i, side, price, qty);
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}

auto match_heavy_benchmark(int n_orders) -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis_qty(1, 100);

	kse::models::price_t base_price = 100;

	auto start = kse::utils::rdtsc();

	for (int i = 0; i < n_orders; i += 2) {
		// Add resting buy order
		order_book->add(i % 10, i, kse::models::side_t::BUY, base_price, 50);
		// Add matching sell order crossing the spread
		order_book->add((i + 1) % 10, i + 1, kse::models::side_t::SELL, base_price, 50);
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}

auto deep_book_benchmark(int n_orders) -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis_qty(1, 100);
	// Large price range
	std::uniform_int_distribution<> dis_price(50, 200);

	auto start = kse::utils::rdtsc();

	for (int i = 0; i < n_orders; ++i) {
		auto price = dis_price(gen);
		auto qty = dis_qty(gen);
		auto side = (i % 2 == 0) ? kse::models::side_t::BUY : kse::models::side_t::SELL;
		order_book->add(i % 10, i, side, price, qty);
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}


auto worst_case_matching_benchmark(int n_orders) -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	kse::models::price_t test_price = 100;
	kse::models::side_t resting_side = kse::models::side_t::BUY;
	kse::models::side_t matching_side = kse::models::side_t::SELL;

	// 1) Add many resting buy orders at same price to build a long queue
	for (int i = 0; i < n_orders; ++i) {
		order_book->add(i % 10, i, resting_side, test_price, 1);
	}

	auto start = kse::utils::rdtsc();

	// 2) Submit matching sell orders that partially fill many resting orders, forcing deep linked-list traversal
	for (int i = 0; i < n_orders; ++i) {
		order_book->add((n_orders + i) % 10, n_orders + i, matching_side, test_price, 50000);
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}


auto mixed_workload_benchmark(int n_ops) -> double {
	kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

	auto order_book = std::make_unique<kse::engine::order_book>(static_cast<kse::models::instrument_id_t>(1), &loggerq, &message_handlers);

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis_price(90, 110);
	std::uniform_int_distribution<> dis_qty(1, 100);
	std::uniform_int_distribution<> dis_op(1, 100);  // 1-70: add, 71-90: cancel, 91-100: modify (for example)

	// Simple order ID tracking
	std::vector<int> active_orders;

	auto start = kse::utils::rdtsc();

	for (int i = 0; i < n_ops; ++i) {
		int op_choice = dis_op(gen);
		if (op_choice <= 70) {
			// Add order
			auto price = dis_price(gen);
			auto qty = dis_qty(gen);
			auto side = (i % 2 == 0) ? kse::models::side_t::BUY : kse::models::side_t::SELL;
			order_book->add(1, i, side, price, qty);
			active_orders.push_back(i);
		}
		else if (op_choice <= 90 && !active_orders.empty()) {
			// Cancel random order
			int idx = std::uniform_int_distribution<int>(0, active_orders.size() - 1)(gen);
			int cancel_id = active_orders[idx];
			order_book->cancel(1, cancel_id); 
			std::swap(active_orders[idx], active_orders.back());
			active_orders.pop_back();
		}
		else if (!active_orders.empty()) {
			// Modify random order
			int idx = std::uniform_int_distribution<int>(0, active_orders.size() - 1)(gen);
			int mod_id = active_orders[idx];
			auto new_qty = dis_qty(gen);
			auto new_price = dis_price(gen);
			order_book->modify(1, mod_id, new_price, new_qty);
		}
	}

	auto end = kse::utils::rdtsc();
	return (end - start) / 2.30;
}



void run_all_benchmarks(int runs, int n_orders) {
	std::ofstream out("benchmark_results.csv");
	out << "Run,Scenario,Time_ns\n";

	for (int i = 0; i < runs; ++i) {
		double t_balanced = launch();
		out << i + 1 << ",50%ADD50%Match," << std::fixed << std::setprecision(0) << t_balanced << "\n";

		double t_add = add_only_benchmark(n_orders);
		out << i + 1 << ",AddOnly," << std::fixed << std::setprecision(0) << t_add << "\n";

		double t_match = match_heavy_benchmark(n_orders);
		out << i + 1 << ",MatchHeavy," << t_match << "\n";

		double t_deep = deep_book_benchmark(n_orders);
		out << i + 1 << ",DeepBook," << t_deep << "\n";

		double t_worst = worst_case_matching_benchmark(n_orders / 2);
		out << i + 1 << ",WorstMatch," << t_worst << "\n";
	}

	out.close();
	std::cout << "Logged to benchmark_results.csv\n";
}



int main() {
	int runs = 10;
	int n_orders = 1000000;
	run_all_benchmarks(runs, n_orders);
	return 0;
}