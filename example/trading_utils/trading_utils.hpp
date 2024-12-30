#pragma once

#include <cstdint>
#include <sstream>

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

        auto to_string() const {
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

    enum class algo_type_t : uint8_t {
        INVALID = 0,
        RANDOM = 1,
        MAKER = 2,
        TAKER = 3,
        MAX = 4
    };

    inline auto algo_type_to_string(algo_type_t type) -> std::string {
        switch (type) {
        case algo_type_t::RANDOM:
            return "RANDOM";
        case algo_type_t::MAKER:
            return "MAKER";
        case algo_type_t::TAKER:
            return "TAKER";
        case algo_type_t::INVALID:
            return "INVALID";
        case algo_type_t::MAX:
            return "MAX";
        }

        return "UNKNOWN";
    }

    inline auto string_to_algo_type(const std::string& str) -> algo_type_t {
        for (auto i = static_cast<int>(algo_type_t::INVALID); i <= static_cast<uint8_t>(algo_type_t::MAX); ++i) {
            const auto algo_type = static_cast<algo_type_t>(i);
            if (algo_type_to_string(algo_type) == str)
                return algo_type;
        }

        return algo_type_t::INVALID;
    }
}
