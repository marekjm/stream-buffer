#!/usr/bin/env bash

set -e

# If in a repository then just use HEAD hash as build commit hash.
if [[ -d .git ]]; then
    HEAD=$(git rev-parse HEAD)
    RET=$?
    if [[ $RET -eq 0 ]]; then
        echo "$HEAD"
        exit 0
    else
        echo ""
        exit $RET
    fi
fi

echo '(no commit)'
