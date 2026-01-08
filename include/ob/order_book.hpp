#pragma once
#include "symbol_book.hpp"
#include <unordered_map>

namespace ob {

// Keeps mapping from locate -> SymbolBook
class OrderBook {
public:
  // Register a symbol (later: from ITCH Stock Directory 'R')
  void add_symbol(StockLocate locate, std::string symbol);

  // Apply events (what your ITCH decoder will call later)
  Status apply(const AddEvent& e);
  Status apply(const CancelEvent& e);
  Status apply(const DeleteEvent& e);
  Status apply(const ExecuteEvent& e);
  Status apply(const ReplaceEvent& e);

  // Queries
  const SymbolBook* find(StockLocate locate) const;
  SymbolBook*       find(StockLocate locate);

private:
  std::unordered_map<StockLocate, SymbolBook> books_;
};

} // namespace ob