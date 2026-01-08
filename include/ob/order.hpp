#pragma once
#include "types.hpp"

namespace ob {

struct Level;

// Intrusive linked list node (FIFO queue at each price level).
struct Order {
  OrderId order_id{};
  Qty qty{};
  Price price{};
  Side side{Side::Buy};

  // Attribution hook (optional). Keep small.
  std::uint32_t mpid{0};
  bool has_mpid{false};

  // Intrusive links inside a Level's FIFO list
  Order* prev{nullptr};
  Order* next{nullptr};
  Level* level{nullptr};
};

} // namespace ob