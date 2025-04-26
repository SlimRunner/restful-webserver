#!/bin/bash

set -e  # Exit on error

CLEAN=false
ALL=false

while [[ "$#" -gt 0 ]]; do
  case $1 in
    --clean) CLEAN=true ;;
    --all) ALL=true ;;
    *) echo "Unknown option: $1"; exit 1 ;;
  esac
  shift
done

if $CLEAN; then
  rm -rf build build_coverage
else
  mkdir -p build
  cd build
  cmake ..
  if $ALL; then make; fi
  cd ..

  mkdir -p build_coverage
  cd build_coverage
  cmake -DCMAKE_BUILD_TYPE=Coverage ..
  if $ALL; then make coverage; fi
  cd ..
fi
