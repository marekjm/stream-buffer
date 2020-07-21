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
#include <stream-buffer/stream-buffer.h>

#include <algorithm>
#include <cstdint>
#include <vector>


namespace Stream_buffer {
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
auto Buffer::head() -> char_type*
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
auto Buffer::get_line(char_type const delimiter) -> std::optional<buffer_type>
{
    auto const pos = std::find(buffer.begin(), buffer.end(), delimiter);
    if (pos == buffer.end()) {
        return {};
    }

    auto line = buffer_type{};
    line.reserve(pos - buffer.begin());
    std::copy(buffer.begin(), pos, std::back_inserter(line));

    auto rest = buffer_type{};
    rest.reserve(buffer.capacity());
    std::copy(pos + 1, buffer.begin() + size(), std::back_inserter(rest));

    level = rest.size();
    rest.resize(buffer.capacity());

    buffer = std::move(rest);

    return line;
}
auto Buffer::grow(size_type const n) -> void
{
    level += n;
}
auto Buffer::resize(size_type const n) -> size_type
{
    auto const old_capacity = buffer.capacity();

    buffer.clear();
    buffer.reserve(n);
    buffer.resize(n);
    buffer.shrink_to_fit();

    return old_capacity;
}
}  // namespace Stream_buffer
