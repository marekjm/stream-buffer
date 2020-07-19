/*
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
#include <string.h>

#include <cstdint>
#include <string>
#include <string_view>

// FIXME do not group custom includes with POSIX and C library includes
// clang-format off
#include <stream-buffer/stream-buffer.h>
// clang-format on


namespace Stream_buffer {
auto split_size_spec(std::string_view const s) -> std::pair<size_t, Unit>
{
    auto const sep = s.find_first_not_of("0123456789");
    auto const n   = s.substr(0, sep);
    auto const u =
        std::string{(sep == std::string_view::npos) ? "B" : s.substr(sep)};

    return {std::strtoull(n.data(), nullptr, 0), UNIT_NAMES.at(u)};
}

auto parse_buffer_size(std::string_view const s) -> size_t
{
    auto const [n, u] = split_size_spec(s);
    auto const unit   = UNIT_SIZES.at(u);
    return (n * static_cast<uint64_t>(unit));
}
}  // namespace Stream_buffer
