#!/bin/bash

set -e  # Exit on error

usage() {
  echo "Usage: $0 [options]"
  echo "Options:"
  echo "  --clean         Clean the build directories."
  echo "  --build         Build the release version only."
  echo "  --int           Update build with cmake but run integration test only."
  echo "  --cover         Build the coverage version only."
  echo "  --makelist      Run CMake only"
  echo "  -h, --help      Display this help message."
  exit 1
}

function run_cmake_only() {
  mkdir -p build
  cd build
  cmake ..
}

function run_integration_only() {
  mkdir -p build
  cd build
  cmake ..
  make
  ctest -R integration_test
  cd ..
}

function build_release() {
  mkdir -p build
  cd build
  cmake ..
  make
  make test
  cd ..
}

function build_coverage() {
  mkdir -p build_coverage
  cd build_coverage
  cmake -DCMAKE_BUILD_TYPE=Coverage ..
  make coverage
  cd ..
}

CLEAN=false
ALL=false
BUILD_ONLY=false
COVER_ONLY=false
INT_TEST_ONLY=false
MAKE_ONLY=false

while [[ "$#" -gt 0 ]]; do
  case $1 in
    --clean) CLEAN=true ;;
    --build) BUILD_ONLY=true;;
    --int) INT_TEST_ONLY=true;;
    --cover) COVER_ONLY=true;;
    --makelist) MAKE_ONLY=true;;
    -h|--help) usage;;
    *) echo "Unknown option: $1"; usage;;
  esac
  shift
done

if $CLEAN; then
  rm -rf build build_coverage
elif $BUILD_ONLY; then
  build_release
elif $MAKE_ONLY; then
  run_cmake_only
elif $COVER_ONLY; then
  build_coverage
elif $INT_TEST_ONLY; then
  run_integration_only
else
  usage
fi
