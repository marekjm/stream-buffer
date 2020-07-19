#!/usr/bin/env bash

#
# This script is a dummy source for data to stream. The output does not make any
# sense -- it is just used to simulate a process that produces output for a long
# time so that the buffering can be tested.
#

set -e

while true; do
    dd if=/dev/urandom bs=4K count=1 2>/dev/null |
        base64 |
        grep -Po '[a-f0-9]{2,}' |
        tr -d '\n' |
        fold
    echo
done
