#pragma once


#include <array>
#include <memory>

#include "models/constants.hpp"
#include "models/client_response.hpp"
#include "models/market_update.hpp"
#include "models/order.hpp"

#include "utils/logger.hpp"
#include "utils/memory_pool.hpp"
#include "utils/utils.hpp"

namespace kse::engine {
	class order_book {

	};

	typedef std::array<std::unique_ptr<order_book>, models::MAX_NUM_INSTRUMENTS> order_book_map;
}


