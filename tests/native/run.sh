#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
binary=$(mktemp /tmp/dmd2-native.XXXXXX)
trap 'rm -f "$binary"' EXIT HUP INT TERM
"${CXX:-c++}" -std=c++11 -g -fsanitize=address,undefined -fno-omit-frame-pointer \
  -I tests/native/stubs -I . tests/native/frame-dimensions.cpp DMDFrame.cpp -o "$binary"
"$binary"
