# `stream-buffer`

> Buffer data received on standard input.

The `stream-buffer` receives data on standard input and buffers it before
sending to its standard output stream.

--------------------------------------------------------------------------------

## Resizing and flushing buffers

To resize buffer to 42 of whatever unit is set:

    /usr/bin/kill -s SIGUSR1 -q 42 -- 123456

To resize buffer to 1 of new unit:

    /usr/bin/kill -s SIGUSR2 -q X -- 123456

...where `X` is the integer index of the desired unit:

- 0: B
- 1: KB
- 2: KiB
- 3: MB
- 4: MiB
- 5: GB
- 6: GiB
- 7: TB
- 8: TiB
- 9: PB
- 10: PiB

To flush the buffer *right now*:

    /usr/bin/kill -s SIGHUP -- 123456

--------------------------------------------------------------------------------

## Why not `stdbuf(1)`?

Because `stdbuf(1)` works by adjusting standard streams settings and then
`execvp(2)`-ing the program. If the "buffered" program adjusts these settings by
itself `stdbuf(1)` does nothing to affect that.

`stream-buffer` works by placing itself as a middleman - it receives data on
standard input, buffers it, and sends it to its standard output. Thus, even if
the program adjusts its stream settings `stream-buffer` it will not matter
because the program cannot affect what `stream-buffer` does.

--------------------------------------------------------------------------------

## License

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
