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
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

// FIXME do not group custom includes with POSIX and C library includes
// clang-format off
#include <stream-buffer/stream-buffer.h>
// clang-format on


using namespace Stream_buffer;

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
        std::cout << "siginfo_t's si_ptr size: " << sizeof(siginfo_t::si_ptr)
                  << " bytes\n";
    }
    return true;
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
static auto stream_data(Buffer& buffer,
                        std::optional<char> line_buffered,
                        int const from,
                        int const to) -> ssize_t
{
    auto const read_size = read(from, buffer.head(), buffer.left());

    if (read_size == 0) {
        std::cerr << "[buffer] pipe closed\n";
        kill(getpid(), SIGQUIT);
        flush(buffer.drain(), to);
        return 0;
    }
    if (read_size <= 0) {
        kill(getpid(), SIGQUIT);
        flush(buffer.drain(), to);
        return -1;
    }

    buffer.grow(read_size);

    if (buffer.full()) {
        flush(buffer.drain(), to);
        return read_size;
    }

    if (line_buffered.has_value()) {
        if (auto line = buffer.get_line(*line_buffered); line) {
            auto line_buf = std::move(*line);
            line_buf.push_back(*line_buffered);
            flush(std::move(line_buf), to);
        }
    }

    return read_size;
}
static auto buffer_loop(std::atomic_bool& sentinel,
                        int const commands_fd,
                        size_t const initial_buffer_size,
                        std::optional<char> const line_buffered,
                        int const from,
                        int const to) -> void
{
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
                auto const res = stream_data(buffer, line_buffered, from, to);
                if (res <= 0) {
                    return;
                }
            } else if (events[i].data.fd == commands_fd) {
                auto command = Commands::Nop;
                read(commands_fd, &command, 1);
                switch (command) {
                case Commands::Flush:
                    flush(buffer.drain(), to);
                    break;
                case Commands::Resize:
                {
                    std::array<uint8_t, 5> data{0};
                    read(commands_fd, data.begin(), data.size());

                    auto unit = static_cast<Unit>(data[0]);
                    auto size = uint32_t{};
                    std::memcpy(&size, data.begin() + 1, sizeof(size));

                    flush(buffer.drain(), to);

                    auto const new_size =
                        size * static_cast<uint64_t>(UNIT_SIZES.at(unit));
                    buffer.resize(new_size);
                    break;
                }
                case Commands::Nop:
                default:
                    break;
                }
            }
        }
    }

    stream_data(buffer, std::nullopt, from, to);
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

    while (not sentinel.load()) {
        /*
         * See sigaction(2) for details about the siginfo_t structure.
         */
        siginfo_t info;
        auto const signal_no = sigwaitinfo(&mask, &info);

        if (signal_no == 0) {
            continue;
        }
        if (signal_no == -1) {
            continue;
        }

        if (signal_no == SIGHUP) {
            auto const command = Commands::Flush;
            write(commands_fd, reinterpret_cast<uint8_t const*>(&command), 1);
        } else if (signal_no == SIGUSR1) {
            constexpr auto SIZE_MASK = uint32_t{0x0fffffff};
            auto const payload = static_cast<uint32_t>(info.si_value.sival_int);

            auto const unit = static_cast<Unit>(payload >> 28);
            auto const size = static_cast<uint32_t>(payload & SIZE_MASK);

            auto const command = static_cast<uint8_t>(Commands::Resize);
            write(commands_fd, &command, 1);

            std::array<uint8_t, 5> data{0};
            data[0] = static_cast<uint8_t>(unit);
            std::memcpy(data.begin() + 1, &size, sizeof(size));

            write(commands_fd, data.data(), data.size());
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

    auto line_buffered   = false;
    auto line_ending     = '\n';
    auto buffer_size_arg = std::string{"4KiB"};

    {
        auto i = 1;
        for (; i < argc; ++i) {
            auto const each = std::string{argv[i]};
            if (each.find("--") != 0) {
                break;
            }
            if (each == "--line") {
                line_buffered = true;
                continue;
            }
        }
        if (i < argc) {
            buffer_size_arg = argv[i];
        }
    }

    try {
        parse_buffer_size(buffer_size_arg);
    } catch (std::out_of_range const&) {
        std::cerr << "error: invalid size: " << buffer_size_arg << "\n";
        return 1;
    }
    auto const initial_buffer_size = parse_buffer_size(buffer_size_arg);

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

        auto worker = std::thread{
            buffer_loop,
            std::ref(sentinel),
            read_end,
            initial_buffer_size,
            (line_buffered ? std::optional<char>{line_ending} : std::nullopt),
            0,
            1};
        auto controller =
            std::thread{receive_commands, std::ref(sentinel), write_end};

        controller.join();
        worker.join();
    }

    return 0;
}
