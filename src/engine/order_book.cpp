#include "order_book.hpp"


namespace engine {

    void OrderBook::match(Order& order) {
        auto& otherSide = order.side == common::Side::BUY ? book[common::Side::SELL] : book[common::Side::BUY];
        auto priceCondition = [&order](float price) {
            return order.side == common::Side::BUY ? price <= order.price.value : price >= order.price.value;
            };

        auto process = [&](auto begin, auto end) {
            for (auto it = begin; it != end; ++it) {
                auto& [price, orders] = *it;
                if (priceCondition(price.value)) {
                    processOrders(order, orders);

                    if (order.volume == 0) {
                        pull(order.id);
                        break;
                    }
                }
                else {
                    break;
                }
            }
            };

        if (order.side == common::Side::BUY) {
            process(otherSide.begin(), otherSide.end());
        }
        else {
            process(otherSide.rbegin(), otherSide.rend());
        }
    }

    void OrderBook::processOrders(Order& order, std::list<Order*>& orders) {
        while (!orders.empty() && order.volume > 0) {
            auto* matchingOrder = orders.front();
            auto tradeVolume = std::min(order.volume, matchingOrder->volume);

            order.volume -= tradeVolume;
            matchingOrder->volume -= tradeVolume;

            auto outputLine = symbol + "," + matchingOrder->price.strValue + "," + std::to_string(tradeVolume) + ","
                + order.id + "," + matchingOrder->id;

            engineOutput->push_back(outputLine);

            if (matchingOrder->volume == 0) {
                pull(matchingOrder->id);
            }
        }
    }

    void OrderBook::pull(const std::string& id, bool popFront) {
        auto it = idOrderMap.find(id);

        if (it != idOrderMap.end()) {
            auto& order = it->second;
            auto& priceOrderMap = book[order.side];
            auto& ordersAtPrice = priceOrderMap[order.price];

            if (popFront) {
                ordersAtPrice.pop_front();
            }
            else {
                ordersAtPrice.remove_if([id](auto elm) {
                    return elm->id == id;
                    });
            }

            idOrderMap.erase(it);
        }

        
    }

    void OrderBook::amend(const std::string& id, common::CustomFloat newPrice, size_t newVolume) {
        auto it = idOrderMap.find(id);
        if (it != idOrderMap.end()) {
            auto& order = it->second;

            if (newPrice.strValue != order.price.strValue || newVolume > order.volume) {
                auto newOrder = order;

                newOrder.price = newPrice;

                newOrder.volume = newVolume;

                pull(id, false);
                insert(newOrder);
            }
            else {
                order.volume = newVolume;
            }
        }
    }

    void OrderBook::insert(Order const& order) {
        auto id = order.id;
        auto [element, isInserted] = idOrderMap.try_emplace(id, order);

        if (isInserted) {
            auto& orderInserted = element->second;
            auto& priceOrderMap = book[orderInserted.side];
            priceOrderMap[orderInserted.price].push_back(&orderInserted);
            match(orderInserted);
        }
    }


    void OrderBook::generateOutput() {
        engineOutput->push_back("===" + symbol + "===");

        auto removeEmptyLists = [](PriceOrderMap& map) {
            for (auto it = map.begin(); it != map.end(); ) {
                if (it->second.empty()) {
                    it = map.erase(it);
                }
                else {
                    ++it;
                }
            }
            };

        auto& sells = book.at(common::Side::SELL);
        auto& buys = book.at(common::Side::BUY);

        removeEmptyLists(sells);
        removeEmptyLists(buys);

        auto format = [](auto itr) -> std::string {
            const auto& orders = itr->second;
            auto price = itr->first;

            size_t volume = 0;

            for (const auto* order : orders) {
                volume += order->volume;
            }

            return price.strValue + ',' + std::to_string(volume);
            };

        auto sellsItr = sells.begin();
        auto buysItr = buys.rbegin();

        auto endBuys = buys.rend();
        auto endSells = sells.end();

        while (buysItr != endBuys || sellsItr != endSells) {
            auto buyPart = buysItr != buys.rend() ? format(buysItr) : ",";
            auto sellPart = sellsItr != sells.end() ? format(sellsItr) : ",";

            auto line = buyPart + "," + sellPart;

            if (line != ",,,") {
                engineOutput->push_back(line);
            }


            if (buysItr != endBuys) {
                buysItr = ++buysItr;
            }
            if (sellsItr != endSells) {
                sellsItr = ++sellsItr;
            }
        }
    }
}
