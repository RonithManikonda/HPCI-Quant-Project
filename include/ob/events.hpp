#pragma once
#include "types.hpp"
#include <optional>
#include <cstdint>

namespace ob {

struct AddEvent {
  StockLocate locate{};
  OrderId order_id{};
  Side side{};
  Qty qty{};
  Price price{};
  // Optional attribution (e.g., MPID from ITCH 'F'), leave empty for now.
  // Use a plain value + flag instead of std::optional for broader compiler compatibility.
  uint32_t mpid{};
  bool has_mpid{};
};

struct CancelEvent {
  StockLocate locate{};
  OrderId order_id{};
  Qty cancel_qty{};
};

struct DeleteEvent {
  StockLocate locate{};
  OrderId order_id{};
};

struct ExecuteEvent {
  StockLocate locate{};
  OrderId order_id{};
  Qty exec_qty{};
};

struct ReplaceEvent {
  StockLocate locate{};
  OrderId old_order_id{};
  OrderId new_order_id{};
  Qty new_qty{};
  Price new_price{};
};

// Later: you can add TradingAction/SystemEvent, etc., without touching core structures.
} // namespace ob