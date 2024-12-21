#include <gtest/gtest.h>

#include "engine/order_book.hpp"

#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "models/market_update.hpp"

using namespace kse::engine;
using namespace kse::models;
using namespace kse::utils;


TEST(OrderBookTest, AddBuyOrderNoMatch) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);
	client_id_t client_id = 1;
	order_id_t client_order_id = 1;
	side_t side = side_t::BUY;
	price_t price = 100;
	quantity_t quantity = 100;

	order_book->add(client_id, client_order_id, side, price, quantity);

	EXPECT_EQ(order_book->get_client_response().type_, client_response_type::ACCEPTED);
	EXPECT_EQ(order_book->get_client_response().client_id_ , client_id);
	EXPECT_EQ(order_book->get_client_response().instrument_id_, 1);
	EXPECT_EQ(order_book->get_client_response().client_order_id_, client_order_id);
	EXPECT_EQ(order_book->get_client_response().market_order_id_, 1);
	EXPECT_EQ(order_book->get_client_response().side_, side);
	EXPECT_EQ(order_book->get_client_response().price_, price);
	EXPECT_EQ(order_book->get_client_response().leaves_qty_, quantity);

	EXPECT_EQ(order_book->get_market_update().type_, market_update_type::ADD);
	EXPECT_EQ(order_book->get_market_update().order_id_, 1);
	EXPECT_EQ(order_book->get_market_update().instrument_id_, 1);
	EXPECT_EQ(order_book->get_market_update().side_, side);
	EXPECT_EQ(order_book->get_market_update().price_, price);
	EXPECT_EQ(order_book->get_market_update().qty_ , quantity);
}

TEST(OrderBookTest, AddSellOrderNoMatch) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);
	client_id_t client_id = 1;
	order_id_t client_order_id = 1;
	side_t side = side_t::SELL;
	price_t price = 100;
	quantity_t quantity = 100;

	order_book->add(client_id, client_order_id, side, price, quantity);

	EXPECT_EQ(order_book->get_client_response().type_, client_response_type::ACCEPTED);
	EXPECT_EQ(order_book->get_client_response().client_id_, client_id);
	EXPECT_EQ(order_book->get_client_response().instrument_id_, 1);
	EXPECT_EQ(order_book->get_client_response().client_order_id_, client_order_id);
	EXPECT_EQ(order_book->get_client_response().market_order_id_, 1);
	EXPECT_EQ(order_book->get_client_response().side_, side);
	EXPECT_EQ(order_book->get_client_response().price_, price);
	EXPECT_EQ(order_book->get_client_response().leaves_qty_, quantity);

	EXPECT_EQ(order_book->get_market_update().type_, market_update_type::ADD);
	EXPECT_EQ(order_book->get_market_update().order_id_, 1);
	EXPECT_EQ(order_book->get_market_update().instrument_id_, 1);
	EXPECT_EQ(order_book->get_market_update().side_, side);
	EXPECT_EQ(order_book->get_market_update().price_, price);
	EXPECT_EQ(order_book->get_market_update().qty_, quantity);
}

TEST(OrderBookTest, AddBuyOrderWithMatch) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);
	client_id_t client_id = 1;
	order_id_t client_order_id = 1;
	side_t side = side_t::BUY;
	price_t price = 100;
	quantity_t quantity = 100;

	order_book->add(2, 2, side_t::SELL, price, quantity);

	order_book->add(client_id, client_order_id, side, price + 1, quantity);

	EXPECT_EQ(order_book->get_client_response().type_, client_response_type::FILLED);
	EXPECT_EQ(order_book->get_client_response().client_id_, 2);
	EXPECT_EQ(order_book->get_client_response().instrument_id_, 1);
	EXPECT_EQ(order_book->get_client_response().client_order_id_, 2);
	EXPECT_EQ(order_book->get_client_response().market_order_id_, 1);
	EXPECT_EQ(order_book->get_client_response().side_, side_t::SELL);
	EXPECT_EQ(order_book->get_client_response().price_, price);
	EXPECT_EQ(order_book->get_client_response().leaves_qty_, 0);

	EXPECT_EQ(order_book->get_market_update().type_, market_update_type::CANCEL);
	EXPECT_EQ(order_book->get_market_update().order_id_, 1);
	EXPECT_EQ(order_book->get_market_update().instrument_id_, 1);
	EXPECT_EQ(order_book->get_market_update().side_, side_t::SELL);
	EXPECT_EQ(order_book->get_market_update().price_, price);
	EXPECT_EQ(order_book->get_market_update().qty_, quantity);
}

TEST(OrderBookTest, AddSellOrderWithMatch) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);
	client_id_t client_id = 1;
	order_id_t client_order_id = 1;
	side_t side = side_t::SELL;
	price_t price = 100;
	quantity_t quantity = 100;

	order_book->add(2, 2, side_t::BUY, price, quantity);

	order_book->add(client_id, client_order_id, side, price - 1, quantity);

	EXPECT_EQ(order_book->get_client_response().type_, client_response_type::FILLED);
	EXPECT_EQ(order_book->get_client_response().client_id_, 2);
	EXPECT_EQ(order_book->get_client_response().instrument_id_, 1);
	EXPECT_EQ(order_book->get_client_response().client_order_id_, 2);
	EXPECT_EQ(order_book->get_client_response().market_order_id_, 1);
	EXPECT_EQ(order_book->get_client_response().side_, side_t::BUY);
	EXPECT_EQ(order_book->get_client_response().price_, price);
	EXPECT_EQ(order_book->get_client_response().leaves_qty_, 0);

	EXPECT_EQ(order_book->get_market_update().type_, market_update_type::CANCEL);
	EXPECT_EQ(order_book->get_market_update().order_id_, 1);
	EXPECT_EQ(order_book->get_market_update().instrument_id_, 1);
	EXPECT_EQ(order_book->get_market_update().side_, side_t::BUY);
	EXPECT_EQ(order_book->get_market_update().price_, price);
	EXPECT_EQ(order_book->get_market_update().qty_, quantity);
}

TEST(OrderBookTest, MultipleOrdersWithPartialAndFullMatches) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);

	client_id_t client_id_1 = 1;
	order_id_t client_order_id_1 = 1;
	side_t side_1 = side_t::BUY;
	price_t price_1 = 102;
	quantity_t qty_1 = 100;

	order_book->add(client_id_1, client_order_id_1, side_1, price_1, qty_1);

	client_id_t client_id_2 = 2;
	order_id_t client_order_id_2 = 2;
	side_t side_2 = side_t::BUY;
	price_t price_2 = 101;
	quantity_t qty_2 = 150;

	order_book->add(client_id_2, client_order_id_2, side_2, price_2, qty_2);

	client_id_t client_id_3 = 3;
	order_id_t client_order_id_3 = 3;
	side_t side_3 = side_t::SELL;
	price_t price_3 = 100;
	quantity_t qty_3 = 50;

	order_book->add(client_id_3, client_order_id_3, side_3, price_3, qty_3);

	client_id_t client_id_4 = 4;
	order_id_t client_order_id_4 = 4;
	side_t side_4 = side_t::SELL;
	price_t price_4 = 100;
	quantity_t qty_4 = 200;

	order_book->add(client_id_4, client_order_id_4, side_4, price_4, qty_4);

	ASSERT_EQ(client_responses.size(), 10);  
	auto response = client_responses.get_next_read_element();
	EXPECT_EQ(response->client_id_, client_id_1);
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->client_id_, client_id_2);
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->client_id_, client_id_3);
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_3);
	EXPECT_EQ(response->leaves_qty_, 0);
	client_responses.next_read_index();
	
	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_1);
	EXPECT_EQ(response->leaves_qty_, 50);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->client_id_, client_id_4);
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_4);
	EXPECT_EQ(response->leaves_qty_, 150);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_1);
	EXPECT_EQ(response->leaves_qty_, 0);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_4);
	EXPECT_EQ(response->leaves_qty_, 0);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_2);
	EXPECT_EQ(response->leaves_qty_, 0);
	client_responses.next_read_index();

	
	ASSERT_EQ(market_updates.size(), 8);
	auto market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::ADD);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::ADD);
	market_updates.next_read_index(); 

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::TRADE);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::MODIFY);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::TRADE);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::CANCEL);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::TRADE);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::CANCEL);
	market_updates.next_read_index();
}

TEST(OrderBookTest, ModifyOrderAndMatch) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);

	// Adding initial BUY order
	client_id_t client_id_1 = 1;
	order_id_t client_order_id_1 = 1;
	side_t side_1 = side_t::BUY;
	price_t price_1 = 100;
	quantity_t qty_1 = 100;

	order_book->add(client_id_1, client_order_id_1, side_1, price_1, qty_1);

	// Adding initial SELL order
	client_id_t client_id_2 = 2;
	order_id_t client_order_id_2 = 2;
	side_t side_2 = side_t::SELL;
	price_t price_2 = 101;
	quantity_t qty_2 = 50;

	order_book->add(client_id_2, client_order_id_2, side_2, price_2, qty_2);

	// Modify BUY order to match with SELL order
	quantity_t new_qty_1 = 150;
	order_book->modify(client_id_1, client_order_id_1, price_1, new_qty_1);

	// Adding another SELL order to match with modified BUY order
	client_id_t client_id_3 = 3;
	order_id_t client_order_id_3 = 3;
	side_t side_3 = side_t::SELL;
	price_t price_3 = 99;
	quantity_t qty_3 = 75;

	order_book->add(client_id_3, client_order_id_3, side_3, price_3, qty_3);

	ASSERT_EQ(client_responses.size(), 7);
	
	auto response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::CANCELED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::ACCEPTED);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_3);
	EXPECT_EQ(response->leaves_qty_, 0);
	client_responses.next_read_index();

	response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::FILLED);
	EXPECT_EQ(response->client_id_, client_id_1);
	EXPECT_EQ(response->leaves_qty_, 75);
	client_responses.next_read_index();
	
	ASSERT_EQ(market_updates.size(), 6);
	auto market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::ADD);
	market_updates.next_read_index();

	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::ADD);
	market_updates.next_read_index();


	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::CANCEL);
	market_updates.next_read_index();


	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::ADD);
	market_updates.next_read_index();


	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::TRADE);
	market_updates.next_read_index();


	market_update = market_updates.get_next_read_element();
	EXPECT_EQ(market_update->type_, market_update_type::MODIFY);
	market_updates.next_read_index();
} 


TEST(OrderBookTest, CancelNonExistentOrder) {
	auto loggerq = kse::utils::logger("order_book_test.log");

	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };
	auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);

	client_id_t client_id = 1;
	order_id_t client_order_id = 1;

	order_book->cancel(client_id, client_order_id);

	ASSERT_EQ(client_responses.size(), 1);
	auto response = client_responses.get_next_read_element();
	EXPECT_EQ(response->type_, client_response_type::CANCEL_REJECTED);
	EXPECT_EQ(response->client_id_, client_id);
}
