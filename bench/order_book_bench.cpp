#include "utils/utils.hpp"
#include <engine/order_book.hpp>
#include <sys/types.h>
#include <unistd.h>

#include <random>

auto launch(const std::unique_ptr<kse::engine::order_book>& order_book) -> void {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis_price(100, 199);
    std::uniform_int_distribution<> dis_below_price(50, 90);
    std::uniform_int_distribution<> dis_qty(1, 100);
    std::uniform_int_distribution<> dis_side(0, 99);
    std::uniform_int_distribution<> dis_offset(1, 10);

    kse::models::price_t base_price = 90;

    auto start = kse::utils::rdtsc();

    for(int i = 0; i < 1000000; i++) {
        const kse::models::price_t trend_adjustment = (i % 2 == 0) ? 1 : -1;
        base_price += trend_adjustment;
        const kse::models::price_t price = base_price + dis_offset(gen);
        const kse::models::quantity_t qty = dis_qty(gen);
        const kse::models::side_t side = (dis_side(gen) < 50) ? kse::models::side_t::BUY : kse::models::side_t::SELL;

       order_book->add(1, i, side, price, qty);
    }

    auto end = kse::utils::rdtsc();

    std::cout << "Time: " << (end - start) / 2.30  << std::endl;
}

auto run_perf() -> void {
    pid_t pid = fork();

    if(pid == 0) {
        const auto parent_pid = getppid();
        std::cout << "Running perf on parent process: " << parent_pid << std::endl;
        execl("/home/gnik/WSL2-Linux-Kernel/tools/perf/perf", "perf", "stat", "-I", "1000", "-M", "Frontend_Bound,Backend_Bound,Bad_Speculation,Retiring", "-p", parent_pid, (char *)NULL);
    }
}


int main() {
    kse::utils::logger loggerq{ "x.log" };
	kse::models::client_response_queue client_responses{ kse::models::MAX_CLIENT_UPDATES };
	kse::models::market_update_queue market_updates{ kse::models::MAX_MARKET_UPDATES };
	kse::engine::message_handler message_handlers{ &client_responses, &market_updates, &loggerq };

    auto order_book = std::make_unique<kse::engine::order_book>(1, &loggerq, &message_handlers);
    
    //run_perf();
    launch(order_book);
}