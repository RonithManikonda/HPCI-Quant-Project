#pragma once
#include <cstddef>
#include <cstdint>

namespace ob::ingest {

struct ItchMessageView {
  char type{};
  const std::uint8_t* body{nullptr};
  std::size_t body_size{0};
};

std::size_t itch_message_size(char type);
bool decode_next_itch(const std::uint8_t* buffer, std::size_t buffer_size, std::size_t* offset,
                      ItchMessageView* out);

} // namespace ob::ingest
