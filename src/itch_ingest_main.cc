#include "ob/ingest/itch.hpp"
#include "ob/ingest/soupbin.hpp"

#include <array>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace {

struct Options {
  std::string host;
  std::string port;
  std::string user;
  std::string pass;
  std::string session;
  std::uint64_t seq{0};
  std::string file;
  std::size_t frames{5};
  bool no_login{false};
  bool verbose{false};
};

std::string trim_ws(std::string_view in) {
  std::size_t start = 0;
  std::size_t end = in.size();
  while (start < end && std::isspace(static_cast<unsigned char>(in[start]))) ++start;
  while (end > start && std::isspace(static_cast<unsigned char>(in[end - 1]))) --end;
  return std::string(in.substr(start, end - start));
}

bool parse_bool(std::string_view value) {
  std::string v = trim_ws(value);
  for (auto& c : v) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return (v == "1" || v == "true" || v == "yes" || v == "y" || v == "on");
}

void load_env_file(const std::string& path, std::unordered_map<std::string, std::string>* out) {
  if (!out) return;
  std::ifstream in(path);
  if (!in) return;
  std::string line;
  while (std::getline(in, line)) {
    std::string trimmed = trim_ws(line);
    if (trimmed.empty() || trimmed[0] == '#') continue;
    auto pos = trimmed.find('=');
    if (pos == std::string::npos) continue;
    std::string key = trim_ws(trimmed.substr(0, pos));
    std::string value = trim_ws(trimmed.substr(pos + 1));
    if (!value.empty() && value.front() == '"' && value.back() == '"' && value.size() >= 2) {
      value = value.substr(1, value.size() - 2);
    }
    if (!key.empty()) (*out)[key] = value;
  }
}

void apply_env_value(Options* opt, const std::string& key, const std::string& value) {
  if (!opt) return;
  if (key == "OB_HOST") opt->host = value;
  else if (key == "OB_PORT") opt->port = value;
  else if (key == "OB_USER") opt->user = value;
  else if (key == "OB_PASS") opt->pass = value;
  else if (key == "OB_SESSION") opt->session = value;
  else if (key == "OB_ITCH_FILE") opt->file = value;
  else if (key == "OB_SEQ" && !value.empty()) opt->seq = static_cast<std::uint64_t>(std::stoull(value));
  else if (key == "OB_FRAMES" && !value.empty()) opt->frames = static_cast<std::size_t>(std::stoull(value));
  else if (key == "OB_NO_LOGIN") opt->no_login = parse_bool(value);
  else if (key == "OB_VERBOSE") opt->verbose = parse_bool(value);
}

void load_env_defaults(Options* opt) {
  std::unordered_map<std::string, std::string> env;
  load_env_file(".env", &env);
  for (const auto& [k, v] : env) apply_env_value(opt, k, v);

  const char* keys[] = {
    "OB_HOST", "OB_PORT", "OB_USER", "OB_PASS", "OB_SESSION",
    "OB_SEQ", "OB_FRAMES", "OB_NO_LOGIN", "OB_VERBOSE", "OB_ITCH_FILE"
  };
  for (const char* key : keys) {
    const char* val = std::getenv(key);
    if (val) apply_env_value(opt, key, val);
  }
}

void print_usage(std::string_view prog) {
  std::cout
    << "Usage:\n"
    << "  " << prog << " --host HOST --port PORT --user USER --pass PASS --session SESSION [--seq N] [--frames N]\n"
    << "  " << prog << " --host HOST --port PORT --no-login [--frames N]\n"
    << "  " << prog << " --file PATH\n"
    << "\n"
    << "Env (.env or environment): OB_HOST, OB_PORT, OB_USER, OB_PASS, OB_SESSION, OB_SEQ, OB_FRAMES, OB_NO_LOGIN, OB_VERBOSE, OB_ITCH_FILE\n";
}

bool parse_args(int argc, char** argv, Options* out) {
  if (!out) return false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    auto require_value = [&](std::string_view name) -> std::string {
      if (i + 1 >= argc) {
        std::cerr << "Missing value for " << name << "\n";
        return {};
      }
      return argv[++i];
    };

    if (arg == "--host") {
      out->host = require_value(arg);
    } else if (arg == "--port") {
      out->port = require_value(arg);
    } else if (arg == "--user") {
      out->user = require_value(arg);
    } else if (arg == "--pass") {
      out->pass = require_value(arg);
    } else if (arg == "--session") {
      out->session = require_value(arg);
    } else if (arg == "--seq") {
      out->seq = static_cast<std::uint64_t>(std::stoull(require_value(arg)));
    } else if (arg == "--file") {
      out->file = require_value(arg);
    } else if (arg == "--frames") {
      out->frames = static_cast<std::size_t>(std::stoull(require_value(arg)));
    } else if (arg == "--no-login") {
      out->no_login = true;
    } else if (arg == "--verbose") {
      out->verbose = true;
    } else if (arg == "--help" || arg == "-h") {
      return false;
    } else {
      std::cerr << "Unknown arg: " << arg << "\n";
      return false;
    }
  }
  return true;
}

void dump_counts(const std::array<std::size_t, 256>& counts) {
  for (std::size_t i = 0; i < counts.size(); ++i) {
    if (counts[i] == 0) continue;
    char type = static_cast<char>(i);
    std::cout << "type " << type << ": " << counts[i] << "\n";
  }
}

int run_file_mode(const Options& opt) {
  std::ifstream in(opt.file, std::ios::binary);
  if (!in) {
    std::cerr << "Failed to open file: " << opt.file << "\n";
    return 1;
  }

  std::vector<std::uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  if (data.empty()) {
    std::cerr << "File is empty: " << opt.file << "\n";
    return 1;
  }

  std::array<std::size_t, 256> counts{};
  std::size_t offset = 0;
  ob::ingest::ItchMessageView msg;
  while (ob::ingest::decode_next_itch(data.data(), data.size(), &offset, &msg)) {
    counts[static_cast<unsigned char>(msg.type)]++;
  }

  if (offset != data.size()) {
    std::cerr << "Stopped early at offset " << offset << ", unknown or incomplete message.\n";
  }

  dump_counts(counts);
  return 0;
}

int run_live_mode(const Options& opt) {
  ob::ingest::SoupBinClient client;
  if (!client.connect_tcp(opt.host, opt.port)) {
    std::cerr << "Failed to connect to " << opt.host << ":" << opt.port << "\n";
    return 1;
  }

  if (!opt.no_login) {
    ob::ingest::SoupBinLogin login{opt.user, opt.pass, opt.session, opt.seq};
    if (!client.send_login(login)) {
      std::cerr << "Failed to send login\n";
      return 1;
    }

    ob::ingest::SoupBinFrame frame;
    if (!client.read_frame(&frame)) {
      std::cerr << "Failed to read login response\n";
      return 1;
    }

    if (frame.type == 'A') {
      std::cout << "Login accepted\n";
    } else if (frame.type == 'J') {
      std::cerr << "Login rejected\n";
      return 1;
    } else {
      std::cout << "Login response type: " << frame.type << "\n";
    }
  }

  std::array<std::size_t, 256> counts{};
  for (std::size_t i = 0; i < opt.frames; ++i) {
    ob::ingest::SoupBinFrame frame;
    if (!client.read_frame(&frame)) {
      std::cerr << "Read failed after " << i << " frames\n";
      break;
    }

    if (opt.verbose) {
      std::cout << "Frame type: " << frame.type << ", bytes=" << frame.payload.size() << "\n";
    }

    if (frame.type == 'S') {
      std::size_t offset = 0;
      ob::ingest::ItchMessageView msg;
      while (ob::ingest::decode_next_itch(frame.payload.data(), frame.payload.size(), &offset, &msg)) {
        counts[static_cast<unsigned char>(msg.type)]++;
      }
      if (offset != frame.payload.size()) {
        std::cerr << "Sequenced payload ended early at offset " << offset << "\n";
      }
    }
  }

  dump_counts(counts);
  return 0;
}

} // namespace

int main(int argc, char** argv) {
  Options opt;
  load_env_defaults(&opt);
  if (!parse_args(argc, argv, &opt)) {
    print_usage(argv[0]);
    return 1;
  }

  if (!opt.file.empty()) {
    return run_file_mode(opt);
  }

  if (opt.host.empty() || opt.port.empty()) {
    std::cerr << "--host and --port are required for live mode\n";
    print_usage(argv[0]);
    return 1;
  }

  if (!opt.no_login && (opt.user.empty() || opt.pass.empty() || opt.session.empty())) {
    std::cerr << "--user, --pass, and --session are required unless --no-login is used\n";
    print_usage(argv[0]);
    return 1;
  }

  return run_live_mode(opt);
}
