#include "ob/ingest/soupbin.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>

namespace ob::ingest {

SoupBinClient::SoupBinClient() = default;

SoupBinClient::~SoupBinClient() {
  close();
}

void SoupBinClient::close() {
  if (fd_ >= 0) {
    ::close(fd_);
    fd_ = -1;
  }
}

bool SoupBinClient::connect_tcp(std::string_view host, std::string_view port) {
  close();

  addrinfo hints{};
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  addrinfo* res = nullptr;
  int rc = ::getaddrinfo(std::string(host).c_str(), std::string(port).c_str(), &hints, &res);
  if (rc != 0) return false;

  for (addrinfo* ai = res; ai; ai = ai->ai_next) {
    int fd = ::socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd < 0) continue;
    if (::connect(fd, ai->ai_addr, ai->ai_addrlen) == 0) {
      fd_ = fd;
      break;
    }
    ::close(fd);
  }

  ::freeaddrinfo(res);
  return fd_ >= 0;
}

std::string SoupBinClient::pad_field(std::string_view input, std::size_t width) {
  if (input.size() >= width) return std::string(input.substr(0, width));
  std::string out(input);
  out.append(width - input.size(), ' ');
  return out;
}

std::string SoupBinClient::format_seq(std::uint64_t seq) {
  std::string s = std::to_string(seq);
  if (s.size() >= 20) return s.substr(s.size() - 20);
  return std::string(20 - s.size(), ' ') + s;
}

bool SoupBinClient::write_all(const std::uint8_t* data, std::size_t len) {
  if (fd_ < 0) return false;
  std::size_t off = 0;
  while (off < len) {
    ssize_t n = ::send(fd_, data + off, len - off, 0);
    if (n <= 0) return false;
    off += static_cast<std::size_t>(n);
  }
  return true;
}

bool SoupBinClient::read_exact(std::uint8_t* data, std::size_t len) {
  if (fd_ < 0) return false;
  std::size_t off = 0;
  while (off < len) {
    ssize_t n = ::recv(fd_, data + off, len - off, 0);
    if (n <= 0) return false;
    off += static_cast<std::size_t>(n);
  }
  return true;
}

bool SoupBinClient::send_login(const SoupBinLogin& login) {
  // TODO: verify field widths and padding rules against your SoupBinTCP spec.
  std::string payload;
  payload.reserve(46);
  payload += pad_field(login.username, 6);
  payload += pad_field(login.password, 10);
  payload += pad_field(login.session, 10);
  payload += format_seq(login.seq);

  std::uint16_t len = static_cast<std::uint16_t>(1 + payload.size());
  std::uint16_t be_len = htons(len);

  std::vector<std::uint8_t> frame;
  frame.reserve(2 + len);
  frame.push_back(static_cast<std::uint8_t>(be_len >> 8));
  frame.push_back(static_cast<std::uint8_t>(be_len & 0xFF));
  frame.push_back(static_cast<std::uint8_t>('L'));
  frame.insert(frame.end(), payload.begin(), payload.end());

  return write_all(frame.data(), frame.size());
}

bool SoupBinClient::send_heartbeat() {
  std::uint16_t len = htons(1);
  std::uint8_t frame[3] = {
    static_cast<std::uint8_t>(len >> 8),
    static_cast<std::uint8_t>(len & 0xFF),
    static_cast<std::uint8_t>('H')
  };
  return write_all(frame, sizeof(frame));
}

bool SoupBinClient::read_frame(SoupBinFrame* out) {
  if (!out) return false;
  std::uint8_t len_bytes[2] = {};
  if (!read_exact(len_bytes, 2)) return false;

  std::uint16_t be_len = static_cast<std::uint16_t>((len_bytes[0] << 8) | len_bytes[1]);
  std::uint16_t len = ntohs(be_len);
  if (len < 1) return false;

  std::uint8_t type = 0;
  if (!read_exact(&type, 1)) return false;

  std::vector<std::uint8_t> payload;
  std::size_t payload_len = len - 1;
  payload.resize(payload_len);
  if (payload_len > 0 && !read_exact(payload.data(), payload_len)) return false;

  out->type = static_cast<char>(type);
  out->payload = std::move(payload);
  return true;
}

} // namespace ob::ingest
