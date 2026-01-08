#include "ob/order_book.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string_view>
#include <vector>

namespace {

std::string format_price(ob::Price p) {
  const bool neg = p < 0;
  std::uint32_t abs = static_cast<std::uint32_t>(neg ? -p : p);
  std::uint32_t whole = abs / 10000;
  std::uint32_t frac = abs % 10000;
  std::ostringstream oss;
  if (neg) oss << "-";
  oss << whole << '.' << std::setw(4) << std::setfill('0') << frac;
  return oss.str();
}

const char* status_name(ob::Status s) {
  switch (s) {
    case ob::Status::Ok: return "Ok";
    case ob::Status::UnknownSymbol: return "UnknownSymbol";
    case ob::Status::UnknownOrder: return "UnknownOrder";
    case ob::Status::DuplicateOrder: return "DuplicateOrder";
    case ob::Status::BadQty: return "BadQty";
    case ob::Status::BadReplace: return "BadReplace";
    case ob::Status::InternalError: return "InternalError";
  }
  return "Unknown";
}

void print_top(const ob::SymbolBook& sb) {
  auto top = sb.top();
  std::cout << "Top: ";
  if (top.has_bid) {
    std::cout << "BID " << format_price(top.bid.price) << " x " << top.bid.qty
              << " (" << top.bid.count << " orders)";
  } else {
    std::cout << "BID --";
  }
  std::cout << " | ";
  if (top.has_ask) {
    std::cout << "ASK " << format_price(top.ask.price) << " x " << top.ask.qty
              << " (" << top.ask.count << " orders)";
  } else {
    std::cout << "ASK --";
  }
  std::cout << "\n";
}

void print_depth(const ob::SymbolBook& sb, ob::Side s, std::size_t n) {
  auto levels = sb.depth(s, n);
  std::cout << (s == ob::Side::Buy ? "Bids:\n" : "Asks:\n");
  if (levels.empty()) {
    std::cout << "  (empty)\n";
    return;
  }
  for (const auto& lv : levels) {
    std::cout << "  " << format_price(lv.price) << " x " << lv.qty
              << " (" << lv.count << " orders)\n";
  }
}

template <typename Event>
void apply_and_log(ob::OrderBook& book, const Event& e, std::string_view label) {
  auto s = book.apply(e);
  std::cout << label << " -> " << status_name(s) << "\n";
}

} // namespace

int main() {
  ob::OrderBook book;
  book.add_symbol(1, "AAPL");
  book.add_symbol(2, "MSFT");

  std::cout << "=== AAPL demo ===\n";
  apply_and_log(book, ob::AddEvent{.locate=1, .order_id=100, .side=ob::Side::Buy, .qty=200, .price=1900000},
                "Add BID #100 200 @ 190.0000");
  apply_and_log(book, ob::AddEvent{.locate=1, .order_id=101, .side=ob::Side::Buy, .qty=100, .price=1900100},
                "Add BID #101 100 @ 190.0100");
  apply_and_log(book, ob::AddEvent{.locate=1, .order_id=102, .side=ob::Side::Buy, .qty=50, .price=1899900},
                "Add BID #102 50 @ 189.9900");
  apply_and_log(book, ob::AddEvent{.locate=1, .order_id=200, .side=ob::Side::Sell, .qty=150, .price=1900500},
                "Add ASK #200 150 @ 190.0500");
  apply_and_log(book, ob::AddEvent{.locate=1, .order_id=201, .side=ob::Side::Sell, .qty=75, .price=1900600},
                "Add ASK #201 75 @ 190.0600");

  if (auto* sb = book.find(1)) {
    print_top(*sb);
    print_depth(*sb, ob::Side::Buy, 5);
    print_depth(*sb, ob::Side::Sell, 5);
  }

  apply_and_log(book, ob::ExecuteEvent{.locate=1, .order_id=100, .exec_qty=120},
                "Execute #100 120");
  apply_and_log(book, ob::CancelEvent{.locate=1, .order_id=101, .cancel_qty=40},
                "Cancel #101 40");
  apply_and_log(book, ob::ReplaceEvent{.locate=1, .old_order_id=200, .new_order_id=210, .new_qty=130, .new_price=1900400},
                "Replace #200 -> #210 130 @ 190.0400");
  apply_and_log(book, ob::DeleteEvent{.locate=1, .order_id=102},
                "Delete #102");

  if (auto* sb = book.find(1)) {
    std::cout << "After updates:\n";
    print_top(*sb);
    print_depth(*sb, ob::Side::Buy, 5);
    print_depth(*sb, ob::Side::Sell, 5);
  }

  std::cout << "\n=== MSFT demo ===\n";
  apply_and_log(book, ob::AddEvent{.locate=2, .order_id=300, .side=ob::Side::Buy, .qty=500, .price=3201500},
                "Add BID #300 500 @ 320.1500");
  apply_and_log(book, ob::AddEvent{.locate=2, .order_id=301, .side=ob::Side::Sell, .qty=400, .price=3202500},
                "Add ASK #301 400 @ 320.2500");
  apply_and_log(book, ob::ExecuteEvent{.locate=2, .order_id=300, .exec_qty=200},
                "Execute #300 200");

  if (auto* sb = book.find(2)) {
    print_top(*sb);
    print_depth(*sb, ob::Side::Buy, 3);
    print_depth(*sb, ob::Side::Sell, 3);
  }

  return 0;
}
