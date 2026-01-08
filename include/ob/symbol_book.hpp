#pragma once
#include "types.hpp"
#include "events.hpp"
#include "level.hpp"
#include <deque>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>

namespace ob {

struct DescBid {
  bool operator()(Price a, Price b) const { return a > b; }
};
struct AscAsk {
  bool operator()(Price a, Price b) const { return a < b; }
};

struct LevelView {
  Price price{};
  std::uint64_t qty{};
  std::uint32_t count{};
};

struct TopOfBook {
  bool has_bid{false};
  bool has_ask{false};
  LevelView bid{};
  LevelView ask{};
};

// Simple object pool for Orders (no deletes, reuse slots).
class OrderPool {
public:
  Order* allocate();
  void  free(Order* o);

  // optional sanity
  std::size_t live() const { return live_; }

private:
  std::deque<Order> storage_;
  std::vector<Order*> free_list_;
  std::size_t live_{0};
};

class SymbolBook {
public:
  explicit SymbolBook(StockLocate loc = 0, std::string sym = {})
    : locate_(loc), symbol_(std::move(sym)) {}

  // Book mutation API
  Status on_add(const AddEvent& e);
  Status on_cancel(const CancelEvent& e);
  Status on_delete(const DeleteEvent& e);
  Status on_execute(const ExecuteEvent& e);
  Status on_replace(const ReplaceEvent& e);

  // Queries
  TopOfBook top() const;
  std::vector<LevelView> depth(Side s, std::size_t n) const;

  // Debug / correctness
  bool validate() const;

  std::string_view symbol() const { return symbol_; }
  StockLocate locate() const { return locate_; }

private:
  using BidMap = std::map<Price, Level, DescBid>;
  using AskMap = std::map<Price, Level, AscAsk>;

  // Helpers
  Level& get_or_create_level(Side s, Price p);
  void maybe_erase_level(Side s, Price p);

  Status remove_order_fully(Order* o);
  Status reduce_order_qty(Order* o, Qty delta); // cancels/execs

  StockLocate locate_{0};
  std::string symbol_;

  // Orders by id (L3)
  std::unordered_map<OrderId, Order*> orders_;

  // Price levels (L2 aggregates + FIFO lists)
  BidMap bids_;
  AskMap asks_;

  // Order memory
  OrderPool pool_;
};

} // namespace ob
