#include "OrderBook.hpp"
#include <cmath>

void OrderBook::add_order(OrderId orderId, Side side, Price price, Volume volume) {
  orders_[orderId] = {price, volume, side, orderId};

  if (side == Side::Bid) {
    auto [it, inserted] = bid_levels_.try_emplace(price, volume);
    if (!inserted) {
      it->second += volume;
    }

  } else {
    auto [it, inserted] = ask_levels_.try_emplace(price, volume);
    if (!inserted) {
      it->second += volume;
    }
  }

}

void OrderBook::modify_order(OrderId orderId, Side side, Price price, Volume volume) {
  cancel_order(orderId);
  add_order(orderId, side, price, volume);
}

void OrderBook::cancel_order(OrderId orderId) {
  if (orders_.find(orderId) == orders_.end()) return;

  auto order = orders_[orderId];

  if (order.side == Side::Bid) {
    auto it = bid_levels_.find(order.price);
    it->second -= order.volume;
    if (it->second == 0) {
      bid_levels_.erase(it);
    }

  } else {
    auto it = ask_levels_.find(order.price);
    it->second -= order.volume;
    if (it->second == 0) {
      ask_levels_.erase(it);
    }
  }

  orders_.erase(orderId);
}

void OrderBook::process_event(const Event &e) {
  switch (e.action) {
    case Action::Add: add_order(e.oid, e.side, e.price, e.volume); return;
    case Action::Modify: modify_order(e.oid, e.side, e.price, e.volume); return;
    case Action::Cancel: cancel_order(e.oid); return;
    default: return;
  }
}

double OrderBook::get_price_at_level(bool is_bid, int k) const {
  if (is_bid) {
    if (k >= (int)bid_levels_.size()) return NAN;
    auto it = bid_levels_.begin();
    std::advance(it, k);
    return static_cast<double>(it->first);
  } else {
    if (k >= (int)ask_levels_.size()) return NAN;
    auto it = ask_levels_.begin();
    std::advance(it, k);
    return static_cast<double>(it->first);
  }
}

double OrderBook::get_vol_at_level(bool is_bid, int k) const {
  if (is_bid) {
    if (k >= (int)bid_levels_.size()) return NAN;
    auto it = bid_levels_.begin();
    std::advance(it, k);
    return static_cast<double>(it->second);
  } else {
    if (k >= (int)ask_levels_.size()) return NAN;
    auto it = ask_levels_.begin();
    std::advance(it, k);
    return static_cast<double>(it->second);
  }
}

double OrderBook::get_mid_price() const {
  if (bid_levels_.empty() || ask_levels_.empty()) return NAN;
  return (static_cast<double>(bid_levels_.begin()->first) +
          static_cast<double>(ask_levels_.begin()->first)) / 2.0;
}

double OrderBook::get_spread() const {
  if (bid_levels_.empty() || ask_levels_.empty()) return NAN;
  return static_cast<double>(ask_levels_.begin()->first) -
         static_cast<double>(bid_levels_.begin()->first);
}
