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
#ifndef STREAM_BUFFER_DATA_H
#define STREAM_BUFFER_DATA_H

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>


namespace Stream_buffer {
constexpr auto VERSION = "0.2.0";


struct Buffer {
    using char_type   = uint8_t;
    using buffer_type = std::vector<char_type>;
    using size_type   = buffer_type::size_type;

  private:
    buffer_type buffer;
    size_type level{0};

  public:
    Buffer(size_type const);

    auto left() const -> size_type;
    auto size() const -> size_type;
    auto full() const -> bool;

    auto head() -> char_type*;

    auto drain() -> buffer_type;
    auto get_line(char_type const) -> std::optional<buffer_type>;

    auto grow(size_type const) -> void;
    auto resize(size_type const) -> size_type;
};


enum class Unit : uint8_t {
    B = 1,
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


auto split_size_spec(std::string_view const s) -> std::pair<size_t, Unit>;
auto parse_buffer_size(std::string_view const s) -> size_t;
}  // namespace Stream_buffer

#endif
