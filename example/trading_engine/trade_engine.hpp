#pragma once 

#include <functional>
#include <memory>

#include "utils/utils.hpp"
#include "utils/lock_free_queue.hpp"
#include "utils/logger.hpp"

#include "models/client_request.hpp"
#include "models/client_response.hpp"
#include "models/market_update.hpp"

#include "market_order_book.hpp"

#include "feature_engine.hpp"
#include "position_keeper.hpp"
#include "order_manager.hpp"
#include "risk_manager.hpp"

#include "market_maker.hpp"
#include "liquidity_taker.hpp"

namespace kse::example::trading {
    class trade_engine {
    public:
        trade_engine(algo_type_t algo_type,
            const trade_engine_config_map& cfg,
            models::client_request_queue* client_requests,
            models::client_response_queue* client_responses,
            models::market_update_queue* market_updates);

        ~trade_engine();

        auto start() -> void {
            run_ = true;
            auto trade_engine_thread = utils::create_thread(-1, [this] { run(); });
            utils::ASSERT(trade_engine_thread.joinable(), "Failed to start TradeEngine thread.");
           trade_engine_thread.detach();
        }

        auto stop() -> void {
            while (incoming_ogw_responses_->size() || incoming_md_updates_->size()) {
                logger_.log("%:% %() % Sleeping till all updates are consumed ogw-size:% md-size:%\n", __FILE__, __LINE__, __func__,
                    utils::get_curren_time_str(&time_str_), incoming_ogw_responses_->size(), incoming_md_updates_->size());

                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }

            logger_.log("%:% %() % POSITIONS\n%\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                position_keeper_.to_string());

            run_ = false;
        }

     
        auto run() noexcept -> void;

        auto send_client_request(const client_request_internal& client_request) noexcept -> void;
        auto on_order_book_update(instrument_id_t instrument_id, price_t price, side_t side, market_order_book* book) noexcept -> void;
        auto on_trade_update(const market_update* market_update, market_order_book* book) noexcept -> void;

        auto on_order_update(const client_response_internal* client_response) noexcept -> void;

        /// Function wrappers to dispatch order book updates, trade events and client responses to the trading algorithm.
        std::function<void(instrument_id_t instrument_id, price_t price, side_t side, market_order_book* book [[maybe_unused]] )> algo_on_order_book_update_;
        std::function<void(const market_update* market_update, market_order_book* book [[maybe_unused]] )> algo_on_trade_update_;
        std::function<void(const client_response_internal* client_response)> algo_on_order_update_;

        auto init_last_event_time() {
            last_event_time_ = utils::get_current_timestamp();
        }

        auto silent_seconds() {
            return (utils::get_current_timestamp() - last_event_time_) / utils::NANOS_PER_SECS;
        }

        trade_engine(const trade_engine&) = delete;

        trade_engine(const trade_engine&&) = delete;

        trade_engine& operator=(const trade_engine&) = delete;

        trade_engine& operator=(const trade_engine&&) = delete;

    private:
        order_book_map instrument_order_book_;

        client_request_queue* outgoing_ogw_requests_ = nullptr;
        client_response_queue* incoming_ogw_responses_ = nullptr;
        market_update_queue* incoming_md_updates_ = nullptr;

        utils::nananoseconds_t last_event_time_ = 0;
        volatile bool run_ = false;

        std::string time_str_;
        utils::logger logger_;

        feature_engine feature_engine_;

        position_keeper position_keeper_{};

        risk_manager risk_manager_;

        order_manager order_manager_;

        std::unique_ptr<market_maker> mm_algo_ = nullptr;
        std::unique_ptr<liquidity_taker> taker_algo_ = nullptr;

        auto default_algo_on_order_book_update(instrument_id_t instrument_id, price_t price, side_t side, market_order_book* book [[maybe_unused]] ) noexcept -> void {
            logger_.log("%:% %() % instrument:% price:% side:%\n", __FILE__, __LINE__, __func__,
                utils::get_curren_time_str(&time_str_), instrument_id, price_to_string(price).c_str(),
                side_to_string(side).c_str());
        }
        auto default_algo_on_trade_update(const market_update* market_update, market_order_book* book [[maybe_unused]]) noexcept -> void {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                market_update->to_string().c_str());
        }
        auto default_algo_on_order_update(const client_response_internal* client_response) noexcept -> void {
            logger_.log("%:% %() % %\n", __FILE__, __LINE__, __func__, utils::get_curren_time_str(&time_str_),
                client_response->to_string().c_str());
        }
    };
}