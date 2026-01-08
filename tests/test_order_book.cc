#include "ob/order_book.hpp"
#include <cassert>
#include <iostream>

static void test_add_cancel_delete() {
  ob::OrderBook book;
  book.add_symbol(1, "AAPL");

  // Add order
  assert(book.apply(ob::AddEvent{.locate=1, .order_id=1, .side=ob::Side::Buy, .qty=100, .price=1000000}) == ob::Status::Ok);

  // Partial cancel
  assert(book.apply(ob::CancelEvent{.locate=1, .order_id=1, .cancel_qty=40}) == ob::Status::Ok);

  // Execute remaining 60
  assert(book.apply(ob::ExecuteEvent{.locate=1, .order_id=1, .exec_qty=60}) == ob::Status::Ok);

  // Order should be gone now
  assert(book.apply(ob::DeleteEvent{.locate=1, .order_id=1}) == ob::Status::UnknownOrder);
}

static void test_price_time_priority() {
  ob::OrderBook book;
  book.add_symbol(1, "AAPL");

  // Two orders same price; FIFO must hold
  assert(book.apply(ob::AddEvent{.locate=1, .order_id=10, .side=ob::Side::Buy, .qty=50, .price=1000000}) == ob::Status::Ok);
  assert(book.apply(ob::AddEvent{.locate=1, .order_id=11, .side=ob::Side::Buy, .qty=60, .price=1000000}) == ob::Status::Ok);

  // Execute 50 from first order => should remove order 10 only
  assert(book.apply(ob::ExecuteEvent{.locate=1, .order_id=10, .exec_qty=50}) == ob::Status::Ok);

  // Now cancel on order 10 should fail, order 11 should exist
  assert(book.apply(ob::CancelEvent{.locate=1, .order_id=10, .cancel_qty=1}) == ob::Status::UnknownOrder);
  assert(book.apply(ob::CancelEvent{.locate=1, .order_id=11, .cancel_qty=10}) == ob::Status::Ok);
}

static void test_replace() {
  ob::OrderBook book;
  book.add_symbol(1, "AAPL");

  assert(book.apply(ob::AddEvent{.locate=1, .order_id=5, .side=ob::Side::Sell, .qty=100, .price=2000000}) == ob::Status::Ok);

  // Replace: old id 5 -> new id 6, new price and qty
  assert(book.apply(ob::ReplaceEvent{.locate=1, .old_order_id=5, .new_order_id=6, .new_qty=120, .new_price=1999900}) == ob::Status::Ok);

  // Old id should be gone
  assert(book.apply(ob::DeleteEvent{.locate=1, .order_id=5}) == ob::Status::UnknownOrder);

  // New id should be mutable
  assert(book.apply(ob::CancelEvent{.locate=1, .order_id=6, .cancel_qty=20}) == ob::Status::Ok);
}

int main() {
  test_add_cancel_delete();
  test_price_time_priority();
  test_replace();
  std::cout << "All tests passed.\n";
}