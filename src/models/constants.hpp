#pragma once 

#include <cstddef>

namespace kse::models {	
	/// Maximum instrustruments traded on the exchange.
	constexpr size_t MAX_NUM_INSTRUMENTS = 8;

	/// Maximum number of client requests and responses
	constexpr size_t MAX_CLIENT_UPDATES = 256 * 1024;

	/// Maximum number of market updates between components.
	constexpr size_t MAX_MARKET_UPDATES = 256 * 1024;

	/// Maximum trading clients.
	constexpr size_t MAX_NUM_CLIENTS = 10;

	/// Maximum number of orders
	constexpr size_t MAX_NUM_ORDERS = 10000002;

	/// Maximum price level depth in the order books.
	constexpr size_t MAX_PRICE_LEVELS = 256;
}