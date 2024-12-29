#pragma once

#include <array>
#include <algorithm>
#include <cstdint>
#include "utils/logger.hpp"
#include "trading_utils/trading_utils.hpp"
#include "models/basic_types.hpp"
#include "om_order.hpp"
#include "position_keeper.hpp"


namespace kse::example::trading {
    enum class risk_check_result : uint8_t {
        INVALID = 0,
        ORDER_TOO_LARGE = 1,
        POSITION_TOO_LARGE = 2,
        LOSS_TOO_LARGE = 3,
        ALLOWED = 4
    };

    inline auto risk_check_result_to_string(risk_check_result result) {
        switch (result) {
        case risk_check_result::INVALID:
            return "INVALID";
        case risk_check_result::ORDER_TOO_LARGE:
            return "ORDER_TOO_LARGE";
        case risk_check_result::POSITION_TOO_LARGE:
            return "POSITION_TOO_LARGE";
        case risk_check_result::LOSS_TOO_LARGE:
            return "LOSS_TOO_LARGE";
        case risk_check_result::ALLOWED:
            return "ALLOWED";
        }

        return "";
    }

    struct risk_info {
        const position_info_t* position_info_ = nullptr;

        trading_utils::risk_config risk_cfg_;

        auto check_pretrade_risk(models::side_t side, models::quantity_t qty) const noexcept {
            if (qty > risk_cfg_.max_order_size_) [[unlikely]]
                return risk_check_result::ORDER_TOO_LARGE;
            if (std::abs(position_info_->position_ + trading_utils::side_to_value(side) * static_cast<int32_t>(qty)) > static_cast<int32_t>(risk_cfg_.max_position_)) [[unlikely]]
                return risk_check_result::POSITION_TOO_LARGE;
            if (position_info_->total_pnl_ < risk_cfg_.max_loss_) [[unlikely]]
                return risk_check_result::LOSS_TOO_LARGE;

            return risk_check_result::ALLOWED;
        }

        auto to_string() const {
            std::stringstream ss;
            ss << "RiskInfo" << "["
                << "pos:" << position_info_->to_string() << " "
                << risk_cfg_.to_string()
                << "]";

            return ss.str();
        }
    };

    using instrument_risk_info_map = std::array<risk_info, models::MAX_NUM_INSTRUMENTS>;

    class risk_manager {
    public:
        risk_manager(utils::logger* logger, const position_keeper* pk, const trading_utils::trade_engine_config_map& instruments_cfg) :
            logger_{ logger } {
            for (models::instrument_id_t i = 0; i < models::MAX_NUM_INSTRUMENTS; ++i) {
                instruments_risk_.at(i).position_info_ = pk->get_position_info(i);
                instruments_risk_.at(i).risk_cfg_ = instruments_cfg.at(i).risk_cfg_;
            };
        }

        risk_manager() = delete;

        risk_manager(const risk_manager&) = delete;

        risk_manager(const risk_manager&&) = delete;

        risk_manager& operator=(const risk_manager&) = delete;

        risk_manager& operator=(const risk_manager&&) = delete;
        auto check_pretrade_risk(models::instrument_id_t instrument_id, models::side_t side, models::quantity_t qty) const noexcept {
            return instruments_risk_.at(instrument_id).check_pretrade_risk(side, qty);
        }

    private:
        std::string time_str_;
        utils::logger* logger_ = nullptr;
        instrument_risk_info_map instruments_risk_;
    };
}