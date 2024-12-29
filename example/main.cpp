#include "feed_handler/feed_handler.hpp"
#include "order_gateway/order_gateway.hpp"

#include "trading_engine/market_order.hpp"
#include "trading_engine/market_order_book.hpp"

#include <iostream>
#include <chrono>

int main() {
	kse::models::market_update_queue updates(10000);
	const int sleep_time = 100 * 1000;
	std::string time_str{};

	auto logger = new kse::utils::logger("trading.log" + std::to_string((uintptr_t)&updates));

	kse::models::client_request_queue requests{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::client_response_queue responses{ kse::models::MAX_CLIENT_UPDATES };


	/*	inline std::string client_request_type_to_string(client_request_type type) {
		switch (type) {
		case client_request_type::NEW:
			return "NEW";
		case client_request_type::CANCEL:
			return "CANCEL";
		case client_request_type::MODIFY:
			return "MODIFY";
		case client_request_type::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct client_request_internal {
		client_request_type type_ = client_request_type::INVALID;

		client_id_t client_id_ = INVALID_CLIENT_ID;
		instrument_id_t instrument_id_ = INVALID_INSTRUMENT_ID;
		order_id_t order_id_ = INVALID_ORDER_ID;
		side_t side_ = side_t::INVALID;
		price_t price_ = INVALID_PRICE;
		quantity_t qty_ = INVALID_QUANTITY;

		auto to_string() const {
			std::stringstream ss;
			ss << "client_request_internal"
				<< " ["
				<< "type:" << client_request_type_to_string(type_)
				<< " client:" << client_id_to_string(client_id_)
				<< " instrument:" << instrument_id_to_string(instrument_id_)
				<< " oid:" << order_id_to_string(order_id_)
				<< " side:" << side_to_string(side_)
				<< " qty:" << quantity_to_string(qty_)
				<< " price:" << price_to_string(price_)
				<< "]";
			return ss.str();
		}
	};*/

	auto* next_write = requests.get_next_write_element();
	*next_write = {kse::models::client_request_type::NEW, 1, 1, 1, kse::models::side_t::BUY, 15, 100};
	requests.next_write_index();

	auto* next_write2 = requests.get_next_write_element();
	*next_write2 = { kse::models::client_request_type::NEW, 1, 2, 2, kse::models::side_t::BUY, 15, 100 };
	requests.next_write_index();

	auto* next_writex = requests.get_next_write_element();
	*next_writex = { kse::models::client_request_type::NEW, 1, 2, 2, kse::models::side_t::SELL, 10, 110 };
	requests.next_write_index();

	auto* next_write3 = requests.get_next_write_element();
	*next_write3 = { kse::models::client_request_type::MODIFY, 1, 1, 1, kse::models::side_t::BUY, 15, 20 };
	requests.next_write_index();

	//54321

	auto& feed_handler = kse::example::market_data::feed_handler::get_instance(&updates, "233.252.14.1", 54322, "233.252.14.3", 54323);
	feed_handler.start();

	auto& order_gateway = kse::example::gateway::order_gateway::get_instance(&requests, &responses, "127.0.0.1", 54321);
	order_gateway.start();

	using namespace std::literals::chrono_literals;

	while (true) {
		logger->log("%:% %() % Sleeping for a few milliseconds..\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
		std::this_thread::sleep_for(sleep_time * 1000ms);
	}

	return 0;
}




