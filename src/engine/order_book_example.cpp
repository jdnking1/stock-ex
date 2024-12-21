#include <iostream>
#include "order_book.hpp"

using namespace kse::engine;
using namespace kse::utils;
using namespace kse::models;

int main() {
    kse::utils::logger lg{ "order_book_example.log" };
    kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
    kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
    kse::engine::message_handler hdlr{ &client_responses, &market_updates, &lg };

    std::unique_ptr<order_book> ob = std::make_unique<order_book>(static_cast<instrument_id_t>(1), &lg, &hdlr);

    ob->add(1, 101, side_t::BUY, 1000, 50); // Client 1, order 101, BUY 1000 @ 50
    ob->add(2, 201, side_t::SELL, 1020, 20); //Client 2, order 201, SELL 1020 @ 20
    ob->add(3, 301, side_t::BUY, 990, 70);  // Client 3, order 301, BUY 990 @ 70
    ob->add(4, 401, side_t::SELL, 1050, 10); //Client 4, order 401, SELL 1050 @ 10

    ob->add(5, 501, side_t::BUY, 1000, 30);  //Client 5, order 501, BUY 1000 @ 30
    ob->add(6, 601, side_t::SELL, 1020, 50); //Client 6, order 601, SELL 1020 @ 50
    ob->add(7, 701, side_t::BUY, 980, 40);   //Client 7, order 701, BUY 980 @ 40
    ob->add(8, 801, side_t::SELL, 1040, 60); //Client 8, order 801, SELL 1040 @ 60

    ob->modify(1, 101, 995, 30);
    ob->modify(2, 201, 1020, 10);
    ob->cancel(2, 201);          

    std::string verbose_output = ob->to_string(true, true);
    std::string compact_output = ob->to_string(false, false);

    std::cout << "Verbose Order Book Output:" << std::endl;
    std::cout << verbose_output << std::endl;

    std::cout << "Compact Order Book Output:" << std::endl;
    std::cout << compact_output << std::endl;
}