#include "ob/order_book.hpp"

namespace ob {

void OrderBook::add_symbol(StockLocate locate, std::string symbol) {
  books_.emplace(locate, SymbolBook{locate, std::move(symbol)});
}

SymbolBook* OrderBook::find(StockLocate locate) {
  auto it = books_.find(locate);
  return (it == books_.end()) ? nullptr : &it->second;
}

const SymbolBook* OrderBook::find(StockLocate locate) const {
  auto it = books_.find(locate);
  return (it == books_.end()) ? nullptr : &it->second;
}

Status OrderBook::apply(const AddEvent& e) {
  auto* b = find(e.locate);
  if (!b) return Status::UnknownSymbol;
  return b->on_add(e);
}

Status OrderBook::apply(const CancelEvent& e) {
  auto* b = find(e.locate);
  if (!b) return Status::UnknownSymbol;
  return b->on_cancel(e);
}

Status OrderBook::apply(const DeleteEvent& e) {
  auto* b = find(e.locate);
  if (!b) return Status::UnknownSymbol;
  return b->on_delete(e);
}

Status OrderBook::apply(const ExecuteEvent& e) {
  auto* b = find(e.locate);
  if (!b) return Status::UnknownSymbol;
  return b->on_execute(e);
}

Status OrderBook::apply(const ReplaceEvent& e) {
  auto* b = find(e.locate);
  if (!b) return Status::UnknownSymbol;
  return b->on_replace(e);
}

} // namespace ob