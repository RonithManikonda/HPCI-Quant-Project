#include "ob/ingest/itch.hpp"

namespace ob::ingest {

std::size_t itch_message_size(char type) {
  // TODO: verify these ITCH 5.0 message sizes against the official spec.
  switch (type) {
    case 'R': return 39; // Stock Directory
    case 'A': return 36; // Add Order (no MPID)
    case 'F': return 40; // Add Order (with MPID)
    case 'X': return 23; // Order Cancel
    case 'D': return 19; // Order Delete
    case 'E': return 30; // Order Executed
    case 'C': return 35; // Order Executed With Price
    case 'U': return 35; // Order Replace
    default: return 0;
  }
}

bool decode_next_itch(const std::uint8_t* buffer, std::size_t buffer_size, std::size_t* offset,
                      ItchMessageView* out) {
  if (!offset || !out) return false;
  if (!buffer || *offset >= buffer_size) return false;

  char type = static_cast<char>(buffer[*offset]);
  std::size_t size = itch_message_size(type);
  if (size == 0) return false;
  if (*offset + size > buffer_size) return false;

  out->type = type;
  out->body = buffer + *offset + 1;
  out->body_size = size - 1;
  *offset += size;
  return true;
}

} // namespace ob::ingest
