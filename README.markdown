# `stream-buffer`

> Buffer data received on standard input.

The `stream-buffer` receives data on standard input and buffers it before
sending to its standard output stream.

--------------------------------------------------------------------------------

# Installation

Assuming you have GCC installed, these commands should install `stream-buffer`
on your system:

    $ make -j 4
    $ make PREFIX=~/.local install

To uninstall the program:

    $ make PREFIX=~/.local uninstall

Two executables are produced and installed:

- `stream-buffer`: the main executable
- `stream-buffer-ctl`: the support program providing control over running
  buffers

--------------------------------------------------------------------------------

# Examples

Some quick-start commands.

## Starting the buffer

Use the buffer as you would use, e.g., `tee(1)` - pipe the output of some
process to it and let stream buffer handle buffering:

    $ some-process --produce=logs | stream-buffer 64KiB > some-process.log

## Resizing buffers

Resize the buffer while it is streaming data:

    $ stream-buffer-ctl 123456 resize 8KiB

Be aware that the buffer is flushed before resizing.

## Flushing buffers

Flushing buffer is accomplished by sending it the `SIGHUP` signal:

    $ kill -s SIGHUP 123456
    $ stream-buffer-ctl 123456 flush

Do it either using `kill(1)` or the control program.

--------------------------------------------------------------------------------

## Why not `stdbuf(1)`?

Because `stdbuf(1)` works by adjusting standard streams settings and then
`execvp(2)`-ing the program. This means that:

- if the "buffered" program adjusts these settings by itself `stdbuf(1)` does
  nothing to affect that

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
