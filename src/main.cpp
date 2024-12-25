#include "order_server/order_server.hpp"

#include "engine/matching_engine.hpp"
#include <csignal>
#include <iostream>


kse::utils::logger* logger = nullptr;
kse::engine::matching_engine* matching_engine = nullptr;

void signal_handler(int) {
    using namespace std::literals::chrono_literals;
    std::this_thread::sleep_for(10s);

    delete logger; logger = nullptr;
    delete matching_engine; matching_engine = nullptr;

    std::this_thread::sleep_for(10s);

    exit(EXIT_SUCCESS);
}

int main() {
    logger = new kse::utils::logger("kse.log");

    std::signal(SIGINT, signal_handler);

    const int sleep_time = 100 * 1000;

    kse::models::client_request_queue client_requests{ kse::models::MAX_CLIENT_UPDATES };
    kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
    kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };

    std::string time_str;

    logger->log("%:% %() % Starting Matching Engine...\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
    matching_engine = new kse::engine::matching_engine(&client_requests, &client_responses, &market_updates);
    matching_engine->start();

    logger->log("%:% %() % Starting Order Server...\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
    auto& server = kse::server::order_server::get_instance(&client_requests, &client_responses, "0.0.0.0", 54321);
    server.start();

    using namespace std::literals::chrono_literals;

    while (true) {
        logger->log("%:% %() % Sleeping for a few milliseconds..\n", __FILE__, __LINE__, __func__, kse::utils::get_curren_time_str(&time_str));
        std::this_thread::sleep_for(sleep_time * 1000ms);
    }
}
