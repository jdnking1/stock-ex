#include "matching_engine.hpp"

#include <algorithm>
#include <chrono>

kse::engine::matching_engine::matching_engine(models::client_request_queue* client_requests, models::client_response_queue* client_responses, models::market_update_queue* market_updates):
	incoming_requests_(client_requests), outgoing_responses_(client_responses), outgoing_market_updates_(market_updates), logger_("matching_engine.log")
{
	/*std::ranges::for_each(instrument_order_books_, [](std::unique_ptr<order_book>& order_book) {

	});*/
}

kse::engine::matching_engine::~matching_engine()
{
	running_ = false;

	using namespace std::literals::chrono_literals;
	std::this_thread::sleep_for(1s);

	incoming_requests_ = nullptr;
	outgoing_responses_ = nullptr;
	outgoing_market_updates_ = nullptr;
}

auto kse::engine::matching_engine::start() -> void
{
	running_ = true;
	auto matching_engine_thread = utils::create_thread(-1, [this]() { run(); });
	matching_engine_thread.detach();
}

auto kse::engine::matching_engine::stop() -> void
{
	running_ = false;
}

