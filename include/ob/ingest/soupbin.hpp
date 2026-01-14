#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ob::ingest {

struct SoupBinFrame {
  char type{};
  std::vector<std::uint8_t> payload;
};

struct SoupBinLogin {
  std::string username; // width 6
  std::string password; // width 10
  std::string session;  // width 10
  std::uint64_t seq{0};
};

class SoupBinClient {
public:
  SoupBinClient();
  ~SoupBinClient();
  SoupBinClient(const SoupBinClient&) = delete;
  SoupBinClient& operator=(const SoupBinClient&) = delete;

  bool connect_tcp(std::string_view host, std::string_view port);
  void close();

  bool send_login(const SoupBinLogin& login);
  bool send_heartbeat();
  bool read_frame(SoupBinFrame* out);

  int socket_fd() const { return fd_; }

private:
  int fd_{-1};

  bool write_all(const std::uint8_t* data, std::size_t len);
  bool read_exact(std::uint8_t* data, std::size_t len);

  static std::string pad_field(std::string_view input, std::size_t width);
  static std::string format_seq(std::uint64_t seq);
};

} // namespace ob::ingest
