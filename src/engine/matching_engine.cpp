#include "matching_engine.hpp"
#include "engine_utils.hpp"

#include <iostream>


namespace engine {
    void Engine::handleRequest(std::vector<std::string> req) {
        if (!req.empty()) {
            auto requestName = common::toRequestName(req[0]);

            switch (requestName) {
            case common::RequestsName::INSERT:
                insert(std::move(req));
                break;
            case common::RequestsName::AMEND:
                amend(std::move(req));
                break;
            case common::RequestsName::PULL:
                pull(std::move(req));
                break;
            default:
                std::cout << "invalid command";
            }
        }
    }

    void Engine::insert(std::vector<std::string> req) {
        if (req.size() != 6 || utils::invalidPriceOrQty(req[4], req[5])) {
            return;
        }

        auto order = makeOrder(req);
        auto symbol = std::move(req[2]);

        OrderBook* orderBook{};

        if (auto orderBookItr = m_symbolToOrderBookMap.find(symbol);
            orderBookItr != m_symbolToOrderBookMap.end()) {
            orderBook = &orderBookItr->second;
        }
        else {
            m_symbolToOrderBookMap.emplace(symbol, OrderBook(symbol, &m_output));
            orderBook = &m_symbolToOrderBookMap.at(symbol);
        }

        orderBook->insert(order);
        m_orderIDToOrderBookMap[order.id] = orderBook;
    }


    void Engine::amend(std::vector<std::string> req) {
        if (req.size() != 4 || utils::invalidPriceOrQty(req[2], req[3])) {
            return;
        }

        auto id = req[1];

        if (auto orderBookItr = m_orderIDToOrderBookMap.find(id);
            orderBookItr != m_orderIDToOrderBookMap.end()) {
            auto* orderBook = orderBookItr->second;
            auto newVolume = std::stoul(req[3]);
            common::CustomFloat newPrice{ std::stof(req[2]), req[2] };
            orderBook->amend(id, newPrice, newVolume);
        }
    }

    void Engine::pull(std::vector<std::string> req) {
        if (req.size() != 2) {
            return;
        }

        auto id = req[1];

        if (auto orderBookItr = m_orderIDToOrderBookMap.find(id);
            orderBookItr != m_orderIDToOrderBookMap.end()) {
            auto* orderBook = orderBookItr->second;
            orderBook->pull(id);
            m_orderIDToOrderBookMap.erase(orderBookItr);
        }
    }

    void Engine::run(std::vector<std::string> const& inputs) {
        for (const auto& input : inputs) {
            auto req = utils::parseInput(input);
            handleRequest(std::move(req));
        }
    }

    std::vector<std::string> Engine::generateOutput() {
        for (auto& [symbol, orderBook] : m_symbolToOrderBookMap) {
            orderBook.generateOutput();
        }


        return m_output;
    }
}