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
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <sys/epoll.h>
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

enum class Commands : uint8_t {
    Nop,
    Flush,
    Resize,
};

static auto flush(std::vector<uint8_t> const buffer, int const to) -> int
{
    return write(to, buffer.data(), buffer.size());
}
static auto stream_data(Buffer& buffer, int const from, int const to) -> ssize_t
{
    auto const read_size = read(from, buffer.head(), buffer.left());

    if (read_size == 0) {
        std::cerr << "[stream_data] EOF\n";
        kill(getpid(), SIGQUIT);
        flush(buffer.drain(), to);
        return 0;
    }
    if (read_size <= 0) {
        std::cerr << "[stream_data] error " << read_size << ": "
                  << strerror_r(read_size, nullptr, 0) << "\n";
        kill(getpid(), SIGQUIT);
        flush(buffer.drain(), to);
        return -1;
    }

    buffer.grow(read_size);
    std::cerr << "[stream_data] " << read_size << " byte(s) in\n";
    std::cerr << "[stream_data] buffer is " << buffer.size()
              << " byte(s) now\n";

    if (buffer.full()) {
        std::cerr << "[stream_data] draining buffer\n";
        flush(buffer.drain(), to);
    }

    return read_size;
}
static auto buffer_loop(std::atomic_bool& sentinel,
                        int const commands_fd,
                        size_t const initial_buffer_size,
                        int const from,
                        int const to) -> void
{
    std::cerr << "[stream_data] initial buffer size: " << initial_buffer_size
              << " byte(s)\n";

    /*
     * See epoll(7) for more details.
     */
    auto const epoll_fd = epoll_create(2);
    if (epoll_fd == -1) {
        std::cerr << "error: could not epoll(7) fd: " << errno
                  << strerror(errno) << "\n";
        kill(getpid(), SIGQUIT);
        return;
    }
    {
        epoll_event ev;
        ev.events  = EPOLLIN;
        ev.data.fd = from;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, from, &ev) == -1) {
            std::cerr << "error: could not add epoll(7) event for input fd: "
                      << errno << strerror(errno) << "\n";
            kill(getpid(), SIGQUIT);
            return;
        }
    }
    {
        epoll_event ev;
        ev.events  = EPOLLIN;
        ev.data.fd = commands_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, commands_fd, &ev) == -1) {
            std::cerr << "error: could not add epoll(7) event for control fd: "
                      << errno << strerror(errno) << "\n";
            kill(getpid(), SIGQUIT);
            return;
        }
    }

    auto buffer = Buffer{initial_buffer_size};
    while (not sentinel.load()) {
        std::cerr << "[stream_data] reading at most " << buffer.left()
                  << " byte(s) (" << buffer.size() << " byte(s) in buffer)\n";

        std::array<epoll_event, 2> events;
        auto const nfds =
            epoll_wait(epoll_fd, events.data(), events.size(), -1);
        if (nfds == -1) {
            std::cerr << "error: failed call to epoll_wait(2)\n";
            kill(getpid(), SIGQUIT);
            flush(buffer.drain(), to);
            break;
        }
        if (nfds == 0) {
            // FIXME isn't this a bug? no fds are ready and yet epoll_wait()
            // returned?
            continue;
        }

        for (auto i = 0; i < nfds; ++i) {
            if (events[i].data.fd == from) {
                auto const res = stream_data(buffer, from, to);
                if (res <= 0) {
                    return;
                }
            } else if (events[i].data.fd == commands_fd) {
                auto command = Commands::Nop;
                read(commands_fd, &command, 1);
                switch (command) {
                case Commands::Flush:
                    std::cerr << "[stream_data] flush\n";
                    flush(buffer.drain(), to);
                    break;
                case Commands::Resize:
                    // FIXME implement
                    break;
                case Commands::Nop:
                default:
                    break;
                }
            }
        }
    }

    stream_data(buffer, from, to);
    flush(buffer.drain(), to);
}
static auto receive_commands(std::atomic_bool& sentinel, int const commands_fd)
    -> void
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
            auto const command = Commands::Flush;
            write(commands_fd, reinterpret_cast<uint8_t const*>(&command), 1);
        } else if (signal_no == SIGUSR1) {
            std::cerr << "[receive_commands] resize buffer: " << info.si_int
                      << "\n";
        } else if (signal_no == SIGUSR2) {
            std::cerr << "[receive_commands] resize buffer: " << info.si_int
                      << "\n";
        } else {
            sentinel.store(true);
        }
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

    {
        std::atomic_bool sentinel = false;

        auto read_end  = int{-1};
        auto write_end = int{-1};
        {
            /*
             * See pipe2(2) and pipe(7) for more details.
             */
            std::array<int, 2> fds;
            auto res = pipe2(fds.data(), O_DIRECT);
            if (res == -1) {
                std::cerr << "error: could not create pipe: " << errno
                          << strerror(errno) << "\n";
                return 1;
            }
            read_end  = fds[0];
            write_end = fds[1];
        }

        auto worker = std::thread{buffer_loop,
                                  std::ref(sentinel),
                                  read_end,
                                  initial_buffer_size,
                                  0,
                                  1};
        auto controller =
            std::thread{receive_commands, std::ref(sentinel), write_end};

        controller.join();
        worker.join();
    }

    std::cerr << "[main] done\n";

    return 0;
}
