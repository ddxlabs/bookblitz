#pragma once

/**
 * @file OrderBook.hpp
 * @brief Core book data structure and event processing logic.
 */

#include "Types.hpp"
#include <map>
#include <functional>

using namespace trading;

class OrderBook {
private:
  std::map<Price, Volume, std::greater<Price>> bid_levels_;
  std::map<Price, Volume, std::less<Price>> ask_levels_;

  struct Order {
    Price price;
    Volume volume;
    Side side;
    OrderId id;
  };

  std::map<OrderId, Order> orders_;

  /** @name Order helpers
   *  Functions for processing book actions.
   *  @{ */

  void add_order(OrderId orderId, Side side, Price price, Volume volume);
  void modify_order(OrderId orderId, Side side, Price price, Volume volume);
  void cancel_order(OrderId orderId);
  /** @} */

public:
  /** @brief Applies an atomic trading event to the book.
   */
  void process_event(const Event &e);

  /**
   * @brief Returns the price at the @p k-th non-empty level (0-indexed).
   * @param is_bid If true, queries the bid side; otherwise the ask side.
   * @param k      Zero-based level index.
   * @return Price at level @p k, or NAN if fewer than @p k+1 levels exist.
   */
  double get_price_at_level(bool is_bid, int k) const;

  /**
   * @brief Returns the total volume at the @p k-th non-empty level (0-indexed).
   * @param is_bid If true, queries the bid side; otherwise the ask side.
   * @param k      Zero-based level index.
   * @return Volume at level @p k, or NAN if fewer than @p k+1 levels exist.
   */
  double get_vol_at_level(bool is_bid, int k) const;

  /**
   * @brief Returns the midpoint between the best bid and best ask.
   * @return Mid price, or NAN if either side of the book is empty.
   */
  double get_mid_price() const;

  /**
   * @brief Returns the spread between the best ask and best bid.
   * @return Ask minus bid, or NAN if either side of the book is empty.
   */
  double get_spread() const;
};
