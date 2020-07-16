/*
 *  stream-buffer -- Buffer data from standard input
 *  Copyright (C) 2020  Marek Marecki
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>


constexpr auto VERSION = "0.0.1";


enum class Unit : uint8_t {
    B,
    KB,
    KiB,
    MB,
    MiB,
    GB,
    GiB,
    TB,
    TiB,
    PB,
    PiB,
};
enum class In_bytes : uint64_t {
    B   = 1,
    KB  = 1000,
    KiB = 1024,
    MB  = KB * KB,
    MiB = KiB * KiB,
    GB  = KB * MB,
    GiB = KiB * MiB,
    TB  = KB * GB,
    TiB = KiB * GiB,
    PB  = KB * TB,
    PiB = KiB * GiB,
};

auto const UNIT_NAMES = std::map<std::string, Unit>{
    {"B", Unit::B},
    {"KB", Unit::KB},
    {"KiB", Unit::KiB},
    {"MB", Unit::MB},
    {"MiB", Unit::MiB},
    {"GB", Unit::GB},
    {"GiB", Unit::GiB},
    {"TB", Unit::TB},
    {"TiB", Unit::TiB},
    {"PB", Unit::PB},
    {"PiB", Unit::PiB},
};
auto const UNIT_SIZES = std::map<Unit, In_bytes>{
    {Unit::B, In_bytes::B},
    {Unit::KB, In_bytes::KB},
    {Unit::KiB, In_bytes::KiB},
    {Unit::MB, In_bytes::MB},
    {Unit::MiB, In_bytes::MiB},
    {Unit::GB, In_bytes::GB},
    {Unit::GiB, In_bytes::GiB},
    {Unit::TB, In_bytes::TB},
    {Unit::TiB, In_bytes::TiB},
    {Unit::PB, In_bytes::PB},
    {Unit::PiB, In_bytes::PiB},
};

static auto parse_buffer_size(std::string_view const s) -> size_t
{
    auto const sep = s.find_first_not_of("0123456789");
    auto const n   = s.substr(0, sep);
    auto const u =
        std::string{(sep == std::string_view::npos) ? "B" : s.substr(sep)};

    try {
        auto const unit = UNIT_SIZES.at(UNIT_NAMES.at(u));
        auto const value =
            std::strtoull(n.data(), nullptr, 0) * static_cast<uint64_t>(unit);

        return value;
    } catch (std::out_of_range const&) {
        std::cerr << "error: invalid unit: `" << u << "'\n";
        exit(1);
    }
}

static auto help_or_version(int argc, char** argv) -> bool
{
    auto verbose         = false;
    auto display_help    = false;
    auto display_version = false;

    for (auto i = 0; i < argc; ++i) {
        auto const each = std::string{argv[i]};
        if (each == "--verbose") {
            verbose = true;
        } else if (each == "--help") {
            display_help = true;
        } else if (each == "-h") {
            display_help = true;
        } else if (each == "--version") {
            display_version = true;
        }
    }

    if (display_help) {
        return execlp("man", "man", "1", "stream-buffer", nullptr);
    }
    if (not display_version) {
        return false;
    }

    std::cout << "stream-buffer version " << VERSION << "\n";
    if (verbose) {
        auto const commit = std::string{STREAM_BUFFER_COMMIT};
        if (commit.empty()) {
            std::cout << "no commit associated with this build\n";
        } else {
            std::cout << "commit: " << STREAM_BUFFER_COMMIT << "\n";
        }
        std::cout << "fingerprint: " << STREAM_BUFFER_FINGERPRINT << "\n";
        std::cout << "siginfo_t's si_int size: " << sizeof(siginfo_t::si_int)
                  << " bytes\n";
    }
    return true;
}

struct Buffer {
    using buffer_type = std::vector<uint8_t>;
    using size_type   = std::vector<uint8_t>::size_type;

  private:
    buffer_type buffer;
    size_type level{0};

  public:
    Buffer(size_type const);

    auto left() const -> size_type;
    auto size() const -> size_type;
    auto full() const -> bool;

    auto head() -> uint8_t*;

    auto drain() -> buffer_type;
    auto grow(size_type const) -> void;
};
Buffer::Buffer(size_type const sz)
{
    buffer.reserve(sz);
    buffer.resize(buffer.capacity());
}
auto Buffer::left() const -> size_type
{
    return (buffer.capacity() - level);
}
auto Buffer::size() const -> size_type
{
    return level;
}
auto Buffer::full() const -> bool
{
    return (level == buffer.capacity());
    ;
}
auto Buffer::head() -> uint8_t*
{
    return (buffer.data() + level);
}
auto Buffer::drain() -> buffer_type
{
    auto x = std::move(buffer);
    x.resize(level);

    buffer.reserve(x.capacity());
    buffer.resize(buffer.capacity());
    level = 0;

    return x;
}
auto Buffer::grow(size_type const n) -> void
{
    level += n;
}

static auto flush(std::vector<uint8_t> const buffer, int const to) -> int
{
    return write(to, buffer.data(), buffer.size());
}
static auto stream_data(std::atomic_bool& sentinel,
                        size_t const initial_buffer_size,
                        int const from,
                        int const to) -> void
{
    std::cerr << "[stream_data] initial buffer size: " << initial_buffer_size
              << " byte(s)\n";

    auto buffer = Buffer{initial_buffer_size};
    while (not sentinel.load()) {
        auto const limit = buffer.left();
        std::cerr << "[stream_data] reading at most " << limit << " byte(s) ("
                  << buffer.size() << " byte(s) in buffer)\n";
        auto const read_size = read(from, buffer.head(), limit);
        if (read_size == 0) {
            std::cerr << "[stream_data] EOF\n";
            kill(getpid(), SIGQUIT);
            flush(buffer.drain(), to);
            break;
        }
        if (read_size <= 0) {
            std::cerr << "[stream_data] error " << read_size << ": "
                      << strerror_r(read_size, nullptr, 0) << "\n";
            kill(getpid(), SIGQUIT);
            flush(buffer.drain(), to);
            break;
        }

        buffer.grow(read_size);
        std::cerr << "[stream_data] " << read_size << " byte(s) in\n";
        std::cerr << "[stream_data] buffer is " << buffer.size()
                  << " byte(s) now\n";

        if (buffer.full()) {
            std::cerr << "[stream_data] draining buffer\n";
            flush(buffer.drain(), to);
        }
    }
}
static auto receive_commands(std::atomic_bool& sentinel) -> void
{
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGHUP);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGPIPE);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGUSR2);

    while (not sentinel.load()) {
        siginfo_t info;
        auto const signal_no = sigwaitinfo(&mask, &info);

        if (signal_no == 0) {
            std::cerr << "[receive_commands] spurious wakeup\n";
            continue;
        }
        if (signal_no == -1) {
            std::cerr << "[receive_commands] error\n";
            continue;
        }

        std::cerr << "[receive_commands] received signal " << signal_no << ": "
                  << strsignal(signal_no) << "\n";

        if (signal_no == SIGHUP) {
            std::cerr << "[receive_commands] flush\n";
        } else if (signal_no == SIGUSR1) {
            std::cerr << "[receive_commands] resize buffer: " << info.si_int
                      << "\n";
        }

        sentinel.store(true);
    }
}

auto main(int argc, char** argv) -> int
{
    if (help_or_version(argc, argv)) {
        return 0;
    }

    auto const initial_buffer_size =
        parse_buffer_size((argc > 1) ? argv[1] : "4KiB");

    {
        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SIGHUP);
        sigaddset(&mask, SIGINT);
        sigaddset(&mask, SIGTERM);
        sigaddset(&mask, SIGPIPE);
        sigaddset(&mask, SIGQUIT);
        sigaddset(&mask, SIGUSR1);
        sigaddset(&mask, SIGUSR2);
        pthread_sigmask(SIG_BLOCK, &mask, nullptr);
    }

    std::atomic_bool sentinel = false;
    auto worker =
        std::thread{stream_data, std::ref(sentinel), initial_buffer_size, 0, 1};
    auto controller = std::thread{receive_commands, std::ref(sentinel)};

    controller.join();
    worker.join();

    std::cerr << "[main] done\n";

    return 0;
}
