#include "feed_handler/feed_handler.hpp"

#include <iostream>
#include <chrono>

int main() {
	kse::models::market_update_queue updates(10000);
	const int sleep_time = 100 * 1000;
	std::string time_str{};

	auto logger = new kse::utils::logger("trading.log" + std::to_string((uintptr_t)&updates));

	auto& feed_handler = kse::example::market_data::feed_handler::get_instance(&updates, "233.252.14.1", 54322, "233.252.14.3", 54323);
	feed_handler.start();

	using namespace std::literals::chrono_literals;

	while (true) {
		logger->log("%:% %() % Sleeping for a few milliseconds..\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
		std::this_thread::sleep_for(sleep_time * 1000ms);
	}

	return 0;
}




