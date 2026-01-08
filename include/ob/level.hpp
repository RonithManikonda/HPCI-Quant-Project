#pragma once
#include "types.hpp"
#include "order.hpp"
#include <cstdint>

namespace ob {

// Represents one price level on one side for one symbol.
struct Level {
  Price price{};
  std::uint64_t total_qty{0};
  std::uint32_t order_count{0};

  Order* head{nullptr}; // FIFO front (oldest)
  Order* tail{nullptr}; // FIFO back (newest)

  // TODO: implement push_back(Order*), unlink(Order*), empty()
  void push_back(Order* o);
  void unlink(Order* o);
  bool empty() const { return order_count == 0; }
};

} // namespace ob