#pragma once

#include <cstdint>

#include "models/constants.hpp"
#include "models/basic_types.hpp"


namespace kse::example::trading_utils
{
    inline constexpr auto side_to_index(models::side_t side) noexcept {
        return side == models::side_t::BUY ? 0 : 1;
    }

    inline constexpr auto side_to_value(models::side_t side) noexcept {
        return side == models::side_t::BUY ? 1 : -1;
    }

    struct risk_config {
        models::quantity_t max_order_size_ = 0;
        models::quantity_t max_position_ = 0;
        double max_loss_ = 0;

        auto to_string() const {
            std::stringstream ss;

            ss << "RiskCfg{"
                << "max-order-size:" << models::quantity_to_string(max_order_size_) << " "
                << "max-position:" << models::quantity_to_string(max_position_) << " "
                << "max-loss:" << max_loss_
                << "}";

            return ss.str();
        }
    };

    struct trade_engine_config {
        models::quantity_t clip_ = 0;
        double threshold_ = 0;
        risk_config risk_cfg_;

        auto toString() const {
            std::stringstream ss;
            ss << "TradeEngineCfg{"
                << "clip:" << models::quantity_to_string(clip_) << " "
                << "thresh:" << threshold_ << " "
                << "risk:" << risk_cfg_.to_string()
                << "}";

            return ss.str();
        }
    };

    using trade_engine_config_map =  std::array<trade_engine_config, models::MAX_NUM_INSTRUMENTS>;
}
