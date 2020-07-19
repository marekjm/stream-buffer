/*
 *  stream-buffer-ctl -- Control stream-buffer processes
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
#include <unistd.h>

#include <cstdint>
#include <iostream>
#include <string>

// FIXME do not group custom includes with POSIX and C library includes
// clang-format off
#include <stream-buffer/stream-buffer.h>
// clang-format on


auto main(int argc, char** argv) -> int
{
    if (argc < 3) {
        return 1;
    }

    auto const pid_of_buffer =
        static_cast<pid_t>(std::strtoull(argv[1], nullptr, 0));
    auto const command = std::string{argv[2]};

    std::cerr << "send '" << command << "' to " << pid_of_buffer << "\n";

    if (command == "flush") {
        kill(pid_of_buffer, SIGHUP);
    } else if (command == "resize") {
        auto const size_spec = std::string{argv[3]};
        std::cerr << "resize buffer to " << size_spec << "\n";

        auto const [size, unit]  = Stream_buffer::split_size_spec(size_spec);
        constexpr auto SIZE_MASK = uint32_t{0x0fffffff};

        if ((size & SIZE_MASK) != size) {
            std::cerr << "error: size too big: maximum size is " << SIZE_MASK
                      << " units\n";
            return 1;
        }

        auto payload = static_cast<uint32_t>((static_cast<uint8_t>(unit) << 28)
                                             | (size & SIZE_MASK));

        std::cerr << "resize to: " << size << " of "
                  << static_cast<uint16_t>(unit) << "\n";
        std::cerr
            << "  i.e.: "
            << (size
                * static_cast<uint64_t>(Stream_buffer::UNIT_SIZES.at(unit)))
            << " byte(s)\n";
        std::cerr << "payload: " << payload << "\n";

        sigval value;
        value.sival_int = payload;
        sigqueue(pid_of_buffer, SIGUSR1, value);
    }

    return 0;
}
