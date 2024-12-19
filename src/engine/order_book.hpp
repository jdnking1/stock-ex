#pragma once


#include "order.hpp"

#include <list>
#include <map>
#include <string>
#include <mutex>

namespace engine {
	class OrderBook {

	public:
		OrderBook() = default;

		explicit OrderBook(std::string symb, std::vector<std::string>* output) : symbol{ std::move(symb) }, engineOutput{ output } {
			book[common::Side::BUY] = {};
			book[common::Side::SELL] = {};
		}

		~OrderBook() = default;

		OrderBook(OrderBook const&) = delete;
		OrderBook& operator=(OrderBook const&) = delete;

		OrderBook(OrderBook&&) = default;
		OrderBook& operator=(OrderBook&&) = default;

		void insert(Order const& order);

		void amend(const std::string& id, common::CustomFloat newPrice, size_t newVolume);

		void pull(const std::string& id, bool popFront = true);

		void processOrders(Order& order, std::list<Order*>& orders);

		void match(Order& order);

		void generateOutput();

	private:
		using PriceOrderMap = std::map<common::CustomFloat, std::list<Order*>>;

		std::map<common::Side, PriceOrderMap> book;
		std::map<std::string, Order> idOrderMap;

		std::string symbol;

		std::vector<std::string>* engineOutput;
	};
}
