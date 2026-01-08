#include "ob/symbol_book.hpp"
#include <cassert>
#include <unordered_set>

namespace ob {

// ---------------- OrderPool ----------------

Order* OrderPool::allocate() {
  if (!free_list_.empty()) {
    Order* o = free_list_.back();
    free_list_.pop_back();
    ++live_;
    return o;
  }
  storage_.emplace_back(Order{});
  ++live_;
  return &storage_.back();
}

void OrderPool::free(Order* o) {
  assert(o != nullptr);
  *o = Order{};
  free_list_.push_back(o);
  --live_;
}

// ---------------- Level ----------------

void Level::push_back(Order* o) {
  o->level = this;
  o->prev = tail;
  o->next = nullptr;
  if (tail) {
    tail->next = o;
  } else {
    head = o;
  }
  tail = o;
  ++order_count;
  total_qty += o->qty;
}

void Level::unlink(Order* o) {
  if (o->prev) {
    o->prev->next = o->next;
  } else {
    head = o->next;
  }
  if (o->next) {
    o->next->prev = o->prev;
  } else {
    tail = o->prev;
  }
  --order_count;
  total_qty -= o->qty;
  o->prev = nullptr;
  o->next = nullptr;
  o->level = nullptr;
}

// ---------------- SymbolBook helpers ----------------

Level& SymbolBook::get_or_create_level(Side s, Price p) {
  if (s == Side::Buy) {
    auto [it, inserted] = bids_.emplace(p, Level{});
    if (inserted) it->second.price = p;
    return it->second;
  } else {
    auto [it, inserted] = asks_.emplace(p, Level{});
    if (inserted) it->second.price = p;
    return it->second;
  }
}

void SymbolBook::maybe_erase_level(Side s, Price p) {
  if (s == Side::Buy) {
    auto it = bids_.find(p);
    if (it != bids_.end() && it->second.empty()) bids_.erase(it);
  } else {
    auto it = asks_.find(p);
    if (it != asks_.end() && it->second.empty()) asks_.erase(it);
  }
}

Status SymbolBook::reduce_order_qty(Order* o, Qty delta) {
  if (delta == 0 || delta > o->qty) return Status::BadQty;
  o->qty -= delta;
  o->level->total_qty -= delta;
  if (o->qty == 0) return remove_order_fully(o);
  return Status::Ok;
}

Status SymbolBook::remove_order_fully(Order* o) {
  Level* lvl = o->level;
  lvl->unlink(o);
  orders_.erase(o->order_id);
  if (lvl->empty()) maybe_erase_level(o->side, lvl->price);
  pool_.free(o);
  return Status::Ok;
}

// ---------------- SymbolBook events ----------------

Status SymbolBook::on_add(const AddEvent& e) {
  if (e.qty == 0) return Status::BadQty;
  if (orders_.find(e.order_id) != orders_.end()) return Status::DuplicateOrder;
  Order* o = pool_.allocate();
  o->order_id = e.order_id;
  o->qty = e.qty;
  o->price = e.price;
  o->side = e.side;
  o->mpid = e.mpid;
  o->has_mpid = e.has_mpid;
  o->prev = nullptr;
  o->next = nullptr;
  o->level = nullptr;
  get_or_create_level(e.side, e.price).push_back(o);
  orders_.emplace(e.order_id, o);
  return Status::Ok;
}

Status SymbolBook::on_cancel(const CancelEvent& e) {
  auto it = orders_.find(e.order_id);
  if (it == orders_.end()) return Status::UnknownOrder;
  return reduce_order_qty(it->second, e.cancel_qty);
}

Status SymbolBook::on_delete(const DeleteEvent& e) {
  auto it = orders_.find(e.order_id);
  if (it == orders_.end()) return Status::UnknownOrder;
  return remove_order_fully(it->second);
}

Status SymbolBook::on_execute(const ExecuteEvent& e) {
  auto it = orders_.find(e.order_id);
  if (it == orders_.end()) return Status::UnknownOrder;
  return reduce_order_qty(it->second, e.exec_qty);
}

Status SymbolBook::on_replace(const ReplaceEvent& e) {
  // ITCH semantics: replace creates a NEW order_id and the old order_id disappears.
  if (e.new_qty == 0) return Status::BadReplace;
  auto it_old = orders_.find(e.old_order_id);
  if (it_old == orders_.end()) return Status::UnknownOrder;
  if (orders_.find(e.new_order_id) != orders_.end()) return Status::DuplicateOrder;
  Order* old = it_old->second;
  Side side = old->side;
  std::uint32_t mpid = old->mpid;
  bool has_mpid = old->has_mpid;
  remove_order_fully(old);
  Order* o = pool_.allocate();
  o->order_id = e.new_order_id;
  o->qty = e.new_qty;
  o->price = e.new_price;
  o->side = side;
  o->mpid = mpid;
  o->has_mpid = has_mpid;
  o->prev = nullptr;
  o->next = nullptr;
  o->level = nullptr;
  get_or_create_level(side, e.new_price).push_back(o);
  orders_.emplace(o->order_id, o);
  return Status::Ok;
}

// ---------------- Queries ----------------

TopOfBook SymbolBook::top() const {
  TopOfBook t;

  if (!bids_.empty()) {
    auto it = bids_.begin();
    t.has_bid = true;
    t.bid = LevelView{it->first, it->second.total_qty, it->second.order_count};
  }
  if (!asks_.empty()) {
    auto it = asks_.begin();
    t.has_ask = true;
    t.ask = LevelView{it->first, it->second.total_qty, it->second.order_count};
  }
  return t;
}

std::vector<LevelView> SymbolBook::depth(Side s, std::size_t n) const {
  std::vector<LevelView> out;
  out.reserve(n);

  if (s == Side::Buy) {
    for (auto it = bids_.begin(); it != bids_.end() && out.size() < n; ++it) {
      out.push_back(LevelView{it->first, it->second.total_qty, it->second.order_count});
    }
  } else {
    for (auto it = asks_.begin(); it != asks_.end() && out.size() < n; ++it) {
      out.push_back(LevelView{it->first, it->second.total_qty, it->second.order_count});
    }
  }
  return out;
}

// ---------------- Validation ----------------

bool SymbolBook::validate() const {
  std::unordered_set<Order*> seen;
  seen.reserve(orders_.size());

  auto check_side = [&](const auto& map, Side s) -> bool {
    for (const auto& [price, level] : map) {
      std::uint64_t sum_qty = 0;
      std::uint32_t count = 0;
      Order* prev = nullptr;
      for (Order* o = level.head; o; o = o->next) {
        if (o->level != &level) return false;
        if (o->side != s) return false;
        if (o->price != price) return false;
        if (o->prev != prev) return false;
        prev = o;
        sum_qty += o->qty;
        ++count;
        seen.insert(o);
      }
      if (prev != level.tail) return false;
      if (sum_qty != level.total_qty) return false;
      if (count != level.order_count) return false;
    }
    return true;
  };

  if (!check_side(bids_, Side::Buy)) return false;
  if (!check_side(asks_, Side::Sell)) return false;

  for (const auto& [id, o] : orders_) {
    if (!o) return false;
    if (o->order_id != id) return false;
    if (o->level == nullptr) return false;
    if (seen.find(o) == seen.end()) return false;
  }
  return true;
}

} // namespace ob
