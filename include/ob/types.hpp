#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace ob {

using OrderId = std::uint64_t;
using Qty = std::uint32_t;

// ITCH prices are commonly fixed-point with 4 implied decimals.
// We'll store as integer "ticks" (e.g., $123.4500 => 1234500 if tick = 1e-4).
using Price = std::int32_t;

using StockLocate = std::uint16_t;

enum class Side : std::uint8_t { Buy, Sell };

inline const char* to_string(Side s) {
  return (s == Side::Buy) ? "B" : "S";
}

// Status codes: keep it explicit; ingestion later can log/metrics on these.
enum class Status : std::uint8_t {
  Ok,
  UnknownSymbol,
  UnknownOrder,
  DuplicateOrder,
  BadQty,
  BadReplace,
  InternalError
};

} // namespace ob