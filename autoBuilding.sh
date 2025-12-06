#!/bin/bash
set -x

BUILD_DIR="$PWD/build"

mkdir -p "$BUILD_DIR"

rm -rf "$BUILD_DIR"/*

cd "$BUILD_DIR" && cmake .. && make

cp -r "$BUILD_DIR"/../src/include/ "$BUILD_DIR"/../lib/