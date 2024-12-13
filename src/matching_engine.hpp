#pragma once

#include "order_book.hpp"
#include "order.hpp"

#include <map>
#include <vector>

namespace engine {
    class Engine {
    public:
        void handleRequest(std::vector<std::string> req);

        void insert(std::vector<std::string> req);

        void amend(std::vector<std::string> req);

        void pull(std::vector<std::string> req);

        void run(std::vector<std::string> const& inputs);

        std::vector<std::string> generateOutput();

    private:
        std::map<std::string, OrderBook*> m_orderIDToOrderBookMap;
        std::map<std::string, OrderBook> m_symbolToOrderBookMap;
        std::vector<std::string> m_output;
    };
}
