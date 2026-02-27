#pragma once

#include <cstdint>

namespace trading {
    using Price = int64_t;
    using Volume = int64_t;
    using OrderId = uint64_t;

    enum class Action : int8_t {
        Add    = 'A',
        Modify = 'M',
        Cancel = 'C',
    };

    enum class Side : int8_t {
        Bid  = 'B',
        Ask  = 'A',
    };

    /**
     * @brief Represents a single market event (L3).
     */
    struct Event {
      Action action;    ///< Action type (ADD, MODIFY, CANCEL)
      Side side;      ///< Side (BID, ASK)
      Price price;    ///< Price level
      Volume volume;     ///< Quantity
      OrderId oid;     ///< Order ID
    };
}
